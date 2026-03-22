#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "treeCommand.h"


void printDirectoryTree(const char* basePath, int depth) {
    DIR* dir; // directory           
    struct dirent* entry;
    struct stat statbuf;

    if (!(dir = opendir(basePath))) // opendir() opens the path of folder provided in basePath
    {
        return; 
    }

    while ((entry = readdir(dir)) != NULL) // reading directory items until it is NULL 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }      /* Every Linux directory contains a '.' (a reference to itself) and a '..' (a reference to its parent). If you do not explicitly continue (skip) past these,
         the recursive function will continually open '.' forever, resulting in a stack overflow crash. */

        for (int i = 0; i < depth; i++) {
            printf("│   ");
        }
        
        printf("├── %s\n", entry->d_name); // d_name prints the file name 

        char newPath[1024];
    
        snprintf(newPath, sizeof(newPath), "%s/%s", basePath, entry->d_name);
        /* snprintf() merges the strings (like... basePath/entry->d_name) and save it in newPath variable*/

        if (lstat(newPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            printDirectoryTree(newPath, depth + 1);
        }
    }
    closedir(dir);
}

executorResult fileTreeCommand(Command* newCommand) {
    executorResult result;
    result.shouldExit = 0; 

    char* targetDir = "." ; 

    if (newCommand->arguments[1] != NULL) {
        if (newCommand->arguments[2] != NULL) {
            fprintf(stderr, "tree: Too many arguments\n");
            result.statusCode = 1; // command has failed 
            return result;
        }
        targetDir = newCommand->arguments[1];
    }

    struct stat statbuf; // stat is a type of struct which stores metainfos about file

    if (stat(targetDir, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) 
    /*  stat() returns 0 if file/folder's path can be read and copy it from targetDir to statbuf ..
        while S_ISDIR() confirms that the file is an directory  */
    {
        perror("tree"); // perror prints human readable error message 
        result.statusCode = 1; // command has failed 
        return result;
    }

   
    printf("%s\n", targetDir);
    printDirectoryTree(targetDir, 0);

    result.statusCode = 0;
    return result;
}