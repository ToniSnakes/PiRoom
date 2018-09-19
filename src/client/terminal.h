#include <errno.h>
#include <stdio.h>
#include <termios.h>

int stream_makeraw(FILE* const stream, struct termios* const state);
int stream_restore(FILE* const stream, const struct termios* const state);