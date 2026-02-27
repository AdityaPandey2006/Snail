#include <stdio.h>
#include <string.h>
#include <parser.h>
#include <executor.h>

executorResult exitCommand(Command* newCommand){
    executorResult result;
    result.shouldExit=1;
    result.statusCode=0;
    return result;
}