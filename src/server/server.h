#include "paf.h"

#define PORT 8888
#define BUFFSIZE 1024
#define MAX_CLIENTS 30

typedef struct {
    int sender;
    enum {
        ECHO = 1,
        LIST = 2,
        ERROR = 3
    } id;
    union {
        struct {
            char dbName[32];
        } list;
        struct {
            char msg[32];
        } error;
    } data;
} plan_t;

extern paf_t* paf; 
