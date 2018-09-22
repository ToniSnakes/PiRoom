#include <termios.h>
#include <stdio.h>

#define MESSAGE_BUFFER_LEN 256
#define CTRL_BUFFER_LEN 16

typedef struct terman_struct Terman;

Terman* terman_constructor();
void terman_destructor(Terman** self);

int terman_pushLine(const Terman* self, const char* line);
int terman_pollMessage(Terman* self, char** msg);