#include <stdio.h>
#include "cdCommand.h"
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "parser.h"
#include "executor.h"
#include <linux/limits.h>
static const char* resolveCdPath(const char *inputPath, char *expandedPath, size_t expandedSize){
    if(inputPath == NULL){
        return NULL;
    }

    if(inputPath[0] != '~'){
        return inputPath;
    }

    if(inputPath[1] != '\0' && inputPath[1] != '/'){
        return inputPath;
    }

    char *home = getenv("HOME");
    if(home == NULL){
        return NULL;
    }

    int written = snprintf(expandedPath, expandedSize, "%s%s", home, inputPath + 1);
    if(written < 0 || (size_t)written >= expandedSize){
        return NULL;
    }

    return expandedPath;
}

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
        char expandedPath[PATH_MAX] = {0};
        const char *targetPath = resolveCdPath(newCommand->arguments[1], expandedPath, sizeof(expandedPath));
        if(targetPath == NULL){
            fprintf(stderr, "cd: invalid home path\n");
            result.shouldExit=0;
            result.statusCode=1;
        }
        else if(chdir(targetPath)==0){
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
