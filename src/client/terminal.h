#include <termios.h>
#include <stdio.h>

#define MESSAGE_BUFFER_LEN 256
#define CTRL_BUFFER_LEN 16

int stream_makeraw(FILE* const stream, struct termios* const state);
int stream_restore(FILE* const stream, const struct termios* const state);

int pushLine(const char* line);
int pollMessage(char** msg);

int setupTerminal();
int cleanupTerminal();