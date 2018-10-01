typedef struct sqlite3 storage_t;

storage_t* storage_init(const char* file);
void storage_free(storage_t** storage);

void runCommand(storage_t* db, char* command, plan_t* plan);