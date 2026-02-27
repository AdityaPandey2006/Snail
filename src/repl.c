#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "repl.h"
#include "executor.h"
#include "parser.h"
#define COMMANDSIZE 50



int readInput(){
    char input[COMMANDSIZE];
    printPrompt();
    if(fgets(input,sizeof(input),stdin)){
        int enterPosition=strcspn(input,"\n");
        input[enterPosition]='\0';
        if(strlen(input)==0){
            return 0;
        }//hitting enter should bring the prompt back again, command[enterposition makes string length 0]
        else{
            Command newCommand=parseCommand(input);
            executorResult result;
            result=executeCommand(&newCommand);
            freeCommand(&newCommand);
            return result.shouldExit;
        }
    }
    else if(feof(stdin)){
        printf("BYE SNAILER");
        return 1;//ctrl+d should exit the file
    }
    else if(ferror(stdin)){
        printf("Error reading the input");
        return 1;
    }
    return 1;
}



void replStart(){
    while(1){
        int quit=readInput();
        if(quit){
            break;
        }
    }
}