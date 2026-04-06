#define _XOPEN_SOURCE 700
#include "parser.h"
#include "executor.h"
#include "externalCommand.h"
#include "fileDump.h"
#include "rmCommand.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>

static int buildPath(char *dest, size_t dest_size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(dest, dest_size, fmt, args);
    va_end(args);

    if (written < 0 || (size_t)written >= dest_size) {
        return 0;   // formatting failed or output was truncated
    }
    return 1;
}
static int removeRecursively(const char *path){
    struct stat st;
    if(lstat(path, &st) != 0){
        perror("lstat failed");
        return 0;
    }

    if(S_ISDIR(st.st_mode)){
        DIR *dir = opendir(path);
        if(dir == NULL){
            perror("opendir failed");
            return 0;
        }

        struct dirent *entry;
        int success = 1;

        while((entry = readdir(dir)) != NULL){
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
                continue;
            }

            char childPath[PATH_MAX] = {0};
            if(!buildPath(childPath, sizeof(childPath), "%s/%s", path, entry->d_name)){
                fprintf(stderr, "Path too long during recursive delete\n");
                success = 0;
                break;
            }

            if(!removeRecursively(childPath)){
                success = 0;
                break;
            }
        }

        closedir(dir);

        if(!success){
            return 0;
        }

        if(rmdir(path) != 0){
            perror("rmdir failed");
            return 0;
        }

        return 1;
    }

    if(remove(path) != 0){
        perror("remove failed");
        return 0;
    }

    return 1;
}



int makeDumpFolder(){
    char* home=getenv("HOME");
    if(home==NULL){
        return 0;
    }
    char dumpPath[PATH_MAX]={0};
    char dumpFilesPath[PATH_MAX]={0};
    char dumpInfoPath[PATH_MAX]={0};
    if (!buildPath(dumpPath, sizeof(dumpPath), "%s/.snailDump", home) ||
    !buildPath(dumpFilesPath, sizeof(dumpFilesPath), "%s/files", dumpPath) ||
    !buildPath(dumpInfoPath, sizeof(dumpInfoPath), "%s/info", dumpPath)) {
    fprintf(stderr, "Path too long while creating dump paths\n");
    return 0;
}
//removed / from the end

    int success=1;
    if(mkdir(dumpPath,0755)==-1&& errno!=EEXIST){
        perror("mkdir files");//for debugging
        success=0;
    }
    if(mkdir(dumpInfoPath,0755)==-1&& errno!=EEXIST){
        perror("mkdir files");//for debugging
        success=0;
    }
    if(mkdir(dumpFilesPath,0755)==-1&& errno!=EEXIST){
        perror("mkdir files");//for debugging
        success=0;
    }
    // if (!success) {
    //     fprintf(stderr, "Dump folder setup failed\n");
    // }
    return success;
}

int cleanDump(){
    char* home=getenv("HOME");
    if(home==NULL){
        return 0;
    }
    char dumpPath[PATH_MAX]={0};
    char dumpFilesPath[PATH_MAX]={0};
    char dumpInfoPath[PATH_MAX]={0};

    if (!buildPath(dumpPath, sizeof(dumpPath), "%s/.snailDump", home) ||
    !buildPath(dumpFilesPath, sizeof(dumpFilesPath), "%s/files", dumpPath) ||
    !buildPath(dumpInfoPath, sizeof(dumpInfoPath), "%s/info", dumpPath)) {
    fprintf(stderr, "Path too long while creating dump paths\n");
    return 0;
    }
//removed / from the end

    int success=1;
    if((access(dumpPath,F_OK)!=0)||(access(dumpFilesPath,F_OK)!=0)||(access(dumpInfoPath,F_OK)!=0)){
        int makeDumpSuccess=makeDumpFolder();
        if(!makeDumpSuccess){
            printf("File Dump Creation Error\n");
            success=0;
            return success;
    //this function is called at the start of each time the shell runs and if the dump file could not be created, once exits the execution of the loop furthur
        }
    }
    DIR* dumpFileDir=opendir(dumpFilesPath);
    if(!dumpFileDir){
        perror("opendir files");
        success=0;
        return success;
    }
    DIR* dumpInfoDir=opendir(dumpInfoPath);
    if(!dumpInfoDir){
        perror("opendir info");
        success=0;
        return success;
    }
    struct dirent* entry;
    while((entry=readdir(dumpFileDir))!=NULL){
        char entryName[PATH_MAX + 1]={0};
        if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0){
            continue;
        }


        strncpy(entryName,entry->d_name,PATH_MAX);
        entryName[PATH_MAX-1]='\0';
        //the path of the entry in the info folder of the dump will be stored here. This path will be required if we wish to delete this file from the info folder during cleanup.
        char entryInfoPath[PATH_MAX + 1]={0};
        //the path of the entry in the files folder of the dump will be stored here. This path will be required if we wish to delete this file from the files folder during cleanup.
        char entryFilePath[PATH_MAX + 1]={0};
        // snprintf(entryInfoPath,sizeof(entryInfoPath), "%s/%s.snailInfo", dumpInfoPath, entryName);
        // snprintf(entryFilePath,sizeof(entryFilePath),"%s/%s", dumpFilesPath, entryName);
        if(!buildPath(entryInfoPath,sizeof(entryInfoPath), "%s/%s.snailInfo", dumpInfoPath, entryName)||
            !buildPath(entryFilePath,sizeof(entryFilePath),"%s/%s", dumpFilesPath, entryName)
        ){
            fprintf(stderr, "Dump entryInfoPath/ entryPath too long\n");
            return 0;
        }
        
        //will read the trash info of the current entry
        FILE* entryInfoFilePointer;
        entryInfoFilePointer=fopen(entryInfoPath,"r");//we open the info file of the entry in read mode to see its deletion time
        if (entryInfoFilePointer==NULL) {
            // printf("Error: Could not open file.\n");
            continue;
        }
//can include multiple success code so that if some files could that should have been deldetd ans their deletion failed, the user can know
        

        int maxLineLength=256;
        char buffer[maxLineLength];
        int dFound=0;
        char deletionTimeString[25]={0};
        while(fgets(buffer,maxLineLength,entryInfoFilePointer)!=NULL){
            if(dFound){
                int newLinePos=strcspn(buffer,"\n");
                buffer[newLinePos]='\0';
                strcpy(deletionTimeString,buffer);
            }
            else{
                if(strncmp(buffer,"DeletionDate:",13)==0){
                    dFound=1;
                }
            }
        }
        fclose(entryInfoFilePointer);
        entryInfoFilePointer = NULL;
        if(!dFound){
            continue;
        }
        char* format = "%Y-%m-%dT%H:%M:%S";
        struct tm timeStorage={0};
        if(strptime(deletionTimeString,format,&timeStorage)==NULL){
            continue;
        }
        time_t deletionTime=mktime(&timeStorage);
        if(deletionTime==(time_t)-1){
            continue;
        }
        time_t currentTime;
        time(&currentTime);  
                if(currentTime - deletionTime > (48 * 60 * 60)){
            struct stat entryStat;
            if(lstat(entryFilePath, &entryStat) != 0){
                perror("lstat failed");
                continue;
            }

            if(S_ISDIR(entryStat.st_mode)){
                if(!removeRecursively(entryFilePath)){
                    fprintf(stderr, "Failed to remove dumped directory: %s\n", entryFilePath);
                    continue;
                }
            }
            else{
                if(remove(entryFilePath) != 0){
                    perror("remove failed");
                    continue;
                }
            }

            if(remove(entryInfoPath) != 0){
                perror("remove failed");
            }
        }
       
    }
    closedir(dumpFileDir);
    closedir(dumpInfoDir);

    return success;
}

//if the parent folder no longer exists, can tell the user to manually cd into the dump and then mv it to the desired folder
int sendToDump(char* entryPath){
    int success=1;
    char* home=getenv("HOME");
    if(home==NULL){
        success=0;
        return success;
    }
    char dumpPath[PATH_MAX]={0};
    char dumpFilesPath[PATH_MAX]={0};
    char dumpInfoPath[PATH_MAX]={0};

    // sprintf(dumpPath,"%s/.snailDump/",home);
    // sprintf(dumpFilesPath,"%s/files/",dumpPath);
    // sprintf(dumpInfoPath,"%s/info/",dumpPath);
    if (!buildPath(dumpPath, sizeof(dumpPath), "%s/.snailDump", home) ||
    !buildPath(dumpFilesPath, sizeof(dumpFilesPath), "%s/files", dumpPath) ||
    !buildPath(dumpInfoPath, sizeof(dumpInfoPath), "%s/info", dumpPath)) {
    fprintf(stderr, "Path too long while creating dump paths\n");
    return 0;
    }//removed / from the end


    //if the dump does not exist, we create it
    if((access(dumpPath,F_OK)!=0)||(access(dumpFilesPath,F_OK)!=0)||(access(dumpInfoPath,F_OK)!=0)){
        success=makeDumpFolder();
        if(!success){
            return success;
        }
    }

    //entryPath is the current path at which the entry is present
    // int maxNameLength=PATH_MAX;
    char entryName[PATH_MAX]={0};
    int ch='/';
    char normalizedEntryPath[PATH_MAX] = {0};
    if(strlen(entryPath) >= sizeof(normalizedEntryPath)){
        fprintf(stderr, "entry path too long\n");
        return 0;
    }

    strncpy(normalizedEntryPath, entryPath, sizeof(normalizedEntryPath) - 1);
    normalizedEntryPath[sizeof(normalizedEntryPath) - 1] = '\0';

    size_t len = strlen(normalizedEntryPath);
    while(len > 1 && normalizedEntryPath[len - 1] == '/'){
        normalizedEntryPath[len - 1] = '\0';
        len--;
    }

        struct stat entryStat;
    if(lstat(normalizedEntryPath, &entryStat) != 0){
        perror("lstat failed");
        return 0;
    }

    const char *entryType = NULL;
    if(S_ISREG(entryStat.st_mode)){
        entryType = "file";
    }
    else if(S_ISDIR(entryStat.st_mode)){
        entryType = "directory";
    }
    else{
        fprintf(stderr, "Unsupported entry type for dump: %s\n", normalizedEntryPath);
        return 0;
    }
    

    char* lastPosOfSlash=strrchr(normalizedEntryPath,ch);
    if(lastPosOfSlash){
        lastPosOfSlash++;
    }
    else{
        lastPosOfSlash=normalizedEntryPath;
    }//will have to look into this. if the user tries to delete "folder" and "folder/", the behaviour might be different
    strncpy(entryName,lastPosOfSlash,sizeof(entryName)-1);
    entryName[sizeof(entryName)-1]='\0';

    //finding deletion time and converting it to the formatted time string
    time_t deletionTime;//the current time is the deletion time
    time(&deletionTime);
    struct tm* timeStorage;
    char deletionTimeString[25]={0};
    timeStorage=localtime(&deletionTime);
    if(!timeStorage){
        success=0;
        return success;
    }
    char* format = "%Y-%m-%dT%H:%M:%S";
    if((strftime(deletionTimeString,25,format,timeStorage)==0)){
        success=0;
        return success;
    }



    //entryFilePath is the location the entry will go to in the files/ folder of the dump
    //can later replace rename if it disturbs cross filesystems too much
    char entryFilePath[PATH_MAX]={0};
    // snprintf(entryFilePath,sizeof(entryFilePath),"%s/%s_%ld",dumpFilesPath,entryName,deletionTime);
    if(!buildPath(entryFilePath,sizeof(entryFilePath),"%s/%s_%ld",dumpFilesPath,entryName,deletionTime)){
        fprintf(stderr, "entryFilePath too long\n");
        return 0;
    }
    if((rename(normalizedEntryPath,entryFilePath))!=0){
        if (errno == EXDEV) {
            // cross-device → fallback copy
            printf("Cross-device move not supported yet\n");
        }
        perror("rename failed");
        success=0;
        return success;
    }

    //entryInfoPath is the location the entry will go to in the info/ folder of the dump
    char entryInfoPath[PATH_MAX]={0};
    // snprintf(entryInfoPath,sizeof(entryInfoPath),"%s/%s_%ld.snailInfo",dumpInfoPath,entryName,deletionTime);
    if(!buildPath(entryInfoPath,sizeof(entryInfoPath),"%s/%s_%ld.snailInfo",dumpInfoPath,entryName,deletionTime)){
        fprintf(stderr, "entryInfoFilePath too long\n");
        return 0;
    }
    //will write the trash info of the file into the entryInfoFile
    FILE* entryInfoFilePointer;
    entryInfoFilePointer=fopen(entryInfoPath,"w");
    if(!entryInfoFilePointer){
        printf("dumping of file %s failed",normalizedEntryPath);
        //if metadata writing fails restore the file.
        if(rename(entryFilePath, normalizedEntryPath) != 0){
            if (errno == EXDEV) {
                // cross-device → fallback copy
                printf("Cross-device move not supported yet\n");
            }
            perror("rollback failed");
        }
        success=0;
        return success;
    }
    char trashInfo[3*PATH_MAX]={0};
    snprintf(trashInfo, sizeof(trashInfo),
         "[Trash Info]\nPath:\n%s\nDeletionDate:\n%s\nType:\n%s\n",
         normalizedEntryPath, deletionTimeString, entryType);
    // if(!buildPath(trashInfo,sizeof(trashInfo),"[Trash Info]\nPath:\n%s\nDeletionDate:\n%s\n",entryPath,deletionTimeString)){
    //     fprintf(stderr, "entryInfoFilePath too long\n");
    //     return 0;
    // }
    fputs(trashInfo,entryInfoFilePointer);
    fclose(entryInfoFilePointer);
    return success;
}


int fileRestoreHelper(char* entryPath){
    if(entryPath == NULL){
        return 0;
    }

    char* home = getenv("HOME");
    if(home == NULL){
        return 0;
    }

    char dumpPath[PATH_MAX] = {0};
    char dumpFilesPath[PATH_MAX] = {0};
    char dumpInfoPath[PATH_MAX] = {0};

    if (!buildPath(dumpPath, sizeof(dumpPath), "%s/.snailDump", home) ||
        !buildPath(dumpFilesPath, sizeof(dumpFilesPath), "%s/files", dumpPath) ||
        !buildPath(dumpInfoPath, sizeof(dumpInfoPath), "%s/info", dumpPath)) {
        fprintf(stderr, "Path too long while creating dump paths\n");
        return 0;
    }

    if (access(dumpPath, F_OK) != 0 ||
        access(dumpFilesPath, F_OK) != 0 ||
        access(dumpInfoPath, F_OK) != 0) {
        return 0;
    }

    DIR* dumpFilesDir = opendir(dumpFilesPath);
    if(dumpFilesDir == NULL){
        perror("opendir files");
        return 0;
    }

    struct dirent* entry;
    int foundMatch = 0;
    time_t latestDeletionTime = (time_t)-1;
    char bestEntryFilePath[PATH_MAX] = {0};
    char bestEntryInfoPath[PATH_MAX] = {0};

    if(access(entryPath, F_OK) == 0){
        fprintf(stderr, "Restore destination already exists: %s\n", entryPath);
        return 0;
    }


    char parentPath[PATH_MAX] = {0};
    strncpy(parentPath, entryPath, sizeof(parentPath) - 1);
    parentPath[sizeof(parentPath) - 1] = '\0';

    char *lastSlash = strrchr(parentPath, '/');
    if(lastSlash != NULL && lastSlash != parentPath){
        *lastSlash = '\0';
        if(access(parentPath, F_OK) != 0){
            fprintf(stderr, "Parent directory does not exist: %s\n", parentPath);
            return 0;
        }
    }


    while((entry = readdir(dumpFilesDir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char entryInfoPath[PATH_MAX] = {0};
        char entryFilePath[PATH_MAX] = {0};

        if (!buildPath(entryInfoPath, sizeof(entryInfoPath), "%s/%s.snailInfo", dumpInfoPath, entry->d_name) ||
            !buildPath(entryFilePath, sizeof(entryFilePath), "%s/%s", dumpFilesPath, entry->d_name)) {
            fprintf(stderr, "Dump entry path too long\n");
            continue;
        }

        FILE* entryInfoFilePointer = fopen(entryInfoPath, "r");
        if(entryInfoFilePointer == NULL){
            continue;
        }

        char buffer[256];
        char originalPath[PATH_MAX] = {0};
        char deletionTimeString[25] = {0};
        int pathFound = 0;
        int deletionFound = 0;

        while(fgets(buffer, sizeof(buffer), entryInfoFilePointer) != NULL){
            if(pathFound){
                buffer[strcspn(buffer, "\n")] = '\0';
                strncpy(originalPath, buffer, sizeof(originalPath) - 1);
                originalPath[sizeof(originalPath) - 1] = '\0';
                pathFound = 0;
                continue;
            }

            if(deletionFound){
                buffer[strcspn(buffer, "\n")] = '\0';
                strncpy(deletionTimeString, buffer, sizeof(deletionTimeString) - 1);
                deletionTimeString[sizeof(deletionTimeString) - 1] = '\0';
                deletionFound = 0;
                continue;
            }

            if(strncmp(buffer, "Path:", 5) == 0){
                pathFound = 1;
            }
            else if(strncmp(buffer, "DeletionDate:", 13) == 0){
                deletionFound = 1;
            }
        }

        fclose(entryInfoFilePointer);

        if(originalPath[0] == '\0' || deletionTimeString[0] == '\0'){
            continue;
        }

        if(strcmp(originalPath, entryPath) != 0){
            continue;
        }

        struct tm timeStorage = {0};
        if(strptime(deletionTimeString, "%Y-%m-%dT%H:%M:%S", &timeStorage) == NULL){
            continue;
        }

        time_t deletionTime = mktime(&timeStorage);
        if(deletionTime == (time_t)-1){
            continue;
        }

        if(!foundMatch || deletionTime > latestDeletionTime){
            latestDeletionTime = deletionTime;
            foundMatch = 1;

            strncpy(bestEntryFilePath, entryFilePath, sizeof(bestEntryFilePath) - 1);
            bestEntryFilePath[sizeof(bestEntryFilePath) - 1] = '\0';

            strncpy(bestEntryInfoPath, entryInfoPath, sizeof(bestEntryInfoPath) - 1);
            bestEntryInfoPath[sizeof(bestEntryInfoPath) - 1] = '\0';
        }
    }

    closedir(dumpFilesDir);

    if(!foundMatch){
        return 0;
    }

    if(rename(bestEntryFilePath, entryPath) != 0){
        if(errno == EXDEV){
            printf("Cross-device move not supported yet\n");
        }
        perror("rename failed");
        return 0;
    }

    if(remove(bestEntryInfoPath) != 0){
        perror("remove failed");
        return 0;
    }

    return 1;
}


int fileRestore(Command* newCmd){
    if(newCmd == NULL || newCmd->argCount < 2 || newCmd->arguments[1] == NULL){
        printf("restore: missing path\n");
        return 0;
    }

    char targetPath[PATH_MAX] = {0};
    if(strlen(newCmd->arguments[1]) >= sizeof(targetPath)){
        fprintf(stderr, "restore path too long\n");
        return 0;
    }

    strncpy(targetPath, newCmd->arguments[1], sizeof(targetPath) - 1);
    targetPath[sizeof(targetPath) - 1] = '\0';

    size_t len = strlen(targetPath);
    while(len > 1 && targetPath[len - 1] == '/'){
        targetPath[len - 1] = '\0';
        len--;
    }

    int success = fileRestoreHelper(targetPath);
    if(!success){
        printf("Could not restore: %s\n", targetPath);
    }

    return success;
}


//can include data compression before sending to the dump if the user wishes for it to be that way
