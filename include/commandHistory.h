#ifndef COMMAND_HIST
#define COMMAND_HIST

#include "parser.h"
#include "executor.h"
#include<stdlib.h>
#define MAX_SIZE 500

typedef struct {
    char cmd[256];
} historyEntry;

// External declarations (defined in .c file)
extern historyEntry History[MAX_SIZE];
extern int currentSize;

// Function declarations
void checkExistance();
void unloadHistory();
void newEntry(Command* cmd);
void loadHistory();

// Helper functions
char** convertToArgs(char *buffer, int *argCount);
char** getprevCmd();

#endif