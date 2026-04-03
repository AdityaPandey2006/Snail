#include<stdio.h>
#include<unistd.h>
#include<dirent.h>
#include "parser.h"
#include <string.h>
#include "commandHistory.h"

historyEntry History[MAX_SIZE];
int currentSize=0;

void checkExistance(){
    FILE *f;
    f=fopen("/mnt/c/Users/ASUS/OneDrive/Documents/GitHub/Snail/history.txt","r"); //read only mode if file not exist returns a NULL value so then we create a new fuile
    if(f==NULL){
        FILE *wp=fopen("/mnt/c/Users/ASUS/OneDrive/Documents/GitHub/Snail/history.txt","a"); //a mode as from a mode we can create a nnew file incase it doesnt exist and if it does it simply is used for appending sometghing
        fclose(wp);
        return;
    }
    else{
        fclose(f);
        return;
    }
}

//this funtion will copy the json file contents into an array of a structure....
void unloadHistory(){  
    
    checkExistance();
    FILE *f=fopen("/mnt/c/Users/ASUS/OneDrive/Documents/GitHub/Snail/history.txt","r");  //to only read and writr
    if (f==NULL) {
        printf("Error: Could not open file.\n");
        return;
    }
    int buffer=256;
    char first[256];
    //now parse the first to hget the current_size
    if(fgets(first, buffer, f) == NULL){
        currentSize = 0;
        fclose(f);
        return;
    }
    sscanf(first, "The current size is : %d", &currentSize);
    //noe that we have the size continue getting the next lines
    int current=0;
    while(fgets(History[current].cmd,buffer,f)){
        //in History[current].cmd the command is now filled
        //parse the command and remove the next line character \n from end
        History[current].cmd[strcspn(History[current].cmd, "\n")] = '\0';
        current++;
    }

    fclose(f);
    return;
}

void newEntry(Command* cmd){
    char buffer[256] = "";
    for(int i = 0; i < cmd->argCount; i++){
        strcat(buffer, cmd->arguments[i]);
        if(i != cmd->argCount - 1){
            strcat(buffer, " ");
        }
    }
    //needed as in the strucure above is a fixed array and argument s is a dynamic different

    if(currentSize<MAX_SIZE){
        strcpy(History[currentSize++].cmd,buffer);//new entry
        return;
    }
    else{
        for(int i=0;i<MAX_SIZE-1;i++){
            strcpy(History[i].cmd,History[i+1].cmd);
        }
        strcpy(History[MAX_SIZE-1].cmd,buffer);
        return;
    }
}

void loadHistory(){
    //enf of every session we should load the array data back into txt file
    FILE*f=fopen("/mnt/c/Users/ASUS/OneDrive/Documents/GitHub/Snail/history.txt","w");
    if(f==NULL){
        printf("Error: Could not save history.\n");
        return;
    }
    fprintf(f,"The current size is : %d\n",currentSize);

    for(int i=0;i<currentSize;i++){
        fprintf(f,"%s\n",History[i].cmd);
    }
    fclose(f);
    return ;
}

char** convertToArgs(char *buffer, int *argCount) {
    char **args = malloc(20 * sizeof(char*)); // max 20 args
    int count = 0;
    char *token = strtok(buffer, " ");

    while(token != NULL) {
        args[count] = malloc(strlen(token) + 1);
        strcpy(args[count], token);

        count++;
        token = strtok(NULL, " ");
    }

    args[count] = NULL;  // important
    *argCount = count;

    return args;
}

char** getprevCmd(){
    if(currentSize==0){
        return NULL;
    }
    char buffer[256];
    strcpy(buffer,History[currentSize-1].cmd);

    int argc;
    char **argv = convertToArgs(buffer, &argc);

    return argv;
}