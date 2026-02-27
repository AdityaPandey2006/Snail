#ifndef CD_H
#define CD_H
#include "parser.h"
#include "executor.h"
#include "exitCommand.h"
#include <stdio.h>
#include <string.h>

executorResult cdCommand(Command* newCommand);

#endif
