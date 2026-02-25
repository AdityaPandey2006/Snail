#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "repl.h"
#define COMMANDSIZE 50



int readInput(){
    char command[COMMANDSIZE];
    printPrompt();
    if(fgets(command,sizeof(command),stdin)){
        int enterPosition=strcspn(command,"\n");
        command[enterPosition]='\0';
        if(!strcmp(command,"exit")){
            printf("BYE SNAILER");
            return 1;
        }//TODO: add space-trimming logic to handle empty commands like enter
        else if(strlen(command)==0){
            return 0;
        }//hitting enter should bring the prompt back again, command[enterposition makes string length 0]
        else{
            printf("Invalid Snail Command \n");
            return 0;
        }//for now all commands are invalid
//else block will change once parser is included
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