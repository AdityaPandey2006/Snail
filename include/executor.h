#ifndef EXECUTOR_H
#define EXECUTOR_H


#include "parser.h"

typedef struct{
    int shouldExit;
    int statusCode;
}executorResult;//will later make executor return this struct



executorResult executeCommand(Command* newCommand);


#endif