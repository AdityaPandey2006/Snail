#include <stdio.h>
#include "cdCommand.h"
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include "executor.h"


executorResult cdCommand(Command* newCommand){
    executorResult result;
    if(newCommand->arguments[1]==NULL){
        char*Home=getenv("HOME");
        if(!Home){
            fprintf(stderr,"Home not set\n");
            result.shouldExit=0;
            result.statusCode=1;//indicates an errored status code
            return result;
        }
        if(chdir(Home)==0){
            result.shouldExit=0;
            result.statusCode=0;
        }
        else{
            perror("cd");
            result.shouldExit=0;
            result.statusCode=1;
        }
    }
    else if(newCommand->arguments[2]!=NULL){
        fprintf(stderr,"Too many arguments\n");
        result.shouldExit=0;
        result.statusCode=1;
    }
    else{
        if(chdir(newCommand->arguments[1])==0){
            result.shouldExit=0;
            result.statusCode=0;

        }
        else{
            perror("cd");
            result.shouldExit=0;
            result.statusCode=1;
        }
    }
    return result;
}
