#ifndef RM_H
#define RM_H
#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include <stdio.h>
#include <string.h>


executorResult fileRemoval(const char *filePath);
executorResult rmCommand(Command *newCommand);

#endif