#include "repl.h"
#include "prompt.h"
#include <signal.h>
#include <stdio.h>

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    
    printPrompt();
}


int main(){
    signal(SIGINT, handle_sigint);
    replStart();
    return 0;
}