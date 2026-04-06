#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include "cdCommand.h"
#include "mkdirCommand.h"
#include "lsCommand.h"
#include "mvCommand.h"
#include "clearCommand.h"
#include "externalCommand.h"
#include "lsCommand.h"
#include "rmdirCommand.h"
#include "rmCommand.h"
#include "treeCommand.h"
#include "dumpList.h"
#include "touchCommand.h"
#include <stdio.h>
#include <string.h>
#include "commandHistory.h"
#include<sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

int setupInputRedirection(Command* newCommand){
    if(newCommand == NULL || newCommand->inputFile == NULL){
        return 0;
    }

    int fileDescriptor = open(newCommand->inputFile, O_RDONLY);
    if(fileDescriptor < 0){
        perror("open failed");
        return 1;
    }

    if(dup2(fileDescriptor, STDIN_FILENO) < 0){
        perror("dup2 failed");
        close(fileDescriptor);
        return 1;
    }

    close(fileDescriptor);
    return 0;
}

int setupOutputRedirection(Command* newCommand){
    if(newCommand == NULL || newCommand->outputFile == NULL){
        return 0;
    }

    int flags = O_WRONLY | O_CREAT;
    if(newCommand->append){
        flags |= O_APPEND;
    }
    else{
        flags |= O_TRUNC;
    }

    int fileDescriptor = open(newCommand->outputFile, flags, 0644);
    if(fileDescriptor < 0){
        perror("open failed");
        return 1;
    }

    if(dup2(fileDescriptor, STDOUT_FILENO) < 0){
        perror("dup2 failed");
        close(fileDescriptor);
        return 1;
    }

    close(fileDescriptor);
    return 0;
}

int isBuiltIn(Command* newCommand){
    if(strcmp(newCommand->commandName,"cd")==0||
    (strcmp(newCommand->commandName,"mv")==0)||
    (strcmp(newCommand->commandName,"ls")==0)||
    (strcmp(newCommand->commandName,"exit")==0)||
    (strcmp(newCommand->commandName,"mkdir")==0)||
    (strcmp(newCommand->commandName,"touch")==0)||
    (strcmp(newCommand->commandName,"rmdir")==0)||
    (strcmp(newCommand->commandName,"rm")==0)||
    (strcmp(newCommand->commandName,"tree")==0)
    ){
        return 1;
    }
    return 0;
}


executorResult executeCommand(Command* newCommand){
    executorResult result;
    newEntry(newCommand);
    int savedSTDIN=-1;
    int savedSTDOUT=-1;
    if(newCommand->argCount==0){
        printf("Nothing to Execute");
        result.shouldExit=0;
        result.statusCode=0;
        return result;
    }
    if(isBuiltIn(newCommand)){
        if(newCommand->inputFile){
            savedSTDIN = dup(STDIN_FILENO);
            if(savedSTDIN < 0){
                perror("dup failed");
                result.shouldExit = 0;
                result.statusCode = 1;
                return result;
            }

            if(setupInputRedirection(newCommand) != 0){
                close(savedSTDIN);
                result.shouldExit = 0;
                result.statusCode = 1;
                return result;
            }
        }
        if(newCommand->outputFile){
            savedSTDOUT = dup(STDOUT_FILENO);
            if(savedSTDOUT < 0){
                perror("dup failed");
                if(savedSTDIN != -1){
                    dup2(savedSTDIN, STDIN_FILENO);
                    close(savedSTDIN);
                }
                result.shouldExit = 0;
                result.statusCode = 1;
                return result;
            }

            if(setupOutputRedirection(newCommand) != 0){
                close(savedSTDOUT);
                if(savedSTDIN != -1){
                    dup2(savedSTDIN, STDIN_FILENO);
                    close(savedSTDIN);
                }
                result.shouldExit = 0;
                result.statusCode = 1;
                return result;
            }
        }
        if(strcmp(newCommand->commandName,"cd")==0){
            result=cdCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"mv")==0){
            result=mvCommand(newCommand);
            //todo: have to make the return type as the executorResult
        }
        else if(strcmp(newCommand->commandName,"ls")==0){
            result=lsCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"exit")==0){
            result=exitCommand();
            
        }
        else if(strcmp(newCommand->commandName,"mkdir")==0){
            result =mkdirCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"clear")==0){
            result=clearCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"touch")==0){
            result=touchCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"rmdir")==0){
            result=rmdirCommand(newCommand);
            
        }
        else if(strcmp(newCommand->commandName,"rm")==0){
            result=rmCommand(newCommand);
        }
        else if (strcmp(newCommand->commandName, "tree") == 0) {
            result = fileTreeCommand(newCommand);
        }
        // else if (strcmp(newCommand->commandName, "dumplist") == 0) {
        //     applyRedirection(newCommand,result);
        //     result = dumpList(newCommand);
        // }
    }
    else{
        result=externalCommand(newCommand);
        
    }
    if(newCommand->inputFile&&savedSTDIN != -1){
        if(dup2(savedSTDIN,STDIN_FILENO) < 0){
            perror("dup2 failed");
            result.shouldExit = 0;
            result.statusCode = 1;
        }
        close(savedSTDIN);
    }
    if(newCommand->outputFile&&savedSTDOUT != -1){
        if(dup2(savedSTDOUT,STDOUT_FILENO) < 0){
            perror("dup2 failed");
            result.shouldExit = 0;
            result.statusCode = 1;
        }
        close(savedSTDOUT);
    }
    return result;

}

//built in helper for pipelines
executorResult executeBuiltIn(Command *cmd){
    executorResult result;
    if(strcmp(cmd->commandName, "cd") == 0){
        result = cdCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "mv") == 0){
        result = mvCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "ls") == 0){
        result = lsCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "exit") == 0){
        result = exitCommand();
    }
    else if(strcmp(cmd->commandName, "mkdir") == 0){
        result = mkdirCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "clear") == 0){
        result = clearCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "touch") == 0){
        result = touchCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "rmdir") == 0){
        result = rmdirCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "rm") == 0){
        result = rmCommand(cmd);
    }
    else if(strcmp(cmd->commandName, "tree") == 0){
        result = fileTreeCommand(cmd);
    }
    else{
        result.shouldExit = 0;
        result.statusCode = -1;
    }

    return result;
}


executorResult executePipes(Pipeline* newPipe){
    //main for pipes ....
    //need to use fork to make all the children of parent process work simultaneosly
    newEntryPipe(newPipe);
    int n=newPipe->numCommands;
    int prevFd=-1;
    for(int i=0;i<n;i++){
        int pipeFd[2];//0->next command reads 1->write from pipe...

        //create a pipe if not the last command
        if(i<n-1){
            if(pipe(pipeFd)==-1){
                perror("Problem creating pipe");
                return (executorResult){0,1};
            }
        }
        //now that pipe is crwated fork the current process

        pid_t pid=fork();

        if(pid==0){
            //child process

            //if not the first take input from first pipe
            if(prevFd!=-1){
                dup2(prevFd,STDIN_FILENO); 
                close(prevFd);
            }
            
            //if not last send putput to the current pipe
            if(i<n-1){
                close(pipeFd[0]);
                dup2(pipeFd[1],STDOUT_FILENO);//send ouput using 1 to write in stdout file
                close(pipeFd[1]);
            }
            //execute command
            Command *cmd=&newPipe->command[i];
            if(setupInputRedirection(cmd) != 0){
                exit(1);
            }
            if(setupOutputRedirection(cmd) != 0){
                exit(1);
            }
            executorResult temp=executeBuiltIn(cmd);
            if(temp.statusCode!=-1){
                exit(temp.statusCode);
            }
            execvp(cmd->commandName,cmd->arguments);
            perror("exec failed");
            exit(1);
        }
        else if(pid>0){
            //parent process
            if(prevFd!=-1){
                close(prevFd);
            }
            if(i<n-1){
                close(pipeFd[1]); //parent doesnt write
                prevFd=pipeFd[0]; //next command reads from here
            }
        }
        else{
            perror("fork failed");
            return (executorResult){0,1};
        }
    }
    int finalStatus=0;
    //wait for all children
    for(int i=0;i<n;i++){
        int status;
        wait(&status);
        if(WIFEXITED(status)){
            int code=WEXITSTATUS(status);
            if(code!=0){
                finalStatus=code;
            }
        }
        else{
            finalStatus = 1;
        }
    }

    return (executorResult){0,finalStatus}; 
}
