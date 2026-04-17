#include<stdio.h>
#include<stdlib.h>
#include "rmCommand.h"
#include "fileDump.h"
#include<unistd.h>
#include<dirent.h>
#include<stdbool.h>
#include"executor.h"
#include<sys/types.h>
#include<sys/stat.h>
#include <errno.h>
#include<time.h>
#include<termios.h>
#include "terminal.h"

typedef struct {
    char *fullPath;
    char *name;
    off_t size;
    double lastUsed;
    double score;
    int selected;  // checkbox state
} fileEntry;


// rm sends entries to the dump instead of deleting them permanently 

executorResult fileRemoval(char *filePath){
    executorResult result;
    if(sendToDump(filePath) == 1){
        result.shouldExit=0;
        result.statusCode=0;
    }
    else{
        result.shouldExit=0;
        result.statusCode=1;
    }
    return result;
}

bool isDir(const char *path) {
    struct stat statbuf;
    // Try to get file status
    if (stat(path, &statbuf) != 0) {
        // stat() failed, which means the path likely does not exist
        // or permissions are an issue.
        return false;
    }

    // Check if the file mode indicates a directory
    return S_ISDIR(statbuf.st_mode);
}

int cmp(const void *a,const void *b){
    return ((fileEntry*)b)->score > ((fileEntry*)a)->score ? 1 : -1;
}

static void discardPendingInput(void) {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

static int readDeletionConfirmation(void) {
    int confirmation;
    while(true){
        confirmation = getchar();
        if(confirmation == EOF){
            return 'n';
        }
        if(confirmation == '\n' || confirmation == '\r'){
            continue;
        }
        if(confirmation == '\033'){
            int next = getchar();
            if(next == '['){
                getchar();
            }
            continue;
        }
        while(confirmation != '\n' && confirmation != '\r' && confirmation != EOF){
            int trailing = getchar();
            if(trailing == '\n' || trailing == '\r' || trailing == EOF){
                break;
            }
        }
        if(confirmation == 'Y' || confirmation == 'y' || confirmation == 'N' || confirmation == 'n'){
            return confirmation;
        }
        printf("\nPlease enter Y or N: ");
        fflush(stdout);
    }
}

void redraw(fileEntry *entries, int count, int cursor, int firstDraw) {
    int show = count < 10 ? count : 10;
    if(!firstDraw){
        // Move back to the top of the UI block before repainting it.
        printf("\033[%dA", show + 2);
        printf("\r");
        printf("\033[J");
    }
    printf("Use UP/DOWN to navigate, SPACE to select, ENTER to confirm, Q/X/Ctrl+C to cancel\n\n");
    for (int i = 0; i < show; i++) {
        if (i == cursor) printf("\033[7m");
        printf("[%s] %-30s  %.1f KB  %.0f days old  score: %.1f\n",
            entries[i].selected ? "x" : " ",
            entries[i].name,
            entries[i].size / 1e3,
            entries[i].lastUsed,
            entries[i].score);

        if (i == cursor) printf("\033[0m");
    }
    fflush(stdout);
}

executorResult interactiveRemoval(char*dirPath){
    //check if the argument after -i is valid or not

    if(dirPath == NULL) {
        return (executorResult){0, 1};
    }

    if(!isDir(dirPath)){
        return (executorResult){0,1};
    }

    DIR *dir=opendir(dirPath);
    struct dirent *entry;
    
    //make a dynamic memory allocated for the file data
    int capacity =32;
    int count=0;
    fileEntry *entries=malloc(capacity*(sizeof(fileEntry)));

    while((entry=readdir(dir))!=NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;        
        //now get file details and get score   

        char *fileName=entry->d_name;
        char *filePath = malloc(strlen(dirPath)+strlen(fileName)+2); //for '/' and an extra space for \0  
        sprintf(filePath, "%s/%s", dirPath, fileName);
        //appended fileName on the directoryPath to get the metadata

        struct stat st;
        if(stat(filePath, &st) != 0){
            free(filePath);
            continue;
        }

        if(S_ISDIR(st.st_mode)){
            free(filePath);
            continue;
        }


        // struct stat st;
        stat(filePath, &st);
        time_t now=time(NULL);
        double age=difftime(now,st.st_mtime)/86400; //scaled in terms of days rather than seconds to save computation
        off_t size=st.st_size;
        //larger prioirry to lastUsed
        double score=(age*0.6)+((size/1e6)*0.4);

        fileEntry file;
        file.fullPath=filePath;
        file.name=strdup(entry->d_name); //as direct use fo fileName would point to dirent memory 
        file.lastUsed=age;
        file.size=size;
        file.score=score;
        file.selected=false;

        if(count==capacity){
            capacity*=2;
            entries=realloc(entries,capacity*sizeof(fileEntry));
        }
        entries[count++]=file;
    }
    closedir(dir);

    //now sort the array and display top 10
    qsort(entries,count,sizeof(fileEntry),cmp);
    //now we need to control which one to select and whcib to not by up/down space and enter...
    int show=count<10? count:10;
    int cursor=0;
    int firstDraw=1;
    struct termios old;

    // Reserve exactly the number of lines the UI will occupy.
    for (int i = 0; i < show + 2; i++) printf("\n");

    fflush(stdout);
    discardPendingInput();


    enableRawMode(&old);

    while(true){
        redraw(entries,count,cursor,firstDraw);
        firstDraw=0;
        char c=getchar();
        if(c=='q' || c=='Q' || c=='x' || c=='X' || c==3){
            disableRawMode(&old);
            discardPendingInput(); //so that if from previous inoutif any leftover was remained its nit carriedin the next promport
            printf("\nInteractive removal cancelled.\n");
            for(int i=0;i<count;i++){
                free(entries[i].name);
                free(entries[i].fullPath);
            }
            free(entries);
            executorResult result;
            result.shouldExit=0;
            result.statusCode=0;
            return result;
        }
        if(c=='\033'){
            getchar();
            char arrow=getchar();
            if(arrow=='A' && cursor > 0){cursor--;}
            if(arrow=='B' && cursor<show-1){cursor++;}
        }
        else if(c== ' '){
            entries[cursor].selected^=1; //if already one then by xor it becomes 0 
        }
        else if(c=='\n'){
            break; //confirm
        }
    }
    disableRawMode(&old);
    discardPendingInput();


    off_t totalSize=0;
    for(int i=0;i<show;i++){
        if(entries[i].selected==1){
            totalSize+=entries[i].size;
        }
    }

    executorResult result;
    result.shouldExit=0;
    result.statusCode=0;

    if(totalSize == 0){
        printf("\nNo files selected.\n");
        for(int i=0;i<count;i++){
            free(entries[i].name);
            free(entries[i].fullPath);
        }
        free(entries);
        return result;
    }

    printf("\nReclaimable: %.2f KB of space\n",totalSize/1e3);
    printf("Confirm Deletion ? (Y/N): ");
    fflush(stdout);
    int confirmation = readDeletionConfirmation();

    if(confirmation=='Y' || confirmation=='y'){
        for(int i=0;i<show;i++){
            if(entries[i].selected){
                executorResult tempResult=fileRemoval(entries[i].fullPath);
                if(tempResult.statusCode==1){
                    result.statusCode=1;
                    perror(entries[i].fullPath);
                    printf("Are you sure you wish to continue (Y/N)? :");
                    char cont=readDeletionConfirmation();
                    if(cont=='Y' || cont=='y'){
                        continue;
                    }
                    else{
                        break;
                    }
                }
            }
        }
    }
    discardPendingInput();
    //free all names of from the entries dynamic array
    for(int i=0;i<count;i++){
        free(entries[i].name);
        free(entries[i].fullPath);     
    }
    free(entries);
    return result;
}

executorResult rmCommand(Command* newCommand){
    executorResult result;
    result.shouldExit=0;
    result.statusCode=0;//in case any deletion fails, this will ater become 1
    if(newCommand->argCount<2){
        printf("file name for rm missing\n");
        result.shouldExit=0;
        result.statusCode=1;
        return result;
    }
    int recursive = 0;
    int force = 0;
    int interactive = 0;
    int targetStart = 1;

    for(; targetStart < newCommand->argCount; targetStart++){
        char *arg = newCommand->arguments[targetStart];
        if(arg == NULL){
            break;
        }
        if(arg[0] != '-' || arg[1] == '\0'){
            break;
        }
        if(strcmp(arg, "--") == 0){
            targetStart++;
            break;
        }

        for(int j = 1; arg[j] != '\0'; j++){
            if(arg[j] == 'r' || arg[j] == 'R'){
                recursive = 1;
            }
            else if(arg[j] == 'f'){
                force = 1;
            }
            else if(arg[j] == 'i'){
                interactive = 1;
            }
            else{
                fprintf(stderr, "rm: invalid option -- '%c'\n", arg[j]);
                result.statusCode = 1;
                return result;
            }
        }
    }

    if(interactive){
        if(recursive || force){
            fprintf(stderr, "rm: -i cannot be combined with -r/-f in this mode\n");
            result.statusCode = 1;
            return result;
        }
        if(targetStart >= newCommand->argCount || newCommand->arguments[targetStart] == NULL) {
            printf("rm -i: missing directory path\n");
            result.statusCode = 1;
            return result;
        }
        if(targetStart + 1 < newCommand->argCount && newCommand->arguments[targetStart + 1] != NULL){
            fprintf(stderr, "rm -i: too many operands\n");
            result.statusCode = 1;
            return result;
        }
        executorResult currResult = interactiveRemoval(newCommand->arguments[targetStart]);
        return currResult;
    }

    if(targetStart >= newCommand->argCount || newCommand->arguments[targetStart] == NULL){
        fprintf(stderr, "rm: missing operand\n");
        result.statusCode = 1;
        return result;
    }

    for(int i = targetStart; newCommand->arguments[i] != NULL; i++){
        char *filePath = newCommand->arguments[i];
        struct stat st;
        if(lstat(filePath, &st) != 0){
            if(force && errno == ENOENT){
                continue;
            }
            perror(filePath);
            result.statusCode = 1;
            continue;
        }

        if(S_ISDIR(st.st_mode) && !recursive){
            fprintf(stderr, "rm: cannot remove '%s': is a directory (use rm -rf)\n", filePath);
            result.statusCode = 1;
            continue;
        }

        executorResult currentResult = fileRemoval(filePath);
        if(currentResult.statusCode == 1){
            result.statusCode = 1;
            fprintf(stderr, "rm: failed to remove '%s'\n", filePath);
            continue;
        }
    }
    //can have multiple error status codes instead of it being 1 for all kinds of rm errors
    return result;
}
