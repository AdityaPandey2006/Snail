#include<stdio.h>
#include<stdlib.h>
#include "rm.h"
#include<unistd.h>
#include<dirent.h>
#include<stdbool.h>
#include"executor.h"
#include<sys/stat.h>
#include <errno.h>


//not adding remove directory yet as it can cause. problems in testing that we have left for future scope....
//for now only implementing file removal .....using rm with -f as an additional command in it ..for force delete of a file

bool fileExists(const char *path){ 
    if(access(path,F_OK)==0){    //funtion to check existance of file without opening it in linux unistd.h we cna use it in windows but with some extra library calls
        return true;
    }
    else{
        return false;
    }
}

executorResult fileRemoval(const char *path){
    if(!fileExists(path)){
        return (executorResult){0,1};  //error ....
    }
    else{
        //simply use ..remove from stdio if not force remove it acts like mlinux rm anyway
        int status=remove(path);
        if(status==0){
            printf("mah brotha!!...'%s' is removed!!\n",path);
            return (executorResult){0,0};
        }
        else{
            perror("Error removing file");
            fprintf(stderr, "Could not delete file %s : %s\n",path,strerror(errno));
            return (executorResult){0,1};
        }

    }
}

executorResult rmCommand(Command *cmd){

    char **args = cmd->arguments;

    if(args[1] == NULL){
        printf("rm: missing operand\n");
        return (executorResult){0,1};
    }
    int i=1;int termination=0;
    while(args[i]!=NULL){
        char *filePath=args[i];
        executorResult result=fileRemoval(filePath);
        if(result.statusCode==1){
            termination=1;
            printf("Terminated");
            break;
        }
        i++;
    }
    if(termination){
        return (executorResult){0,1};
    }
    else{
        return (executorResult){0,0};
    }
}

