#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include "terminal.h"
#define PORT 8888
#define BUFFSIZE 1024

int setnonblock(int sock) {
   int flags;
   flags = fcntl(sock, F_GETFL, 0);
   if (-1 == flags)
      return -1;
   return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char** argv)
{
    int sockfd; // socket file descriptor
    int opt = 1; // socket option value
    char buffer[BUFFSIZE] = { 0 };
    struct sockaddr_in serv_addr;
    int addrlen = sizeof(serv_addr);
    struct timeval timeout; // select timeout

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    } // socket creation

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("socket options failed");
        return -1;
    } // set socket options

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; // always set family to AF_INET
    serv_addr.sin_port = htons(PORT);
    //serv_addr.sin_addr.s_addr = INADDR_LOOPBACK; // localhost

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("invalid address");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, addrlen)) {
        perror("connection failed");
        return -1;
    } // connect to the server;

    //printf("Connected to the server!\n"); // this sometimes does
	//fflush(stdout); // print, so I took it out
    //read(sockfd, buffer, BUFFSIZE); // this blocks reading for some
    //printf("%s", buffer);			  // unknown reason

    fd_set readfds;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

	char message[50];
	memset(message, 0, 50);
	size_t messageLen = 0;
	
	struct termios saved;
	stream_makeraw(stdin, &saved);
	
	char i = 0;

	memset(buffer, 0, BUFFSIZE);

	setnonblock(sockfd); // sets the socket to non-blocking, so it
						 // only reads when it has something to read
	
	fd_set servfds; // when the server read was blocking, I tried
    int max_sd = sockfd; // with select, but it was not the answer
    int activity;
	struct timeval servto;
	servto.tv_sec = 1;
	servto.tv_sec = 0;

	FD_ZERO(&servfds); // still from the failed select attempt
	FD_SET(sockfd, &servfds);
	activity = select(max_sd + 1, &servfds, NULL, NULL, &servto);
	
	while (1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		
		if (select(1, &readfds, NULL, NULL, &timeout)) {
			fflush(stdin);
			char c = getchar();
			printf("%c", c);
			fflush(stdout);
			if (c == 127) { // backspace
				if ((int)messageLen > 0) { // typecasting for compare
					message[--messageLen] = '\0';
					printf("\r");
					printf("%s ", message);
					printf("\033[1D");
					fflush(stdout);
				}
			}
			else if (c == '\n') {
				if (messageLen != 0) {
					printf("\033[1A\r");
					printf("                                   \r");
					send(sockfd, message, strlen(message), 0);
					//printf("Message: %s\n", message);
					messageLen = 0;
					memset(message, 0, 50);
				}
				/*else { // only partly stops the empty line problem
					printf("\033[1A\r"); // because the terminal is
					fflush(stdout); // already one line down, so I
				}*/ // temporarily removed it since it looks weird
			}
			else if (c == 'Q') {
				break;
			}
			else {
				message[messageLen++] = c;	
			}
		}

		if (activity == 0) { // to check for timeout
			printf("timeout\n\n\n"); // spoiler alert
			fflush(stdout); // it never times out
		}
		else if (activity) { // so this is basically always true
			read(sockfd, buffer, BUFFSIZE);
			if (strlen(buffer)) { // if there is a new message
				printf("\r%s\n%s", buffer, message);
				fflush(stdout);
				memset(buffer, 0, BUFFSIZE);
			}
		}
	}
	
	stream_restore(stdin, &saved);

    return 0;
}
