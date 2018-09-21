#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "terminal.h"

#define PORT 8888
#define BUFFSIZE 1024

const int ONE = 1;
int createSocketClient(const char* addr, int port, int options, int flags)
{
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    } // socket creation

    if (setsockopt(sockfd, SOL_SOCKET, options, &ONE, sizeof(ONE))) {
        perror("socket options failed");
        return -1;
    } // set socket options

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; // always set family to AF_INET
    serv_addr.sin_port = htons(port); // htons converts byte order

    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        printf("invalid address");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        perror("connection failed");
        return -1;
    }

    // non-blocking
    int sockFlags = fcntl(sockfd, F_GETFL, 0);
    if (sockFlags == -1) {
        perror("unable to get flags");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, sockFlags | flags) == -1) {
        perror("unable to set flags");
        return -1;
    }

    return sockfd;
}

int main(int argc, char** argv)
{
    char buffer[BUFFSIZE];
    memset(buffer, 0, BUFFSIZE);

    int sockfd = createSocketClient("127.0.0.1", PORT, SO_REUSEADDR | SO_REUSEPORT, O_NONBLOCK);
    if (sockfd == -1) {
        perror("error while creating socket");
        return -1;
    }


    memset(buffer, 0, BUFFSIZE);

    int activity;
    struct timeval servto;
    servto.tv_sec = 1;
    servto.tv_sec = 0;

    fd_set servfds;
    FD_ZERO(&servfds);
    FD_SET(sockfd, &servfds);
    activity = select(sockfd + 1, &servfds, NULL, NULL, &servto);

    if (!activity) {
        printf("timeout\n\n\n");
        fflush(stdout);
        return -1;
    }
    
    setupTerminal();

    int quit = 0;
    while (!quit) {

        char* msg;
        int res = pollMessage(&msg);
        switch (res) {
        case -1:
            quit = 1;
            break;
        case 0:
            break;
        default:
            send(sockfd, msg, res, 0);
            break;
        }

        read(sockfd, buffer, BUFFSIZE);

        if (strlen(buffer)) { // if there is a new message
            pushLine(buffer);
            memset(buffer, 0, BUFFSIZE);
        }
    }

    cleanupTerminal();

    return 0;
}
