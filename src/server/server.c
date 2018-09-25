#include "sqliteInterface.h"

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

#define PORT 8888
#define BUFFSIZE 1024
#define MAX_CLIENTS 30

int main(int argc, char** argv)
{
    int master_socket; // master socket, duh
    struct sockaddr_in address; // server address
    int addrlen; // address length
    char buffer[BUFFSIZE] = { 0 };
    fd_set readfds; // set of socket descriptors
    int new_socket; // new socket, duuh
    char* welcome = "Welcome to the server!\n";
    int valread; // return value of read
    int client_socket[MAX_CLIENTS];
    int activity; // value of activity
    sqlite3* db; // the server's database
    char* database = "testdb"; // placeholder
    int rc; // return from database opening

#pragma region init_sqlite
    rc = sqlite3_open(database, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return (1);
    }
#pragma endregion

    memset(client_socket, 0, MAX_CLIENTS * sizeof(int));

#pragma region init_socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("master socket creation failed");
        return -1;
    } // create the master socket

    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("socket options failed");
        return -1;
    } // set socket options

    address.sin_family = AF_INET; // always set family to AF_INET
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    addrlen = sizeof(address);
    if (bind(master_socket, (struct sockaddr*)&address, addrlen) < 0) {
        perror("binding failed");
        return -1;
    } // bind master socket to port 8888
#pragma endregion

    if (listen(master_socket, 3) < 0) {
        perror("listening failed");
        return -1;
    } // set the master socket to listen
    printf("Server is up and running!\n");
    printf("Master socket is %d\n", master_socket);
    printf("Waiting for connections...\n");

    while (1) {
        FD_ZERO(&readfds); // clear the socket set
        FD_SET(master_socket, &readfds); // add master socket to set
        int max_sd = master_socket; // max value of sd

        //add children to the set
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = client_socket[i]; // socket descriptor
            //if valid descriptor add to list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            //highest file descriptor number, need it for select fct
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // wait for an activity on one of the sockets, timeout NULL
        // so wait indefinately
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //if something happened on the master socket,
        // then it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
#pragma region add_new_client
            if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept failure");
                return -1;
            }

            // describe the new connection
            printf("New connection, socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            //send new connection greeting message
            if (send(new_socket, welcome, strlen(welcome), 0) != strlen(welcome)) {
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

            if (FD_ISSET(sd, &readfds)) {
#pragma region communicate_with_client
                //check if it was for closing, and read the
                //incoming message
                if ((valread = read(sd, buffer, BUFFSIZE)) == 0) {
                    // somebody disconnected, get his details and
                    // print
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // close the socket and mark as 0 in the list
                    close(sd);
                    client_socket[i] = 0;
                }

                //echo back the message that came in
                else {
                    if (buffer[0] == '\n') {
                        continue;
                    }
                    // set the string terminating NULL byte on the
                    // end of the data read
                    buffer[valread] = '\0';
                    if (buffer[0] == '/') {
                        // if the buffer is a command known
                        char* scommand = "/list users";
                        if (strcmp(scommand, buffer) == 0) {
                            char* command = "SELECT * FROM test;";
                            runCommand(db, command, sd);
                            continue;
                        }
                    }
                    char message[BUFFSIZE];
                    char* prefix = "Client ";
                    char* prefix2 = ": ";
                    char name = 'A' + i;
                    strcpy(message, prefix);
                    int plen = strlen(prefix);
                    message[plen] = name;
                    message[plen + 1] = '\0';
                    strcat(message, prefix2);
                    strcat(message, buffer);
                    for (int j = 0; j < MAX_CLIENTS; ++j) {
                        int csd = client_socket[j];
                        if (csd > 0) {
                            if (send(csd, message, strlen(message), 0) != strlen(message)) {
                                printf("Could not send message to %d!\n", csd);
                            }
                        }
                    }
                }
#pragma endregion
            }
        }
    }

    return 0;
}