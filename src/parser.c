#include "parser.h"
#include <string.h>
#include<stdlib.h>

void freeCommand(Command *cmd) {
    if (cmd==NULL||cmd->arguments==NULL){
        return;
    }
    for (int i=0;i<cmd->argCount;i++) {
        free(cmd->arguments[i]);
    }
    free(cmd->arguments);
    cmd->commandName = NULL;
    cmd->arguments = NULL;
    cmd->argCount = 0;
}


Command parseCommand(char* input){
    char delimiters[]=" ";
    int capacity=8;
    int argCount=0;
    char** arguments=(char **)malloc(capacity*sizeof(char*));
    if(arguments==NULL){
        Command emptyCmd;
        emptyCmd.commandName = NULL;
        emptyCmd.arguments = NULL;
        emptyCmd.argCount = 0;
        return emptyCmd;
    }
    char* token=strtok(input," ");
    if(token==NULL){
        free(arguments);
        Command emptyCmd;
        emptyCmd.commandName = NULL;
        emptyCmd.arguments = NULL;
        emptyCmd.argCount = 0;
        return emptyCmd;
    }
    while(token!=NULL){
        if(argCount+1>=capacity){
            capacity*=2;
            char** tempArguments=(char**) realloc(arguments,capacity*sizeof(char*));
            if(tempArguments){
                arguments=tempArguments;
            }
            else{
                // Realloc failed — clean everything
                for (int j = 0; j < argCount; j++) {
                    free(arguments[j]);
                }
                free(arguments);

                Command errorCmd;
                errorCmd.commandName = NULL;
                errorCmd.arguments = NULL;
                errorCmd.argCount = 0;
                return errorCmd;
            }
        }
        arguments[argCount]=(char*)malloc(strlen(token)+1);
        if (arguments[argCount] == NULL) {
            for (int j = 0; j < argCount; j++) {
                free(arguments[j]);
            }
            free(arguments);
            Command errorCmd = {NULL, NULL, 0};
            return errorCmd;
        }
        strcpy(arguments[argCount],token);
        argCount++;
        token=strtok(NULL," ");
    }
    arguments[argCount]=NULL;
    Command newCommand;
    newCommand.commandName=arguments[0];
    newCommand.arguments=arguments;
    newCommand.argCount=argCount;
    return newCommand;
}