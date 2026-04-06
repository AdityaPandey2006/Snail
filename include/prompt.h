#ifndef PROMPT_H
#define PROMPT_H


void printPrompt(void);
void refreshPromptTimestamp(void);
void setLastCommandDurationMs(long long durationMs);
void setLastCommandStatus(int statusCode);


#endif
