#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "repl.h"
#include "executor.h"
#include "parser.h"
#include "fileDump.h"
#include<termios.h>
#include<unistd.h>
#include "commandHistory.h"
#include "terminal.h"

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

int readInput(){
    char input[COMMANDSIZE] = "";
    int cursorPos=0;
    int len = 0;
    struct termios old;
    enableRawMode(&old);
    printPrompt();
    int historyIndex = currentSize;
    while(1){
        char c;
        if(read(STDIN_FILENO, &c, 1) <= 0){
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
                // redraw
                printf("\33[2K\r");
                printPrompt();
                printf("%s", input);
                // reposition cursor
                for(int i = len; i > cursorPos; i--){
                    printf("\b");
                }
                fflush(stdout);
            }
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
                        printf("\33[2K\r");
                        printPrompt();

                        strncpy(input, History[historyIndex].cmd,COMMANDSIZE-1);
                        input[COMMANDSIZE - 1] = '\0';
                        len = strlen(input);
                        cursorPos=len;
                        printf("%s", input);
                        fflush(stdout);
                    }
                }
                // down arrow
                else if(seq[1] == 'B'){
                    if(historyIndex < currentSize - 1){
                        historyIndex++;
                        printf("\33[2K\r");
                        printPrompt();

                        strncpy(input, History[historyIndex].cmd,COMMANDSIZE-1);
                        input[COMMANDSIZE-1]='\0';
                        len = strlen(input);
                        cursorPos=len;
                        printf("%s", input);
                        fflush(stdout);
                    }
                    else{
                        historyIndex = currentSize;
                        printf("\33[2K\r");
                        printPrompt();
                        len = 0;
                        cursorPos = 0; 
                        input[0] = '\0';
                        fflush(stdout);
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
            printPrompt();
            printf("%s", input);
            fflush(stdout);
        }
        //ctrl + D to close the shell 
        else if(c==4){
            disableRawMode(&old);
            printf("BYE SNAILER!! See ya again!\n");
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
                // redraw
                printf("\33[2K\r");
                printPrompt();
                printf("%s", input);
                // move cursor back
                for(int i = len; i > cursorPos; i--){
                    printf("\b");
                }
                fflush(stdout);
            }
        }
    }
    disableRawMode(&old);
    if(strlen(input) == 0){
        return 0;
    }
    Command newCommand = parseCommand(input);
    // newEntry(&newCommand); //already reading this in time of executor.c
    executorResult result = executeCommand(&newCommand);
    freeCommand(&newCommand);
    return result.shouldExit;
}


void replStart(){
    int dumpCleanSuccess=cleanDump();
    if(!dumpCleanSuccess){
        printf("Warning: Dump cleanup error\n");
    }
    unloadHistory();//to load the data in an array...
    atexit(loadHistory); //auto save the data
    snailPrinter();
    while(1){
        int quit=readInput();
        if(quit){ 
            break;
        }
    }
}