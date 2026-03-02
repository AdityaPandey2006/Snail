#include "prompt.h"
#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>

void printPrompt(){
    char workingDirectory[PATH_MAX];
    getcwd(workingDirectory,PATH_MAX);//PATH_MAX is maximum size file path string 
    printf("%s>\n",workingDirectory);
    // printf("snail>");
    fflush(stdout);
}