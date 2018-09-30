#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>

#include "initDB.h"
#include "sqlite3.h"

typedef struct sqlite3 storage_t;

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