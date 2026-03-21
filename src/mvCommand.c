#include<stdio.h>
#include<stdlib.h>
#include"executor.h"
#include<unistd.h>
#include<dirent.h>
#include"mvCommand.h"
#include<stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include<errno.h>
#include<libgen.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define STAT_FUNC stat
#define STAT_STRUCT struct stat


/*so here for our personal shell we wont use the standard workig of mv as it can cause users to unknowingly oerform operation that they dont want
mv performs ----overwriting ,renaming,moving file ,....

mv -over filePath filePath .......for overwirting
mv -re filePath newName......for renaming...
mv -shift filePath DirPath.......

this type of syntax make easier to work for users...we can add additional in future.....

*/

//helpers....
bool isdirPath(const char* path) {
    if (path == NULL) {
        return false;
    }
    STAT_STRUCT info;
    if (STAT_FUNC(path, &info)==0) {
        return S_ISDIR(info.st_mode);
    }
    return false; // Path does not exist or error occurred
}

bool isfilePath(const char* path) {
    if (path == NULL) {
        return false;
    }
    STAT_STRUCT info;
    if (STAT_FUNC(path, &info)==0) {
        return S_ISREG(info.st_mode);
    }
    return false; // Path does not exist or error occurred
}

int copyFile(const char *srcPath, const char *destPath) {

    int src = open(srcPath, O_RDONLY);
    if (src < 0) return -1;

    struct stat st;
    fstat(src, &st);

    int dest = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dest < 0) {
        close(src);
        return -1;
    }

    off_t offset = 0;
    ssize_t bytes = sendfile(dest, src, &offset, st.st_size);
    if (bytes < 0) {
        close(src);
        close(dest);
        return -1;
    }

    close(src);
    close(dest);

    return 0;
}

//for now only two arguments will be worked upon....
executorResult overwrite(char **args){

    char *sourcePath = args[0];
    char *destPath   = args[1];
    if(!isfilePath(sourcePath)){
        printf("Source '%s' is not a valid file\n",sourcePath);
        return (executorResult){0,1};
    }
    if(!isfilePath(destPath)){
        printf("Dest'%s' is not a valid file\n",destPath);
        return (executorResult){0,1};
    }
    if(rename(sourcePath,destPath)==0){
        printf("Successfully overwritten '%s' with '%s'\n",destPath,sourcePath);
        return (executorResult){0,0};
    }
    if(errno==EXDEV){
        if(copyFile(sourcePath,destPath)!=0){
            printf("Failed to copy '%s' to '%s' :%s\n",sourcePath,destPath,strerror(errno));
            return (executorResult){0,1};
        }
        if(unlink(sourcePath)!=0){
            printf("file copied butr coudn't remove source file");
            return (executorResult){0,1};
        }
        printf("Successfully overwritten '%s' with '%s' across filesystems\n", destPath, sourcePath);
        return (executorResult){0, 0};
    }

    //any other rename error....in overwritten
    printf("overwrite failed: %s\n",strerror(errno));
    return (executorResult){0,1};
}
executorResult changeDir(char **args){
    
    char *sourcePath = args[0];
    char *destPath   = args[1];
    if(!isfilePath(sourcePath)){
        printf("Source '%s' is not a valid file\n",sourcePath);
        return (executorResult){0,1};
    }
    if(!isdirPath(destPath)){
        printf("Dest'%s' is not a valid directory\n",destPath);
        return (executorResult){0,1};
    }

    char fileCopy[1024];
    strncpy(fileCopy, sourcePath, sizeof(fileCopy));
    char *fileName = basename(fileCopy);  // get actual file name...

    // Build the new full path: dir_path + "/" + filename fir directory....
    char newPath[1024];
    snprintf(newPath, sizeof(newPath), "%s/%s", destPath, fileName);

    if(rename(sourcePath,newPath)==0){
        printf("Successfully moved '%s' to new directory...\n",fileName);
        return (executorResult){0,0};
    }
    else{
        if(errno==EXDEV){
            printf("Cross file system activity detected!\n");
            if(copyFile(sourcePath,newPath)==0){
                if(remove(sourcePath)==0){
                    printf("moved %s to %s (across file systems)\n",fileName,destPath);
                    return (executorResult){0,0};
                }
                else{
                    printf("could not remove the original file..for moving....");
                    return (executorResult){0,1};
                }
            }
            else{
                perror("error in moving file across file systems\n");
                return (executorResult){0,1};
            }
        }
        if(errno==EACCES){
            printf("permission required to move");
            return (executorResult){0,1};
        }
    }
    return (executorResult){0,1};
}
executorResult renameFile(char **args){
    char*oldFile=args[0];
    char*newFile=args[1];
    if(!isfilePath(oldFile)){
        printf("%s is not a valid file path",oldFile);
        return (executorResult){0,1};
    }
    if(rename(oldFile,newFile)==0){
        printf("renamed....'%s' as '%s' \n",oldFile,newFile);
        return (executorResult){0,0};
    }
    else{
        perror("error renaming file...");
        return (executorResult){0,1};
    }
}



executorResult mvCommand(Command *cmd){
    //recognise what type of funtion wish to perform ....
    char **args=cmd->arguments;

    bool re=false;
    bool over=false;
    bool shift=false;

    if(args[1]==NULL){
        printf("please give operational command [re,over,shift]\n");
        return (executorResult){0,1};
    }
    if(strcmp(args[1],"-re")==0){
        re=true;
    }
    else if(strcmp(args[1],"-over")==0){
        over=true;
    }
    else if(strcmp(args[1],"-shift")==0){
        shift=true;
    }
    else{
        printf("Invalid mv option\n");
        return (executorResult){0,1};
    }

    if(args[2]==NULL || args[3]==NULL || args[4]!=NULL){
        printf("invalid amount of arguments pls try again");
        return (executorResult){0,1};
    }

    if(over){
        return overwrite(&args[2]);
    }
    else if(shift){
        return changeDir(&args[2]);
    }
    else if(re){
        return renameFile(&args[2]);
    }
    return (executorResult){0,1};
}