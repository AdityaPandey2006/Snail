#include<stdio.h>
#include<stdlib.h>
#include "rmCommand.h"
#include<unistd.h>
#include<dirent.h>
#include<stdbool.h>
#include"executor.h"
#include<sys/stat.h>
#include <errno.h>



//for now only implementing file removal using rm with -f as an additional command in it for force delete of a file

executorResult fileRemoval(const char *filePath){
    executorResult result;
    //simply use remove from stdio if not force remove it acts like mlinux rm anyway
    if(remove(filePath)==0){
        result.shouldExit=0;
        result.statusCode=0;
    }
    else{
        result.shouldExit=0;
        result.statusCode=1;
    }
    return result;
}

executorResult rmCommand(Command* newCommand){
    executorResult result;
    result.shouldExit=0;
    result.statusCode=0;//in case any deletion fails, this will ater become 1
    if(newCommand->argCount<2){
        printf("file name for rm missing\n");
        result.shouldExit=0;
        result.statusCode=1;
        return result;
    }
    for(int i=1;newCommand->arguments[i]!=NULL;i++){
        char* filePath=newCommand->arguments[i];
        executorResult currentResult=fileRemoval(filePath);
        if(currentResult.statusCode==1){
            result.statusCode=1;
            result.shouldExit=0;
            perror(filePath);
            continue;
        }
    }
    //can have multiple error status codes instead of it being 1 for all kinds of rm errors
    return result;
}

//will be implementing an interactive deletion option later