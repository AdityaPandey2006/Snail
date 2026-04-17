#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "repl.h"
#include "config.h"
#include "executor.h"
#include "parser.h"
#include "fileDump.h"
#include<termios.h>
#include<unistd.h>
#include "commandHistory.h"
#include "terminal.h"
#include<stdbool.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <linux/limits.h>
#define COMMANDSIZE 256

//enable raw mode and disabe raw mode helpers important to read dynamically
void enableRawMode(struct termios *old){
    struct termios new;
    tcgetattr(STDIN_FILENO, old);
    new = *old;
    new.c_lflag &= ~(ICANON | ECHO); // disable buffering + echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    return;
}

void disableRawMode(struct termios *old){
    tcsetattr(STDIN_FILENO, TCSANOW, old);
    return;
}

void snailPrinter(){
    const SnailConfig *config = getSnailConfig();
    if(!config->show_snail_art){
        return;
    }
        printf(
"                                                              d::\n"
"                                                              ''d$:     :h\n"
"                                                                 d$   :$h'\n"
"                              .......                             d$..$h'\n"
"                         ..cc$$$$$$$$$c.                  ...c$$$$$$$$$h\n"
"                       .d$$'        '$$$h.              cc$$$$:::::d$!$h\n"
"                     c$$'              '$$c           .$$$$:::()::d$!$h'\n"
"                   .c$$'                 $$h.        .d$$::::::::d$!!$h\n"
"                 .$$h'                    $$$.       $$$::::::::$$!!$h'\n"
"                .$$h'      .;dd,           $$$.      $$$:::::::d$!!$h'\n"
"                $$$'      dd$$$hh,          $$$.     $$$:::::::d$!!$h\n"
"                $$$      d$$' '$hh.          $$$.   .$$$:::::d$!!!!$h\n"
"                $$$      $$d    $$$           d$$.  $$$:::::d$!!!!$h'\n"
"                $$$      $$$ d  $$$          .d$$$.c$$::::::d$!!!!$h\n"
"                $$$.     '$$$$  $$$         .$$$$$$$$::::::d$$!!!$h'\n"
"                '$$h.           $$$       .d$$:::::::::::::d$$!!!$h\n"
"                  \"$\"$$h        .$$$    ,$$$:::::::::::::::d$$!!!$h'\n"
"..hh              .c$$:$h       $$$'   ,$$$::::::::::::::d$$!!!$h'\n"
"d$$           .c$$$:::$$h,     $$h    $$$:::::::::::::d$$$!!!$h'\n"
"$$$.        .$$h::::::::$$$dddd$h'   $$$::::::::::::dd$$!!!!$h'\n"
"'$$$$c...cc$$h:::::::::::$$$$$$$hhhh$$$::::::::::d$$$$$$$hhh'\n"
" '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$hhhhhhhh'''\n"
    );
return;
}

static void executeConfiguredCommand(const char *commandText){
    if(commandText == NULL || commandText[0] == '\0'){
        return;
    }

    char buffer[COMMANDSIZE] = {0};
    strncpy(buffer, commandText, sizeof(buffer) - 1);

    int hasPipe = 0;
    for(char *cursor = buffer; *cursor != '\0'; cursor++){
        if(*cursor == '|'){
            hasPipe = 1;
            break;
        }
    }

    if(!hasPipe){
        Command newCommand = parseCommand(buffer);
        if(newCommand.commandName != NULL){
            executorResult result = executeCommand(&newCommand);
            (void)result;
        }
        freeCommand(&newCommand);
    }
    else{
        Pipeline newPipe = parsePipes(buffer);
        if(newPipe.numCommands > 0){
            executorResult result = executePipes(&newPipe);
            (void)result;
        }
        freePipes(&newPipe, newPipe.numCommands);
    }
}

static int isTokenDelimiter(char ch){
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void redrawInputLine(const char *input, int len, int cursorPos){
    printf("\33[2K\r");
    printPrompt();
    printf("%s", input);
    for(int i = len; i > cursorPos; i--){
        printf("\b");
    }
    fflush(stdout);
}

static int startsWithIgnoreCase(const char *text, const char *prefix){
    size_t i = 0;
    while(prefix[i] != '\0'){
        if(text[i] == '\0'){
            return 0;
        }
        if(tolower((unsigned char)text[i]) != tolower((unsigned char)prefix[i])){
            return 0;
        }
        i++;
    }
    return 1;
}

static void trimToCommonPrefixIgnoreCase(char *prefix, const char *candidate){
    size_t i = 0;
    while(prefix[i] != '\0' && candidate[i] != '\0' &&
          tolower((unsigned char)prefix[i]) == tolower((unsigned char)candidate[i])){
        i++;
    }
    prefix[i] = '\0';
}

static int resolveSearchDirectory(const char *typedDir, char *resolvedDir, size_t resolvedSize){
    if(typedDir == NULL || typedDir[0] == '\0'){
        if(resolvedSize < 2){
            return 0;
        }
        strcpy(resolvedDir, ".");
        return 1;
    }

    if(typedDir[0] == '~'){
        const char *home = getenv("HOME");
        if(home == NULL){
            return 0;
        }
        if(typedDir[1] == '\0'){
            int written = snprintf(resolvedDir, resolvedSize, "%s", home);
            return written >= 0 && (size_t)written < resolvedSize;
        }
        if(typedDir[1] == '/'){
            int written = snprintf(resolvedDir, resolvedSize, "%s%s", home, typedDir + 1);
            return written >= 0 && (size_t)written < resolvedSize;
        }
        // ~user not supported right now.
        return 0;
    }

    int written = snprintf(resolvedDir, resolvedSize, "%s", typedDir);
    return written >= 0 && (size_t)written < resolvedSize;
}

static int isDirectoryMatch(const char *resolvedDir, const char *entryName){
    char path[PATH_MAX] = {0};
    int written = snprintf(path, sizeof(path), "%s/%s", resolvedDir, entryName);
    if(written < 0 || (size_t)written >= sizeof(path)){
        return 0;
    }

    struct stat st;
    if(stat(path, &st) != 0){
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int replaceInputRange(
    char *input,
    int *len,
    int *cursorPos,
    int start,
    int endExclusive,
    const char *replacement
){
    int oldLen = endExclusive - start;
    int newLen = (int)strlen(replacement);
    int delta = newLen - oldLen;
    if(*len + delta >= COMMANDSIZE){
        return 0;
    }

    memmove(&input[start + newLen], &input[endExclusive], (*len - endExclusive) + 1);
    memcpy(&input[start], replacement, newLen);
    *len += delta;
    *cursorPos = start + newLen;
    return 1;
}

static void handleTabCompletion(char *input, int *len, int *cursorPos){
    if(input == NULL || len == NULL || cursorPos == NULL){
        return;
    }
    if(*cursorPos < 0 || *cursorPos > *len){
        return;
    }

    int tokenStart = *cursorPos;
    while(tokenStart > 0 && !isTokenDelimiter(input[tokenStart - 1])){
        tokenStart--;
    }

    int tokenLen = *cursorPos - tokenStart;
    if(tokenLen <= 0){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }

    char token[PATH_MAX] = {0};
    if((size_t)tokenLen >= sizeof(token)){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }
    memcpy(token, &input[tokenStart], tokenLen);
    token[tokenLen] = '\0';

    char typedDir[PATH_MAX] = {0};
    char namePrefix[PATH_MAX] = {0};
    char *lastSlash = strrchr(token, '/');
    if(lastSlash != NULL){
        size_t dirLen = (size_t)(lastSlash - token + 1);
        if(dirLen >= sizeof(typedDir)){
            write(STDOUT_FILENO, "\a", 1);
            return;
        }
        memcpy(typedDir, token, dirLen);
        typedDir[dirLen] = '\0';
        snprintf(namePrefix, sizeof(namePrefix), "%s", lastSlash + 1);
    }
    else{
        typedDir[0] = '\0';
        snprintf(namePrefix, sizeof(namePrefix), "%s", token);
    }

    char resolvedDir[PATH_MAX] = {0};
    if(!resolveSearchDirectory(typedDir, resolvedDir, sizeof(resolvedDir))){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }

    DIR *dir = opendir(resolvedDir);
    if(dir == NULL){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }

    struct dirent *entry;
    int matchCount = 0;
    char firstMatch[PATH_MAX] = {0};
    char commonPrefix[PATH_MAX] = {0};
    char matchList[64][PATH_MAX];
    int listed = 0;
    size_t prefixLen = strlen(namePrefix);

    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        if(!startsWithIgnoreCase(entry->d_name, namePrefix)){
            continue;
        }

        if(matchCount == 0){
            snprintf(firstMatch, sizeof(firstMatch), "%s", entry->d_name);
            snprintf(commonPrefix, sizeof(commonPrefix), "%s", entry->d_name);
        }
        else{
            trimToCommonPrefixIgnoreCase(commonPrefix, entry->d_name);
        }

        if(listed < (int)(sizeof(matchList) / sizeof(matchList[0]))){
            snprintf(matchList[listed], sizeof(matchList[listed]), "%s", entry->d_name);
            listed++;
        }
        matchCount++;
    }
    closedir(dir);

    if(matchCount == 0){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }

    char completionName[PATH_MAX] = {0};
    if(matchCount == 1){
        snprintf(completionName, sizeof(completionName), "%s", firstMatch);
    }
    else{
        if(strlen(commonPrefix) > prefixLen){
            snprintf(completionName, sizeof(completionName), "%s", commonPrefix);
        }
        else{
            printf("\n");
            for(int i = 0; i < listed; i++){
                printf("%s  ", matchList[i]);
            }
            if(matchCount > listed){
                printf("... ");
            }
            printf("\n");
            return;
        }
    }

    char replacement[PATH_MAX] = {0};
    int written = snprintf(replacement, sizeof(replacement), "%s%s", typedDir, completionName);
    if(written < 0 || (size_t)written >= sizeof(replacement)){
        write(STDOUT_FILENO, "\a", 1);
        return;
    }

    if(matchCount == 1 && isDirectoryMatch(resolvedDir, completionName)){
        size_t replLen = strlen(replacement);
        if(replLen + 1 < sizeof(replacement)){
            replacement[replLen] = '/';
            replacement[replLen + 1] = '\0';
        }
    }

    if(!replaceInputRange(input, len, cursorPos, tokenStart, *cursorPos, replacement)){
        write(STDOUT_FILENO, "\a", 1);
    }
}

int readInput(){
    char input[COMMANDSIZE] = "";
    int cursorPos=0;
    int len = 0;
    struct termios old;
    enableRawMode(&old);
    refreshPromptTimestamp();
    printPrompt();
    int historyIndex = currentSize;
    while(1){
        char c;
        ssize_t readResult = read(STDIN_FILENO, &c, 1);
        if(readResult <= 0){
            if(readResult < 0 && errno == EINTR){
                input[0] = '\0';
                len = 0;
                cursorPos = 0;
                historyIndex = currentSize;
                refreshPromptTimestamp();
                redrawInputLine(input, len, cursorPos);
                continue;
            }
            disableRawMode(&old);
            return 1;
        }
        //enter key
        if(c == '\n'){
            input[len] = '\0';
            printf("\n");
            break;
        }
        //backspace 
        else if(c == 127){
            if(cursorPos > 0){
                // shift left
                for(int i = cursorPos - 1; i < len - 1; i++){
                    input[i] = input[i + 1];
                }
                len--;
                cursorPos--;
                input[len] = '\0';
                redrawInputLine(input, len, cursorPos);
            }
        }
        else if(c == '\t'){
            historyIndex = currentSize;
            handleTabCompletion(input, &len, &cursorPos);
            redrawInputLine(input, len, cursorPos);
        }
        // escape sequence....
        else if(c == 27){
            char seq[2];
            if(read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if(read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            if(seq[0] == '['){

                // ↑ up arrow
                if(seq[1] == 'A'){
                    if(historyIndex > 0){
                        historyIndex--;

                        strncpy(input, History[historyIndex].cmd,COMMANDSIZE-1);
                        input[COMMANDSIZE - 1] = '\0';
                        len = strlen(input);
                        cursorPos=len;
                        redrawInputLine(input, len, cursorPos);
                    }
                }
                // down arrow
                else if(seq[1] == 'B'){
                    if(historyIndex < currentSize - 1){
                        historyIndex++;

                        strncpy(input, History[historyIndex].cmd,COMMANDSIZE-1);
                        input[COMMANDSIZE-1]='\0';
                        len = strlen(input);
                        cursorPos=len;
                        redrawInputLine(input, len, cursorPos);
                    }
                    else{
                        historyIndex = currentSize;
                        len = 0;
                        cursorPos = 0; 
                        input[0] = '\0';
                        redrawInputLine(input, len, cursorPos);
                    }
                }
                //<- arrow
                else if(seq[1]=='D'){
                    if(cursorPos > 0){
                        cursorPos--;
                        printf("\b");
                        fflush(stdout);
                    }
                }
                //-> arrow
                else if(seq[1]=='C'){
                    if(cursorPos < len){
                        printf("\033[C"); // move cursor right
                        cursorPos++;
                        fflush(stdout);
                    }
                }
            }
        }
        // Ctrl + L to clear screen
        else if(c==12){
            printf("\033[H\033[J");
            refreshPromptTimestamp();
            redrawInputLine(input, len, cursorPos);
        }
        //ctrl + D to close the shell 
        else if(c==4){
            disableRawMode(&old);
            printf("BYE SNAILER!!\n");
            return 1;

        }
        else{
            if(len < COMMANDSIZE - 1){
                // shift right
                historyIndex = currentSize;

                for(int i = len; i > cursorPos; i--){
                    input[i] = input[i - 1];
                }
                input[cursorPos] = c;
                len++;
                cursorPos++;
                input[len] = '\0';
                redrawInputLine(input, len, cursorPos);
            }
        }
    }
    disableRawMode(&old);
    if(strlen(input) == 0){
        return 0;
    }
    //now here add a loop that allows the parsing of command accrdint to existance of the pipes
    char *st=input;
    bool isPipe=false;
    while(*st!='\0'){
        if(*st=='|'){
            isPipe=true;
            break;
        }
        st++;
    }
    executorResult result;
    struct timespec startTime;
    struct timespec endTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    if(!isPipe){
        Command newCommand = parseCommand(input);
        // newEntry(&newCommand); //already reading this in time of executor.c
        result=executeCommand(&newCommand);
        freeCommand(&newCommand);
    }
    else{
        Pipeline newPipe=parsePipes(input);
        if(newPipe.numCommands==0){
            fprintf(stderr, "Invalid pipeline\n");
            return 0;
        }
        result=executePipes(&newPipe);
        freePipes(&newPipe,newPipe.numCommands);
    }
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    long long durationMs = (endTime.tv_sec - startTime.tv_sec) * 1000LL +
                           (endTime.tv_nsec - startTime.tv_nsec) / 1000000LL;
    setLastCommandDurationMs(durationMs);
    setLastCommandStatus(result.statusCode);
    return result.shouldExit;
}


void replStart(){
    int dumpCleanSuccess = 1;
    int dumpRemovedCount = 0;

    if(!ensureSnailConfigExists()){
        fprintf(stderr, "Warning: could not create ~/snailShellrc, using in-memory defaults\n");
    }
    loadSnailConfig();

    const SnailConfig *config = getSnailConfig();
    if(config->clear_screen_on_start){
        printf("\033[H\033[J");
    }
    if(config->clean_dump_on_start){
        dumpCleanSuccess = cleanDump(&dumpRemovedCount);
    }
    unloadHistory();//to load the data in an array...
    atexit(loadHistory); //auto save the data
    snailPrinter();

    if(config->startup_command_count > 0){
        executeConfiguredCommand(config->startup_commands[0]);
        if(config->clean_dump_on_start){
            printf("Dump cleanup removed %d entr%s.\n",
                   dumpRemovedCount,
                   dumpRemovedCount == 1 ? "y" : "ies");
            if(!dumpCleanSuccess){
                printf("Warning: Dump cleanup error\n");
            }
        }
        for(int i = 1; i < config->startup_command_count; i++){
            executeConfiguredCommand(config->startup_commands[i]);
        }
    }
    else if(config->clean_dump_on_start){
        printf("Dump cleanup removed %d entr%s.\n",
               dumpRemovedCount,
               dumpRemovedCount == 1 ? "y" : "ies");
        if(!dumpCleanSuccess){
            printf("Warning: Dump cleanup error\n");
        }
    }

    while(1){
        int quit=readInput();
        if(quit){ 
            break;
        }
    }
}
