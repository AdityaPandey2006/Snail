#include<stdio.h>
#include "cd.h"
#include<stdlib.h>
#include<unistd.h>


int commandCd(char**args){
    if(args[1]==NULL){
        char*Home=getenv("HOME");
        if(!Home){
            fprintf(stderr,"Home not set\n");
            return 1;
        }
        if(chdir(Home)!=0){
            perror("cd");
        }
    }
    else if(args[2]!=NULL){
        fprintf(stderr,"Too many arguments\n");
    }
    else{
        if(chdir(args[1])!=0){
            perror("cd");
        }
    }
    return 1;
}
