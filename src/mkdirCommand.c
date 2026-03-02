#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "mkdirCommand.h"

executorResult mkdirCommand(Command* newcommand){
    executorResult result;
    result.shouldExit=0;

    if (newcommand->argCount<2){
        fprintf(stderr,"mkdir:missing operand");
        result.statusCode=1;
        return result;
    }

    for (int i=1;i<newcommand->argCount;i++){
        if(mkdir(newcommand->arguments[i],0755)!=0){
            perror("mkdir");
            result.statusCode=1;
        }else {
            result.statusCode=0;
        } 
    }
    return result;
}