#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "touchCommand.h"

executorResult touchCommand(Command* newCommand) {

    executorResult result;
    result.shouldExit = 0;
    result.statusCode = 0;

    if (newCommand->argCount < 2) {
        fprintf(stderr, "touch: missing file operand\n");
        result.statusCode = 1;
        return result;
    }
    
    for (int i = 1; i < newCommand->argCount; i++) {

        char *filename = newCommand->arguments[i];

        // Create file if it doesn't exist
        int fd = open(filename, O_WRONLY | O_CREAT, 0644);

        if (fd < 0) {
            perror("touch");
            result.statusCode = 1;
            continue;
        }

        close(fd);

        // Update timestamps
        if (utime(filename, NULL) != 0) {
            perror("touch");
            result.statusCode = 1;
            continue;
        }
    }

    return result;
}