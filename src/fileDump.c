#include "parser.h"
#include "executor.h"
#include "externalCommand.h"
#include "fileDump.h"
#include "rmCommand.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include<dirent.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
int makeDumpFolder(){
    char* home=getenv("HOME");
    char dumpPath[PATH_MAX];
    sprintf("%s/.snailDump/",home);
    char dumpFilesPath[PATH_MAX];
    sprintf("%s/files/",dumpPath);
    char dumpInfoPath[PATH_MAX];
    sprintf("%s/info/",dumpPath);
    int success=1;
    if(mkdir(dumpPath,0755)==-1&& errno!=EEXIST){
        success=0;
    }
    if(mkdir(dumpInfoPath,0755)==-1&& errno!=EEXIST){
        success=0;
    }
    if(mkdir(dumpFilesPath,0755)==-1&& errno!=EEXIST){
        success=0;
    }
    return success;
}

int clearDump(){
    char* home=getenv("HOME");
    char dumpPath[PATH_MAX];
    char dumpFilesPath[PATH_MAX];
    char dumpInfoPath[PATH_MAX];

    sprintf("%s/.snailDump/",home);
    sprintf("%s/files/",dumpPath);
    sprintf("%s/info/",dumpPath);

    int startLoop=1;
    if((access(dumpPath,F_OK)!=0)||(access(dumpFilesPath,F_OK)!=0)||(access(dumpInfoPath,F_OK)!=0)){
        int success=makeDumpFolder();
        if(!success){
            printf("File Dump Creation Error\n");
            startLoop=0;
            return startLoop;
//this function is called at the start of each time the shell runs and if the dump file could not be created, once exits the execution of the loop furthur
        }
    }
    DIR* dumpDir=opendir(dumpPath);
    DIR* dumpFileDir=opendir(dumpFilesPath);
    DIR* dumpInfoDir=opendir(dumpInfoPath);
    struct dirent* entry;
    int dumpCleanSuccess=1;
    while((entry=readdir(dumpFileDir))!=NULL){
        char entryName[PATH_MAX + 1];
        if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0){
            continue;
        }
        strcpy(entryName,entry->d_name);
        char entryInfoPath[PATH_MAX + 1];//the path of the entry in the info folder of the dump will be stored here. This path will be required if we wish to delete this file from the info folder during cleanup.
        char entryFilePath[PATH_MAX + 1];//the path of the entry in the files folder of the dump will be stored here. This path will be required if we wish to delete this file from the files folder during cleanup.
        sprintf(entryInfoPath, "%s/%s.snailInfo", dumpInfoPath, entryName);
        sprintf(entryFilePath,"%s/%s", dumpFilesPath, entryName);
        FILE* entryInfoFilePointer;
        entryInfoFilePointer=fopen(entryInfoPath,"r");//we open the info file of the entry in read mode to see its deletion time
        if (entryInfoFilePointer==NULL) {
        // printf("Error: Could not open file.\n");
        continue;
        }
        int maxLineLength=256;
        char buffer[maxLineLength];
        int dFound=0;
        char deletionTimeString[25];
        while(fgets(buffer,maxLineLength,entryInfoFilePointer)!=NULL){
            if(dFound){
                int newLinePos=strcspn(buffer,"\n");
                buffer[newLinePos]='\0';
                strcpy(deletionTimeString,buffer);
            }
            else{
                if(strcnmp(buffer,"DeletionDate",13)==0){
                    dFound=1;
                }
            }
        }
        fclose(entryInfoFilePointer);
        char* format = "%Y-%m-%dT%H:%M:%S";
        struct tm timeStorage={0};
        if(strptime(deletionTimeString,format,&timeStorage)==NULL){
            dumpCleanSuccess=0;
        }
        time_t deletionTime=mktime(&timeStorage);
        if(deletionTime==(time_t)-1){
            dumpCleanSuccess=0;
        }
        time_t currentTime;
        time(&currentTime);  
        if(currentTime-deletionTime>(48*60*60)){
            remove(entryInfoPath);
            remove(entryFilePath);
        }       
    }
    closedir(dumpDir);
    closedir(dumpFileDir);
    closedir(dumpInfoDir);

    return startLoop;
}


int sendToDump(char* entryPath){
    char* home=getenv("HOME");
    char dumpPath[PATH_MAX];
    char dumpFilesPath[PATH_MAX];
    char dumpInfoPath[PATH_MAX];

    sprintf("%s/.snailDump/",home);
    sprintf("%s/files/",dumpPath);
    sprintf("%s/info/",dumpPath);
    int maxNameLength=256;
    char entryName[maxNameLength];
    int ch='/';
    char* lastPosOfSlash=strchr(entryPath,ch);
    lastPosOfSlash++;
    strcpy(entryName,lastPosOfSlash);
    
}