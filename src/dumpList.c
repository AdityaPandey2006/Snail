#include "dumpList.h"
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
static int buildPath(char *dest, size_t destSize, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(dest, destSize, fmt, args);
    va_end(args);

    return written >= 0 && (size_t)written < destSize;
}

executorResult dumpList(Command* newCommand){
    executorResult result = {0, 0};

    if(newCommand->argCount > 1){
        fprintf(stderr, "dumpList: too many arguments\n");
        result.statusCode = 1;
        return result;
    }

    const char *home = getenv("HOME");
    if(home == NULL){
        fprintf(stderr, "dumpList: HOME is not set\n");
        result.statusCode = 1;
        return result;
    }

    char dumpPath[PATH_MAX] = {0};
    char dumpInfoPath[PATH_MAX] = {0};
    if(!buildPath(dumpPath, sizeof(dumpPath), "%s/.snailDump", home) ||
       !buildPath(dumpInfoPath, sizeof(dumpInfoPath), "%s/info", dumpPath)){
        fprintf(stderr, "dumpList: dump path too long\n");
        result.statusCode = 1;
        return result;
    }

    if(access(dumpPath, F_OK) != 0 || access(dumpInfoPath, F_OK) != 0){
        printf("The dump is empty or has not been created yet.\n");
        return result;
    }

    DIR *dumpInfoDir = opendir(dumpInfoPath);
    if(dumpInfoDir == NULL){
        perror("dumpList");
        result.statusCode = 1;
        return result;
    }

    printf("%-50s | %-19s | %s\n", "ORIGINAL PATH", "DELETION DATE", "TYPE");
    printf("---------------------------------------------------+---------------------+----------\n");

    struct dirent *entry;
    int count = 0;
    while((entry = readdir(dumpInfoDir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char infoFilePath[PATH_MAX] = {0};
        if(!buildPath(infoFilePath, sizeof(infoFilePath), "%s/%s", dumpInfoPath, entry->d_name)){
            continue;
        }

        FILE *fp = fopen(infoFilePath, "r");
        if(fp == NULL){
            continue;
        }

        char buffer[512];
        char originalPath[PATH_MAX] = {0};
        char deletionDate[25] = {0};
        char entryType[32] = {0};
        int expectPath = 0;
        int expectDeletionDate = 0;
        int expectType = 0;

        while(fgets(buffer, sizeof(buffer), fp) != NULL){
            buffer[strcspn(buffer, "\n")] = '\0';

            if(expectPath){
                strncpy(originalPath, buffer, sizeof(originalPath) - 1);
                expectPath = 0;
                continue;
            }
            if(expectDeletionDate){
                strncpy(deletionDate, buffer, sizeof(deletionDate) - 1);
                expectDeletionDate = 0;
                continue;
            }
            if(expectType){
                strncpy(entryType, buffer, sizeof(entryType) - 1);
                expectType = 0;
                continue;
            }

            if(strcmp(buffer, "Path:") == 0){
                expectPath = 1;
            }
            else if(strcmp(buffer, "DeletionDate:") == 0){
                expectDeletionDate = 1;
            }
            else if(strcmp(buffer, "Type:") == 0){
                expectType = 1;
            }
        }

        fclose(fp);

        if(originalPath[0] == '\0'){
            continue;
        }

        if(deletionDate[0] == '\0'){
            strncpy(deletionDate, "unknown", sizeof(deletionDate) - 1);
        }
        if(entryType[0] == '\0'){
            strncpy(entryType, "unknown", sizeof(entryType) - 1);
        }

        printf("%-50.50s | %-19s | %s\n", originalPath, deletionDate, entryType);
        count++;
    }

    closedir(dumpInfoDir);

    if(count == 0){
        printf("The dump is currently empty.\n");
    }
    else{
        printf("---------------------------------------------------+---------------------+----------\n");
        printf("Total items in dump: %d\n", count);
    }

    return result;
}
