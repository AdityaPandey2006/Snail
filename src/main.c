#include "repl.h"
#include "prompt.h"
#include <signal.h>
#include <stdio.h>

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    
    refreshPromptTimestamp();
    printPrompt();
}


int main(){
    signal(SIGINT, handle_sigint);

    setvbuf(stdout, NULL, _IONBF, 0); //for gui its very important as we dont want th 
    setvbuf(stderr, NULL, _IONBF, 0);

    replStart();
    return 0;
}
