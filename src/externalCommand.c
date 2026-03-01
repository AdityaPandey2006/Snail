#include "parser.h"
#include "executor.h"
#include "externalCommand.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>


executorResult externalCommand(Command* newCommand){
    executorResult result;
    int pid=fork();
    //fork returns 0 for the child program

    
    int status;
    
    if(pid==0){
        execvp(newCommand->arguments[0],newCommand->arguments);
        //on success, execvp does not return to the calling process
        //reraching here means execvp has failed
        perror("External command execution failed");
        result.shouldExit=0;
        result.statusCode=1;
        exit(1);
        
    }
    else if(pid<0){
        //indicates error while forking
        perror("External command execution failed, could not fork");
        result.statusCode = 1;
        result.shouldExit = 0;
    }
    else{
        //fork returns pid of child process for the parent program
        waitpid(pid,&status,0);
        if(WIFEXITED(status)){
            result.statusCode=WEXITSTATUS(status);
//will have to change error status codes to -1 everywhere else because WEXITSTATUS returns between 0 and 255
        }
        else{
            result.statusCode=1;
        }
        result.shouldExit=0;
    }
    return result;
}