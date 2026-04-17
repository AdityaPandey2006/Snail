#include "repl.h"
#include "prompt.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handle_sigint(int sig) {
    (void)sig;
    const char newline = '\n';
    write(STDOUT_FILENO, &newline, 1);
}


int main(){
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // keep read() interruptible so REPL can redraw prompt on EINTR
    sigaction(SIGINT, &sa, NULL);

    setvbuf(stdout, NULL, _IONBF, 0); //for gui its very important as we dont want th 
    setvbuf(stderr, NULL, _IONBF, 0);

    replStart();
    return 0;
}
