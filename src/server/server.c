#include <arpa/inet.h> //close
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h> //close

#include "paf.h"
#include "sqlite3.h"

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

paf_t* paf;
sqlite3* db;
int master_socket;
int client_socket[MAX_CLIENTS];
char buffer[BUFFSIZE];

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

void runCommand(sqlite3* db, char* command, plan_t* plan)
{
    char* errorMsg;
    int rc = sqlite3_exec(db, command, callback, plan, &errorMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errorMsg);
        sqlite3_free(errorMsg);
    }
}

void sendToAll(int sender, const char* msg)
{
    for (int j = 0; j < MAX_CLIENTS; ++j) {
        int fd = client_socket[j];
        if (fd > 0) {
            if (dprintf(fd, "Client %c: %s", 'A' + sender, msg) <= 0) {
                printf("Could not send message to %d!\n", fd);
            }
        }
    }
}

void parseRequest(const char* msg, void* plan_void)
{
    plan_t* plan = plan_void;

    if (msg[0] == '\n') {
        paf_finishFinalStep(paf, plan);
        return;
    }

    if (msg[0] == '/') {
        printf("parsing cmd\n");
        if (strncmp("/list", msg, 5) == 0 && strlen(msg) > 6) {
            printf("is list cmd\n");
            plan->id = LIST;
            strncpy(plan->data.list.dbName, msg + 6, 32);
        } else {
            plan->id = ERROR;
            strncpy(plan->data.error.msg, "not a valid command", 32);
        }
    } else {
        plan->id = ECHO;
    }

    paf_finishStep(paf, plan);
}

void nextStep(const char* msg, void* plan_void)
{
    plan_t* plan = plan_void;
    printf("step id: %d\n", plan->id);
    char cmd[64];
    switch (plan->id) {
    case ECHO:
        sendToAll(plan->sender, msg);
        paf_finishFinalStep(paf, plan);
        break;
    case LIST:
        printf("listing\n");
        snprintf(cmd, 64, "SELECT * FROM %s", plan->data.list.dbName);
        runCommand(db, cmd, plan);
        break;
    case ERROR:
    default:
        paf_finishFinalStep(paf, plan);
        break;
    }
}

int createDescriptorSet(fd_set* set)
{
    FD_ZERO(set); // clear the socket set
    FD_SET(master_socket, set); // add master socket to set
    int max_sd = master_socket; // max value of sd

    //add children to the set
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int sd = client_socket[i]; // socket descriptor
        //if valid descriptor add to list
        if (sd > 0) {
            FD_SET(sd, set);
        }
        //highest file descriptor number, need it for select fct
        if (sd > max_sd) {
            max_sd = sd;
        }
    }

    return max_sd;
}

int createSocket()
{
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("master socket creation failed");
        return -1;
    } // create the master socket

    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("socket options failed");
        return -1;
    } // set socket options

    struct sockaddr_in address = {
        .sin_family = AF_INET, // always set family to AF_INET
        .sin_port = htons(PORT),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("binding failed");
        return -1;
    } // bind master socket to port 8888
    return 0;
}

int main(int argc, char** argv)
{
    memset(client_socket, 0, MAX_CLIENTS * sizeof(int));

    paf = paf_init(sizeof(plan_t), nextStep, parseRequest);

#pragma region init_sqlite
    int rc = sqlite3_open("testdb", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return (1);
    }
#pragma endregion

    if (createSocket() == -1)
        return -1;

    if (listen(master_socket, 3) < 0) {
        perror("listening failed");
        return -1;
    } // set the master socket to listen
    printf("Server is up and running!\n");
    printf("Master socket is %d\n", master_socket);
    printf("Waiting for connections...\n");

    while (1) {
        paf_dispatch(paf);

        fd_set readfds; // set of socket descriptors
        int max_sd = createDescriptorSet(&readfds);

        // wait for an activity on one of the sockets, timeout NULL
        // so wait indefinately
        if ((select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //if something happened on the master socket,
        // then it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
#pragma region add_new_client
            struct sockaddr_in address;
            socklen_t addrlen;
            int new_socket;
            if ((new_socket = accept(master_socket, (struct sockaddr*)&address, &addrlen)) < 0) {
                perror("accept failure");
                return -1;
            }

            // describe the new connection
            printf("New connection, socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            //send new connection greeting message
            if (dprintf(new_socket, "Welcome to the server!\n") <= 0) {
                printf("Welcome message not sent!\n");
            } else {
                printf("New client welcomed!\n");
            }

            //add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                // if position is empty;
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
#pragma endregion
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = client_socket[i];

            if (!FD_ISSET(sd, &readfds))
                continue;

#pragma region communicate_with_client
            //check if it was for closing, and read the
            //incoming message
            int valread;
            if ((valread = read(sd, buffer, BUFFSIZE - 1)) == 0) {
                // somebody disconnected, get his details and
                // print
                struct sockaddr_in address;
                socklen_t addrlen;
                getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                // close the socket and mark as 0 in the list
                close(sd);
                client_socket[i] = 0;
            } else {
                plan_t planInit = { .sender = sd };
                buffer[valread] = '\0';
                paf_newMessage(paf, buffer, &planInit);
            }
#pragma endregion
        }
    }

    return 0;
}