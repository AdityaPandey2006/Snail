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
#include <ctype.h>
#include <limits.h>
#include <sys/ioctl.h>
#include"executor.h"
#include "config.h"

static int isDotEntry(const char *name){
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static int hexDigitToInt(char ch){
    if(ch >= '0' && ch <= '9'){
        return ch - '0';
    }
    ch = (char)tolower((unsigned char)ch);
    if(ch >= 'a' && ch <= 'f'){
        return 10 + (ch - 'a');
    }
    return -1;
}

static int parseHexColor(const char *hex, int *red, int *green, int *blue){
    if(hex == NULL || strlen(hex) != 7 || hex[0] != '#'){
        return 0;
    }

    int values[6];
    for(int i = 0; i < 6; i++){
        values[i] = hexDigitToInt(hex[i + 1]);
        if(values[i] < 0){
            return 0;
        }
    }

    *red = values[0] * 16 + values[1];
    *green = values[2] * 16 + values[3];
    *blue = values[4] * 16 + values[5];
    return 1;
}

static void printColorHex(const char *hex){
    int red;
    int green;
    int blue;
    if(parseHexColor(hex, &red, &green, &blue)){
        printf("\033[38;2;%d;%d;%dm", red, green, blue);
    }
}

static void printEntryNameColored(const char *name, mode_t mode){
    const SnailConfig *config = getSnailConfig();
    const char *colorHex = config->ls_file_color;

    if(S_ISDIR(mode)){
        colorHex = config->ls_directory_color;
    }

    int red;
    int green;
    int blue;
    if(parseHexColor(colorHex, &red, &green, &blue)){
        printColorHex(colorHex);
        printf("%s", name);
        printf("\033[0m");
        return;
    }

    printf("%s", name);
}

typedef struct{
    char *name;
    mode_t mode;
}LsEntry;

static int compareLsEntriesByName(const void *a, const void *b){
    const LsEntry *left = (const LsEntry*)a;
    const LsEntry *right = (const LsEntry*)b;
    return strcmp(left->name, right->name);
}

static int getTerminalColumns(void){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0){
        return ws.ws_col;
    }
    return 80;
}

executorResult basicLs(const char *path){
    DIR *dir=opendir(path);
    struct dirent*entry;
    if(dir==NULL){
        perror("Naah aint nothin here!!");
        return (executorResult){0, 1};
    }

    int capacity = 32;
    int count = 0;
    int maxNameLen = 0;
    LsEntry *entries = malloc(sizeof(LsEntry) * capacity);
    if(entries == NULL){
        closedir(dir);
        perror("malloc");
        return (executorResult){0, 1};
    }

    while((entry=readdir(dir))!=NULL){
        if(isDotEntry(entry->d_name)){
            continue;
        }

        char fullpath[PATH_MAX];
        struct stat st;
        int fullPathWritten = snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        if(fullPathWritten < 0 || (size_t)fullPathWritten >= sizeof(fullpath) || lstat(fullpath, &st) != 0){
            continue;
        }

        if(count == capacity){
            capacity *= 2;
            LsEntry *resized = realloc(entries, sizeof(LsEntry) * capacity);
            if(resized == NULL){
                for(int i = 0; i < count; i++){
                    free(entries[i].name);
                }
                free(entries);
                closedir(dir);
                perror("realloc");
                return (executorResult){0, 1};
            }
            entries = resized;
        }

        entries[count].name = strdup(entry->d_name);
        if(entries[count].name == NULL){
            for(int i = 0; i < count; i++){
                free(entries[i].name);
            }
            free(entries);
            closedir(dir);
            perror("strdup");
            return (executorResult){0, 1};
        }
        entries[count].mode = st.st_mode;
        int nameLen = (int)strlen(entries[count].name);
        if(nameLen > maxNameLen){
            maxNameLen = nameLen;
        }
        count++;
    }

    closedir(dir);

    if(count == 0){
        free(entries);
        return (executorResult){0, 0};
    }

    qsort(entries, count, sizeof(LsEntry), compareLsEntriesByName);

    int terminalCols = getTerminalColumns();
    int cellWidth = maxNameLen + 2;
    if(cellWidth < 4){
        cellWidth = 4;
    }
    int cols = terminalCols / cellWidth;
    if(cols < 1){
        cols = 1;
    }
    int rows = (count + cols - 1) / cols;

    for(int row = 0; row < rows; row++){
        for(int col = 0; col < cols; col++){
            int index = col * rows + row; // column-major like common ls output
            if(index >= count){
                continue;
            }

            printEntryNameColored(entries[index].name, entries[index].mode);

            if(col < cols - 1){
                int padding = cellWidth - (int)strlen(entries[index].name);
                if(padding < 1){
                    padding = 1;
                }
                for(int p = 0; p < padding; p++){
                    printf(" ");
                }
            }
        }
        printf("\n");
    }

    for(int i = 0; i < count; i++){
        free(entries[i].name);
    }
    free(entries);

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
        printf(" ");
        printEntryNameColored(entry->d_name, st.st_mode);
        printf("\n");
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
