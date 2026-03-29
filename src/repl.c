#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "repl.h"
#include "executor.h"
#include "parser.h"
#include "fileDump.h"
#define COMMANDSIZE 50

void snailPrinter(){
        printf(
"                                                              d::\n"
"                                                              ''d$:     :h\n"
"                                                                 d$   :$h'\n"
"                              .......                             d$..$h'\n"
"                         ..cc$$$$$$$$$c.                  ...c$$$$$$$$$h\n"
"                       .d$$'        '$$$h.              cc$$$$:::::d$!$h\n"
"                     c$$'              '$$c           .$$$$:::()::d$!$h'\n"
"                   .c$$'                 $$h.        .d$$::::::::d$!!$h\n"
"                 .$$h'                    $$$.       $$$::::::::$$!!$h'\n"
"                .$$h'      .;dd,           $$$.      $$$:::::::d$!!$h'\n"
"                $$$'      dd$$$hh,          $$$.     $$$:::::::d$!!$h\n"
"                $$$      d$$' '$hh.          $$$.   .$$$:::::d$!!!!$h\n"
"                $$$      $$d    $$$           d$$.  $$$:::::d$!!!!$h'\n"
"                $$$      $$$ d  $$$          .d$$$.c$$::::::d$!!!!$h\n"
"                $$$.     '$$$$  $$$         .$$$$$$$$::::::d$$!!!$h'\n"
"                '$$h.           $$$       .d$$:::::::::::::d$$!!!$h\n"
"                  \"$\"$$h        .$$$    ,$$$:::::::::::::::d$$!!!$h'\n"
"..hh              .c$$:$h       $$$'   ,$$$::::::::::::::d$$!!!$h'\n"
"d$$           .c$$$:::$$h,     $$h    $$$:::::::::::::d$$$!!!$h'\n"
"$$$.        .$$h::::::::$$$dddd$h'   $$$::::::::::::dd$$!!!!$h'\n"
"'$$$$c...cc$$h:::::::::::$$$$$$$hhhh$$$::::::::::d$$$$$$$hhh'\n"
" '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$hhhhhhhh'''\n"
    );
return;
}

int readInput(){
    char input[COMMANDSIZE];
    printPrompt();
    if(fgets(input,sizeof(input),stdin)){
        int enterPosition=strcspn(input,"\n");
        input[enterPosition]='\0';
        if(strlen(input)==0){
            return 0;
        }//hitting enter should bring the prompt back again, command[enterposition makes string length 0]
        if (strcmp(input, "\f") == 0 || strcmp(input, "^L") == 0) {
            printf("\033[H\033[J");
            return 0;
        }
        else{
            Command newCommand=parseCommand(input);
            executorResult result;
            result=executeCommand(&newCommand);
            freeCommand(&newCommand);
            return result.shouldExit;
        }
    }
    else if(feof(stdin)){
        printf("BYE SNAILER \n");
        return 1;//ctrl+d should exit the file
    }
    else if(ferror(stdin)){
        printf("Error reading the input");
        return 1;
    }
    return 1;
}



void replStart(){
    int dumpCleanSuccess=cleanDump();
    if(!dumpCleanSuccess){
        printf("Warning: Dump cleanup error\n");
    }
    snailPrinter();
    while(1){
        int quit=readInput();
        if(quit){
            break;
        }
    }
}