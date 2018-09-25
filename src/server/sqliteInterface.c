#include "sqliteInterface.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define BUFFSIZE 1024

// A callback function to be invoked for each result row coming out
// of the evaluated SQL statements
// The first paramater is taken from the fourth paramater of
// sqlite3_exec and in this case I use it to send the user sd
// The other paramaters are the results from the statements
static int callback(void* plan, int argc, char** argv, char** azColName)
{
    int user = *((int*)plan);
    //printf("%d\n", user);
    int i;
    for (i = 0; i < argc; i++) {
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        char message[BUFFSIZE];
        memset(message, 0, BUFFSIZE);
        strcpy(message, azColName[i]);
        char* extra1 = " = ";
        strcat(message, extra1);
        char* extra2 = "NULL";
        if (argv[i]) {
            strcat(message, argv[i]);
        } else {
            strcat(message, extra2);
        }
        message[strlen(message)] = '\n';
        send(user, message, strlen(message), 0);
    }
    //printf("\n");
    return 0;
}

void runCommand(sqlite3* db, char* command, int user)
{
    int rc;
    char* errorMsg;
    rc = sqlite3_exec(db, command, callback, &user, &errorMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errorMsg);
        sqlite3_free(errorMsg);
    }
}