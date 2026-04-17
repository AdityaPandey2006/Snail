#include <stdio.h>
#include <unistd.h>
#include "rmdirCommand.h"
#include "fileDump.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

static int isDirectoryPath(const char *path){
    struct stat st;
    if(path == NULL){
        return 0;
    }
    if(lstat(path, &st) != 0){
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int isEmptyDirectory(const char *path){
    DIR *dir = opendir(path);
    if(dir == NULL){
        return -1;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        closedir(dir);
        return 0;
    }

    closedir(dir);
    return 1;
}

executorResult rmdirCommand(Command* newCommand) {

    executorResult result;
    result.shouldExit = 0;
    result.statusCode = 0;

    if (newCommand->argCount < 2) {
        fprintf(stderr, "rmdir: missing operand\n");
        result.statusCode = 1;
        return result;
    }

    for (int i = 1; i < newCommand->argCount; i++) {

        char *dir = newCommand->arguments[i];

        if(!isDirectoryPath(dir)){
            fprintf(stderr, "rmdir: '%s' is not a directory\n", dir);
            result.statusCode = 1;
            continue;
        }

        int isEmpty = isEmptyDirectory(dir);
        if(isEmpty < 0){
            perror("rmdir");
            result.statusCode = 1;
            continue;
        }
        if(isEmpty == 0){
            fprintf(stderr, "rmdir: failed to remove '%s': Directory not empty\n", dir);
            result.statusCode = 1;
            continue;
        }

        if(sendToDump(dir) != 1){
            fprintf(stderr, "rmdir: failed to send '%s' to dump\n", dir);
            result.statusCode = 1;
            continue;
        }
    }

    return result;
}
