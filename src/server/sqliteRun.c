#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#define BUFFSIZE 1024

static int callback(void* NotUsed, int argc, char** argv, char** azColName)
{
    int user = *((int*)NotUsed);
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

void runCommand(sqlite3* db, char* command, int user, char* zErrMsg)
{
    int rc;
    //int used = 999;
    rc = sqlite3_exec(db, command, callback, &user, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}