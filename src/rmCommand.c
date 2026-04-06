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


// rm sends entries to the dump instead of deleting them permanently.

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

void redraw(fileEntry *entries, int count, int cursor) {
    int show = count < 10 ? count : 10;

    // move cursor UP to overwrite previous UI
    printf("\033[%dA", show + 3);

    printf("\033[J"); // clear below

    printf("Use UP/DOWN to navigate, SPACE to select, ENTER to confirm\n\n");

    for (int i = 0; i < show; i++) {
        if (i == cursor) printf("\033[7m");

        printf("[%s] %-30s  %.1f MB  %.0f days old  score: %.1f\n",
            entries[i].selected ? "x" : " ",
            entries[i].name,
            entries[i].size / 1e6,
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
    struct termios old;

    int move= count < 10 ? count : 10;
    printf("\n");  // move below command
    // reserve space so moving up works
    for (int i = 0; i < move + 7; i++) printf("\n");

    fflush(stdout);
    tcflush(STDIN_FILENO, TCIFLUSH);


    enableRawMode(&old);

    while(true){
        redraw(entries,count,cursor); 
        char c=getchar();
        if(c=='x' || c=='X'){
            disableRawMode(&old);
            executorResult result;
            result.shouldExit=0;
            result.statusCode=1;
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


    off_t totalSize=0;
    for(int i=0;i<show;i++){
        if(entries[i].selected==1){
            totalSize+=entries[i].size;
        }
    }

    executorResult result;
    result.shouldExit=0;
    result.statusCode=0;

    printf("\nReclaimable: %.2f MB of space\n",totalSize/1e6);
    //this extra two lines of flush were wriitten to drain extra characers if use accidentally presse extra characters before conformation to not cause wrong confitrm
    printf("Confirm Deletion ? (Y/N): ");
    fflush(stdout);
    int confirmation;
    do {
        confirmation = getchar();
    } while (confirmation == '\n' || confirmation == '\r');

    if(confirmation=='Y' || confirmation=='y'){
        for(int i=0;i<show;i++){
            if(entries[i].selected){
                executorResult tempResult=fileRemoval(entries[i].fullPath);
                if(tempResult.statusCode==1){
                    result.statusCode=1;
                    perror(entries[i].fullPath);
                    printf("Are you sure you wish to continue (Y/N)? :");
                    char cont=getchar();
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
    if(strcmp(newCommand->arguments[1], "-i") == 0) {
        if(newCommand->argCount < 3 || newCommand->arguments[2] == NULL) {
            printf("rm -i: missing directory path\n");
            result.statusCode = 1;
            return result;
        }
        executorResult currResult = interactiveRemoval(newCommand->arguments[2]);
        return currResult;
    }

    for(int i=1;newCommand->arguments[i]!=NULL;i++){
        char* filePath=newCommand->arguments[i];
        executorResult currentResult=fileRemoval(filePath);
        if(currentResult.statusCode==1){
            result.statusCode=1;
            result.shouldExit=0;
            perror(filePath);
            continue;
        }
    }
    //can have multiple error status codes instead of it being 1 for all kinds of rm errors
    return result;
}

//will be implementing an interactive deletion option later
