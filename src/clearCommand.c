#include <stdio.h>
#include "clearCommand.h"

executorResult clearCommand(Command* newcommand){
    executorResult result;
    result.shouldExit=0;

    if (newcommand->argCount>1){
        fprintf(stderr,"clear: too many arguments\n");
        result.statusCode=1;
        return result;
    }
    
    printf("\033[H\033[J");

    result.statusCode = 0;
    return result;
}