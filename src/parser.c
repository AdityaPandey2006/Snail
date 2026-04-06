#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
void freeCommand(Command *cmd) {
    if (cmd==NULL){
        return;
    }
    if(cmd->arguments != NULL){
        for (int i=0;i<cmd->argCount;i++) {
            free(cmd->arguments[i]);
        }
        free(cmd->arguments);
    }
    if(cmd->inputFile){
        free(cmd->inputFile);
    }
    if(cmd->outputFile){
        free(cmd->outputFile);
    }
    cmd->commandName = NULL;
    cmd->arguments = NULL;
    cmd->inputFile = NULL;
    cmd->outputFile = NULL;
    cmd->argCount = 0;
}

void freePipes(Pipeline *pip,int count){
    if(pip==NULL){
        return ;
    }
    for(int i=0;i<count;i++){
        freeCommand(&pip->command[i]);
    }
    free(pip->command);
    pip->command = NULL;   // prevents dangling pointer bugs
    return;
}

Pipeline parsePipes(char* input){
    int capacity=2;
    int count=0;
    Pipeline result;

    result.command=(Command*)malloc(capacity*sizeof(Command));
    result.numCommands=0;

    if(result.command==NULL){
        result.numCommands = 0;
        return result;
    }

    char*saveptr;
    char* token=strtok_r(input,"|",&saveptr); //basically token is a pointer that goes from start ofthe inout to the '|'
    //now simply iterative pass commands tio the parse commands
    while(token!=NULL){
        //to remove the leading spaces or tabs
        while(*token==' ' || *token=='\t' || *token=='\r' || *token=='\n'){
            token++;
        }
        if(*token=='\0'){
            freePipes(&result,count);
            result.numCommands = 0;
            return result;
        }    

        //to remove trailing spaces or tabs
        char*ending=token+strlen(token)-1;
        while(ending>=token && (*ending==' ' || *ending=='\t' || *ending=='\r' || *ending=='\n')){
            *ending='\0';
            ending--;
        }
        if(*token == '\0'){
            freePipes(&result,count);
            result.numCommands = 0;
            return result;
        } 
        //if say the limit is broken reallocate space.,.....
        if(capacity==count){
            capacity*=2;
            Command *temp=(Command*)realloc(result.command,capacity*sizeof(Command));
            if (!temp){
                // cleanup
                freePipes(&result,count);
                result.numCommands = 0;
                return result;
            }
            result.command=temp;
        }

        Command cmd=parseCommand(token);
        
        if(cmd.commandName==NULL){
            freePipes(&result,count);
            result.numCommands=0;
            return result;
        }

        result.command[count]=cmd;
        count++;
        token=strtok_r(NULL,"|",&saveptr);
    }
    //once result.command are allocated trhe respective commands we return 
    result.numCommands=count;
    return result;
}


Command parseCommand(char* input){
    char delimiters[]=" \r\t\n"; 
    Command newCommand={0};
    newCommand.inputFile=NULL;
    newCommand.outputFile=NULL;
    // char delimiters[]=" ";
    int capacity=8;
    int argCount=0;
    char** arguments=(char **)malloc(capacity*sizeof(char*));
    int append=0;
    // char* outputFile=(char*) malloc(sizeof())
    if(arguments==NULL){
        Command emptyCmd;
        emptyCmd.commandName = NULL;
        emptyCmd.arguments = NULL;
        emptyCmd.argCount = 0;
        emptyCmd.inputFile=NULL;
        emptyCmd.outputFile=NULL;
        emptyCmd.append=0;
        return emptyCmd;
    }
    char *saveptr;
    char* token=strtok_r(input,delimiters,&saveptr);
    if(token==NULL){
        free(arguments);
        Command emptyCmd;
        emptyCmd.commandName = NULL;
        emptyCmd.arguments = NULL;
        emptyCmd.argCount = 0;
        emptyCmd.inputFile=NULL;
        emptyCmd.outputFile=NULL;
        emptyCmd.append=0;
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
                errorCmd.inputFile = NULL;
                errorCmd.outputFile = NULL;
                errorCmd.append = 0;
                return errorCmd;
            }
        }
        if(strcmp(token,"<")==0){
            if(newCommand.inputFile != NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.outputFile);
                free(newCommand.inputFile);
                printf("multiple input redirections are not allowed\n");
                Command errorCmd = {0};
                return errorCmd;
            }

            token = strtok_r(NULL,delimiters,&saveptr);
            if(token == NULL || strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.outputFile);
                printf("no arguments provided after the < arrow");
                Command emptyCmd = {0};
                return emptyCmd;
            }

            newCommand.inputFile = malloc(strlen(token) + 1);
            if(newCommand.inputFile == NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.outputFile);
                Command errorCmd = {0};
                return errorCmd;
            }

            strcpy(newCommand.inputFile, token);
            token = strtok_r(NULL,delimiters,&saveptr);
            continue;
        }
        else if(strcmp(token,">")==0){
            if(newCommand.outputFile != NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                printf("multiple output redirections are not allowed\n");
                Command errorCmd = {0};
                return errorCmd;
            }

            token = strtok_r(NULL,delimiters,&saveptr);
            if(token == NULL || strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                printf("no arguments provided after the > arrow");
                Command emptyCmd = {0};
                return emptyCmd;
            }

            newCommand.outputFile = malloc(strlen(token) + 1);
            if(newCommand.outputFile == NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                Command errorCmd = {0};
                return errorCmd;
            }

            strcpy(newCommand.outputFile, token);
            append = 0;

            token = strtok_r(NULL,delimiters,&saveptr);
            continue;
        }
        else if(strcmp(token,">>")==0){
            if(newCommand.outputFile != NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                printf("multiple output redirections are not allowed\n");
                Command errorCmd = {0};
                return errorCmd;
            }

            token = strtok_r(NULL,delimiters,&saveptr);
            if(token == NULL || strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                printf("no arguments provided after the >> arrow");
                Command emptyCmd = {0};
                return emptyCmd;
            }

            newCommand.outputFile = malloc(strlen(token) + 1);
            if(newCommand.outputFile == NULL){
                for(int j = 0; j < argCount; j++){
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                Command errorCmd = {0};
                return errorCmd;
            }

            strcpy(newCommand.outputFile, token);
            append = 1;

            token = strtok_r(NULL,delimiters,&saveptr);
            continue;
        }

        else{
            arguments[argCount]=(char*)malloc(strlen(token)+1);
            if (arguments[argCount] == NULL) {
                for (int j = 0; j < argCount; j++) {
                    free(arguments[j]);
                }
                free(arguments);
                free(newCommand.inputFile);
                free(newCommand.outputFile);
                Command errorCmd = {0};
                return errorCmd;
            }
            strcpy(arguments[argCount],token);
            argCount++;
            token=strtok_r(NULL,delimiters,&saveptr);
        }
        
    }
    arguments[argCount]=NULL;
    newCommand.commandName=arguments[0];
    newCommand.arguments=arguments;
    newCommand.argCount=argCount;
    newCommand.append=append;
    return newCommand;
}
