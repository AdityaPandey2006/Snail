#include "prompt.h"
#include "config.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static long long gLastCommandDurationMs = -1;
static int gLastCommandStatus = 0;
static char gPromptTimeText[64] = {0};
static int gPromptNeedsFullRender = 1;

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

static void printAnsiColor(const char *hex){
    int red;
    int green;
    int blue;
    if(parseHexColor(hex, &red, &green, &blue)){
        printf("\033[38;2;%d;%d;%dm", red, green, blue);
    }
}

static void printRgbColor(int red, int green, int blue){
    printf("\033[38;2;%d;%d;%dm", red, green, blue);
}

static void printStyledPath(const char *path, const SnailConfig *config){
    if(path == NULL){
        return;
    }

    if(!config->rainbow_directory){
        printAnsiColor(config->directory_color);
        printf("%s", path);
        return;
    }

    static const int rainbowColors[][3] = {
        {255, 95, 86},
        {255, 184, 108},
        {255, 255, 102},
        {80, 250, 123},
        {97, 175, 239},
        {189, 147, 249}
    };

    size_t colorCount = sizeof(rainbowColors) / sizeof(rainbowColors[0]);
    size_t segmentIndex = 0;
    size_t i = 0;

    while(path[i] != '\0'){
        if(path[i] == '/'){
            printAnsiColor(config->foreground);
            putchar('/');
            i++;
            continue;
        }

        size_t start = i;
        while(path[i] != '\0' && path[i] != '/'){
            i++;
        }

        if(i > start){
            const int *color = rainbowColors[segmentIndex % colorCount];
            printRgbColor(color[0], color[1], color[2]);
            printf("%.*s", (int)(i - start), path + start);
            segmentIndex++;
        }
    }
}

static void formatPath(char *dest, size_t destSize, const char *cwd, const SnailConfig *config){
    if(dest == NULL || destSize == 0 || cwd == NULL || config == NULL){
        return;
    }

    const char *home = getenv("HOME");
    int homeRelative = 0;
    char homePath[PATH_MAX] = {0};
    if(home != NULL && strncmp(cwd, home, strlen(home)) == 0){
        homeRelative = 1;
        if(cwd[strlen(home)] == '\0'){
            snprintf(homePath, sizeof(homePath), "~");
        }
        else{
            snprintf(homePath, sizeof(homePath), "~%s", cwd + strlen(home));
        }
    }

    if(config->path_style == PROMPT_PATH_BASENAME){
        const char *lastSlash = strrchr(cwd, '/');
        if(lastSlash == NULL){
            snprintf(dest, destSize, "%s", cwd);
        }
        else if(*(lastSlash + 1) == '\0' && lastSlash != cwd){
            snprintf(dest, destSize, "%s", lastSlash);
        }
        else{
            snprintf(dest, destSize, "%s", lastSlash + 1);
        }
        return;
    }

    if(config->path_style == PROMPT_PATH_HOME_RELATIVE && homeRelative){
        snprintf(dest, destSize, "%s", homePath);
        return;
    }

    if(config->path_style == PROMPT_PATH_SHORTENED){
        const char *source = homeRelative ? homePath : cwd;
        char temp[PATH_MAX] = {0};
        strncpy(temp, source, sizeof(temp) - 1);

        char result[PATH_MAX] = {0};
        char *saveptr = NULL;
        char *token = strtok_r(temp, "/", &saveptr);
        int first = 1;
        while(token != NULL){
            char segment[PATH_MAX] = {0};
            char *nextToken = strtok_r(NULL, "/", &saveptr);

            if(first && source[0] == '~'){
                snprintf(segment, sizeof(segment), "~");
            }
            else if(nextToken == NULL){
                snprintf(segment, sizeof(segment), "%s", token);
            }
            else{
                snprintf(segment, sizeof(segment), "%c", token[0]);
            }

            if(strlen(result) > 0){
                strncat(result, "/", sizeof(result) - strlen(result) - 1);
            }

            strncat(result, segment, sizeof(result) - strlen(result) - 1);
            token = nextToken;
            first = 0;
        }

        if(result[0] == '\0'){
            snprintf(result, sizeof(result), "%s", source);
        }
        snprintf(dest, destSize, "%s", result);
        return;
    }

    snprintf(dest, destSize, "%s", cwd);
}

static int readGitBranchFromDirectory(const char *directoryPath, char *branch, size_t branchSize){
    char headPath[PATH_MAX] = {0};
    if(snprintf(headPath, sizeof(headPath), "%s/.git/HEAD", directoryPath) >= (int)sizeof(headPath)){
        return 0;
    }

    FILE *headFile = fopen(headPath, "r");
    if(headFile == NULL){
        return 0;
    }

    char line[256] = {0};
    if(fgets(line, sizeof(line), headFile) == NULL){
        fclose(headFile);
        return 0;
    }
    fclose(headFile);

    line[strcspn(line, "\n")] = '\0';
    if(strncmp(line, "ref: ", 5) == 0){
        const char *lastSlash = strrchr(line + 5, '/');
        const char *name = lastSlash == NULL ? line + 5 : lastSlash + 1;
        snprintf(branch, branchSize, "%s", name);
        return 1;
    }

    snprintf(branch, branchSize, "%.7s", line);
    return 1;
}

static int getGitBranch(const char *cwd, char *branch, size_t branchSize){
    char currentPath[PATH_MAX] = {0};
    strncpy(currentPath, cwd, sizeof(currentPath) - 1);

    while(1){
        if(readGitBranchFromDirectory(currentPath, branch, branchSize)){
            return 1;
        }

        char *lastSlash = strrchr(currentPath, '/');
        if(lastSlash == NULL){
            break;
        }
        if(lastSlash == currentPath){
            currentPath[1] = '\0';
            if(readGitBranchFromDirectory(currentPath, branch, branchSize)){
                return 1;
            }
            break;
        }
        *lastSlash = '\0';
    }

    return 0;
}

static void formatDuration(char *dest, size_t destSize, long long durationMs){
    if(durationMs < 1000){
        snprintf(dest, destSize, "%lldms", durationMs);
    }
    else if(durationMs < 60000){
        snprintf(dest, destSize, "%.1fs", durationMs / 1000.0);
    }
    else{
        long long totalSeconds = durationMs / 1000;
        long long minutes = totalSeconds / 60;
        long long seconds = totalSeconds % 60;
        snprintf(dest, destSize, "%lldm %llds", minutes, seconds);
    }
}

static void appendSegment(char *dest, size_t destSize, const char *text){
    size_t currentLength = strlen(dest);
    if(currentLength >= destSize - 1){
        return;
    }
    snprintf(dest + currentLength, destSize - currentLength, "%s", text);
}

static void appendMetadata(char *dest, size_t destSize, const char *separator, const char *text){
    if(text == NULL || text[0] == '\0'){
        return;
    }

    if(dest[0] != '\0'){
        appendSegment(dest, destSize, separator);
    }
    appendSegment(dest, destSize, text);
}

static void printPromptBody(const SnailConfig *config, const char *cwd){
    if(config->show_user){
        const char *user = getenv("USER");
        if(user != NULL){
            printAnsiColor(config->foreground);
            printf("%s", user);
            if(config->show_hostname){
                char host[256] = {0};
                if(gethostname(host, sizeof(host)) == 0){
                    printf("@%s", host);
                }
            }
            printf(" ");
        }
    }
    else if(config->show_hostname){
        char host[256] = {0};
        if(gethostname(host, sizeof(host)) == 0){
            printAnsiColor(config->foreground);
            printf("%s ", host);
        }
    }

    char displayPath[PATH_MAX] = {0};
    formatPath(displayPath, sizeof(displayPath), cwd, config);
    printStyledPath(displayPath, config);
}

static void buildPromptMeta(char *meta, size_t metaSize, const SnailConfig *config, const char *cwd){
    char gitBranch[128] = {0};
    if(config->show_git_branch && getGitBranch(cwd, gitBranch, sizeof(gitBranch))){
        char gitText[160] = {0};
        snprintf(gitText, sizeof(gitText), "on %s", gitBranch);
        appendMetadata(meta, metaSize, " | ", gitText);
    }

    if(config->show_exit_status && gLastCommandStatus != 0){
        char statusText[64] = {0};
        snprintf(statusText, sizeof(statusText), "exit %d", gLastCommandStatus);
        appendMetadata(meta, metaSize, " | ", statusText);
    }

    if(config->show_last_duration &&
       gLastCommandDurationMs >= 0 &&
       gLastCommandDurationMs >= config->duration_threshold_ms){
        char durationText[128] = {0};
        char formattedDuration[64] = {0};
        formatDuration(formattedDuration, sizeof(formattedDuration), gLastCommandDurationMs);
        snprintf(durationText, sizeof(durationText), "took %s", formattedDuration);
        appendMetadata(meta, metaSize, " | ", durationText);
    }

    if(config->show_time){
        if(gPromptTimeText[0] == '\0'){
            refreshPromptTimestamp();
        }
        appendMetadata(meta, metaSize, " | ", gPromptTimeText);
    }
}

void setLastCommandDurationMs(long long durationMs){
    gLastCommandDurationMs = durationMs;
}

void setLastCommandStatus(int statusCode){
    gLastCommandStatus = statusCode;
}

void refreshPromptTimestamp(void){
    time_t now = time(NULL);
    struct tm *localTime = localtime(&now);
    if(localTime == NULL){
        gPromptTimeText[0] = '\0';
        gPromptNeedsFullRender = 1;
        return;
    }

    strftime(gPromptTimeText, sizeof(gPromptTimeText), "%H:%M:%S", localTime);
    gPromptNeedsFullRender = 1;
}

void printPrompt(void){
    const SnailConfig *config = getSnailConfig();
    char workingDirectory[PATH_MAX] = {0};
    if(getcwd(workingDirectory, sizeof(workingDirectory)) == NULL){
        snprintf(workingDirectory, sizeof(workingDirectory), ".");
    }

    char promptMeta[512] = {0};
    buildPromptMeta(promptMeta, sizeof(promptMeta), config, workingDirectory);

    if((!config->two_line_prompt) || gPromptNeedsFullRender){
        if(gPromptNeedsFullRender && config->newline_before_prompt){
            printf("\n");
        }

        printPromptBody(config, workingDirectory);

        if(promptMeta[0] != '\0'){
            if(config->show_exit_status && gLastCommandStatus != 0){
                char *exitPos = strstr(promptMeta, "exit ");
                printAnsiColor(config->foreground);
                printf(" ");
                if(exitPos != NULL){
                    size_t prefixLength = (size_t)(exitPos - promptMeta);
                    if(prefixLength > 0){
                        printf("%.*s", (int)prefixLength, promptMeta);
                    }
                    char *afterExit = strstr(exitPos, " | ");
                    printAnsiColor(config->error_color);
                    if(afterExit != NULL){
                        printf("%.*s", (int)(afterExit - exitPos), exitPos);
                        printAnsiColor(config->foreground);
                        printf("%s", afterExit);
                    }
                    else{
                        printf("%s", exitPos);
                    }
                }
                else{
                    printf("%s", promptMeta);
                }
            }
            else{
                printAnsiColor(config->foreground);
                printf(" %s", promptMeta);
            }
        }

        if(config->two_line_prompt){
            printf("\n");
        }
    }

    printAnsiColor(config->prompt_color);
    printf("%s", config->separator);
    printf("\033[0m");
    fflush(stdout);
    gPromptNeedsFullRender = 0;
}
