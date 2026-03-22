#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include "cdCommand.h"
#include "mkdirCommand.h"
#include "lsCommand.h"
#include "mvCommand.h"
#include "clearCommand.h"
#include "externalCommand.h"
#include "lsCommand.h"
#include "mvCommand.h"
#include "touchCommand.h"
#include "rmdirCommand.h"
#include "rmCommand.h"
#include "treeCommand.h"
#include "dumpList.h"
#include <stdio.h>
#include <string.h>

executorResult executeCommand(Command* newCommand){
    executorResult result;
    if(newCommand->argCount==0){
        printf("Nothing to Execute");
        result.shouldExit=0;
        result.statusCode=0;
        return result;
    }
    if(strcmp(newCommand->commandName,"cd")==0){
        result=cdCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"mv")==0){
        result=mvCommand(newCommand);//todo: have to make the return type as the executorResult
    }
    else if(strcmp(newCommand->commandName,"ls")==0){
        result=lsCommand(newCommand);//todo: have to make the return type as the executorResult
        mvCommand(newCommand);//todo: have to make the return type as the executorResult
    }
    else if(strcmp(newCommand->commandName,"ls")==0){
        lsCommand(newCommand);//todo: have to make the return type as the executorResult
    }
    else if(strcmp(newCommand->commandName,"exit")==0){
        result=exitCommand();
    }
    else if(strcmp(newCommand->commandName,"mkdir")==0){
        result =mkdirCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"clear")==0){
        result=clearCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"touch")==0){
        result=touchCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"rmdir")==0){
        result=rmdirCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"rm")==0){
        result=rmCommand(newCommand);
    }
    else if (strcmp(newCommand->commandName, "tree") == 0) {
        return treeCommand(newCommand);
    }
    else if (strcmp(newCommand->commandName, "dumplist") == 0) {
        result = dumpList(newCommand);
    }
    else{
        //result=externalCommand(newCommand);
        
    }
    return result;

}
