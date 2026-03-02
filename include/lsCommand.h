#ifndef LS_H
#define LS_H
#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include <stdio.h>
#include <string.h>

executorResult lsCommand(Command *cmd); //** pouinter to pointer of character...basically a way to point to a array of strings...... */

#endif