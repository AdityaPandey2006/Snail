#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include "cdCommand.h"
#include "mkdirCommand.h"
#include "clearCommand.h"
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
        // mvCommand(newCommand);//todo: have to make the return type as the executorResult
    }
    else if(strcmp(newCommand->commandName,"ls")==0){
        // lsCommand(newCommand);//todo: have to make the return type as the executorResult
    }
    else if(strcmp(newCommand->commandName,"exit")==0){
        result=exitCommand();
    }
    else if(strcmp(newCommand->commandName,"mkdir")==0){
        result = mkdirCommand(newCommand);
    }
    else if(strcmp(newCommand->commandName,"clear")==0){
        result=clearCommand(newCommand);
    }
    else{
        // executeExternal(newCommand);
    }
    return result;

}
