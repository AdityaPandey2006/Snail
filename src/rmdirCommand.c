#include <stdio.h>
#include <unistd.h>
#include "rmdirCommand.h"

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

        if (rmdir(dir) != 0) {
            perror("rmdir");
            result.statusCode = 1;
            continue;
        }
    }

    return result;
}