#include <openssl/sha.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "initDB.h"
#include "sqlite3.h"
#include "paf.h"
#include "server.h"
#include "storage.h"

void sha256(char* string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    int len;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    len = strlen(string);
    SHA256_Update(&sha256, string, len);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", (unsigned char)hash[i]);
    }
    outputBuffer[64] = 0;
}

storage_t* storage_init(const char* file)
{
    sqlite3* db;
    if (sqlite3_open(file, &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return NULL;
    }
    return db;
}

void storage_free(storage_t** storage)
{
    sqlite3_close_v2(*storage);
    *storage = NULL;
}

// temporary (should generalize over paf)
// A callback function to be invoked for each result row coming out
// of the evaluated SQL statements
// The first paramater is taken from the fourth paramater of
// sqlite3_exec and in this case I use it to send the user sd
// The other paramaters are the results from the statements
static int callback(void* plan_void, int argc, char** argv, char** azColName)
{
    plan_t* plan = plan_void;
    for (int i = 0; i < argc; i++) {
        char message[BUFFSIZE];
        snprintf(message, BUFFSIZE, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");

        send(plan->sender, message, strlen(message), 0);
    }
    paf_finishFinalStep(paf, plan);
    return 0;
}

void runCommand(storage_t* db, char* command, plan_t* plan)
{
    char* errorMsg;
    int rc = sqlite3_exec(db, command, callback, plan, &errorMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errorMsg);
        sqlite3_free(errorMsg);
    }
}