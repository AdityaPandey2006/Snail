#include "ls.h"
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


int basicLs(const char *path){
    DIR *dir=opendir(path);
    struct dirent*entry;
    if(dir==NULL){
        perror("Naah aint nothin here!!");
        return 1;
    }
    while((entry=readdir(dir))!=NULL){
        printf("%s\n",entry->d_name);
    }

    closedir(dir);
    return 1;

}
int advancedLs(const char*path){
    DIR *dir=opendir(path);
    struct dirent*entry;
    struct stat st;
    if(dir==NULL){
        perror("Naah homeboy nothin here!!");
        return 1;
    }
    while((entry=readdir(dir))!=NULL){
        if(lstat(entry->d_name,&st)==-1){
            perror("nope nothin to see!!");
            continue;
        }

        //to get owner info....for any file,,,,,
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        printf(" %s", pw ? pw->pw_name : "?");
        printf(" %s", gr ? gr->gr_name : "?");
        //size of file....
        printf(" %5ld", st.st_size);
        //modified date
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf),
                 "%b %d %H:%M",
                 localtime(&st.st_mtime));

        printf(" %s", timebuf);
        //file name
        printf("%s\n",entry->d_name);
    }
    closedir(dir);
    return 1;

}

int commandLs(char ** args){     //acess the funtion defined in ls.h.....
    //check if ls is with a -l or not

    char*path=".";
    if(args[1]==NULL){ //only ...ls
        return basicLs(path);
    }
    if(args[1]!=NULL && strcmp(args[1],"-l")==0 && args[2]==NULL){//only ls-l
        return advancedLs(path);
    }
    if(args[1]!=NULL && strcmp(args[1],"-l")!=0){ //ls some folder
        path=args[1];
        return basicLs(path);
    }
    if(args[1]!=NULL && strcmp(args[1],"-l")==0 && args[2]!=NULL){//only ls-l
        path=args[2];
        return advancedLs(path);
    }
}

