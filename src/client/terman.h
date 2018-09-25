#include <termios.h>
#include <stdio.h>

typedef struct terman_struct Terman;

Terman* terman_constructor();
void terman_destructor(Terman** self);

int terman_pushLine(const Terman* self, const char* line);
int terman_pollMessage(Terman* self, char** msg);