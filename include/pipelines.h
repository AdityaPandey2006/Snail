#ifndef PIPES_H
#define PIPES_H

#include "parser.h"
#include<stdio.h>

typedef struct{
    Command *command;
    int numCommands;
}pipeline;

#endif