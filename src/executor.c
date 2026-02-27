#include "parser.h"
#include <stdio.h>

void executeCommand(Command* newCommand){
    if(newCommand->argCount==0){
        printf("Nothing to Execute");
        return;
    }
    if(strcmp(newCommand->commandName,"cd")==0){
        cdCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"mv")==0){
        mvCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"ls")==0){
        lsCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"exit")==0){
        lsCommand(newCommand);
    }
    else{

    }

}
