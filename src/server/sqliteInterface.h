#include <sqlite3.h>

// Takes the command and runs it with sqlite3_exec
void runCommand(sqlite3* db, char* command, int user);
