#ifndef EXECUTOR_H
#define EXECUTOR_H


#include "parser.h"

void mvCommand(Command* newCommand);
void cdCommand(Command* newCommand);
void lsCommand(Command* newCommand);
void executeCommand(Command* newCommand);

#endif