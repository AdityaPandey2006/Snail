#ifndef PARSER_H
#define PARSER_H

#include <string.h>
#include <stdint.h>



typedef struct{
    char* commandName;//name of the command
    char** arguments;//arguments that it is taking, the commandName will also be an argument and will be null terminated for the external commands we will be implementing later
    int argCount;//0-based indexing for now
}Command;

typedef struct{
    Command *command;  //array of commands 
    int numCommands;
}Pipeline;

Pipeline parsePipes(char* input);
Command parseCommand(char* input);
void freeCommand(Command *cmd);
void freePipes(Pipeline* pip,int count);

#endif