#include "lsCommand.h"
#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include<stdbool.h>
#include<string.h>
#include"executor.h"

static int isDotEntry(const char *name){
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

executorResult basicLs(const char *path){
    DIR *dir=opendir(path);
    struct dirent*entry;
    if(dir==NULL){
        perror("Naah aint nothin here!!");
        return (executorResult){0, 1};
    }
    while((entry=readdir(dir))!=NULL){
        if(isDotEntry(entry->d_name)){
            continue;
        }
        printf("%s\n",entry->d_name);
    }

    closedir(dir);
    return (executorResult){0, 0};

}
executorResult advancedLs(const char*path){
    DIR *dir=opendir(path);
    struct dirent*entry;
    struct stat st;
    if(dir==NULL){
        perror("Naah homeboy nothin here!!");
        return (executorResult){0, 1};
    }
    
    while((entry=readdir(dir))!=NULL){
        if(isDotEntry(entry->d_name)){
            continue;
        }
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name); //created full path because ....entry->d_name is relative the the current folder ....so need the full path for safe measure s in
        //case want to open file /folder from other resources....
    
        if(lstat(fullpath,&st)==-1){
            perror("nope nothin to see!!");
            continue;
        }

        //to get owner info....for any file,,,,,using pwuid n groupid(grgid)....
        struct passwd *pw = getpwuid(st.st_uid);    
        struct group  *gr = getgrgid(st.st_gid);

        printf("%-8s",pw?pw->pw_name:"?");
        printf("%-8s",gr?gr->gr_name: "?");
        //size of file....
        printf(" %5ld", st.st_size);
        //modified date
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf),"%b %d %H:%M",localtime(&st.st_mtime));

        printf(" %s", timebuf);
        //file name
        printf(" %s\n",entry->d_name);
    }
    closedir(dir);
    return (executorResult){0, 0};

}

executorResult lsCommand(Command *cmd){
    char **args = cmd->arguments;
    char* path = ".";
    if(args[1]==NULL){
        return basicLs(path);
    }
    if(strcmp(args[1],"-l")==0){
        if(args[2]==NULL){
            return advancedLs(path);
        } else {
            return advancedLs(args[2]);
        }
    }
    return basicLs(args[1]);
}
