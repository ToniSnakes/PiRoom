// A callback function to be invoked for each result row coming out
// of the evaluated SQL statements
// The first paramater is taken from the fourth paramater of
// sqlite3_exec and in this case I use it to send the user sd
// The other paramaters are the results from the statements
static int callback(void* NotUsed, int argc, char** argv, char** azColName);

// Takes the command and runs it with sqlite3_exec
void runCommand(sqlite3* db, char* command, int user, char* zErrMsg);
