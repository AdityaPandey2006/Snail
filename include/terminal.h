#ifndef TERMINAL_H
#define TERMINAL_H

#include<termios.h>

//create to take RAW keyboard input directly not parsed input in the form of fgets()

void enableRawMode(struct termios *old);
void disableRawMode(struct termios *old);

#endif