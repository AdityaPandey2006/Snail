#ifndef EXECUTOR_H
#define EXECUTOR_H


#include "parser.h"

typedef struct{
    int shouldExit;
    int statusCode;
}executorResult;//will later make executor return this struct


executorResult mvCommand(Command* newCommand);
executorResult cdCommand(Command* newCommand);
executorResult lsCommand(Command* newCommand);
executorResult executeCommand(Command* newCommand);

#endif