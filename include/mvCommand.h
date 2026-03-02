#ifndef MOVE_H
#define MOVE_H
#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include <stdio.h>
#include <string.h>

executorResult mvCommand(Command *cmd);

#endif