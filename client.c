#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#define PORT 8888
#define BUFFSIZE 1024

int main (int argc, char **argv)
{
	int sockfd; // socket file descriptor
	int opt = 1; // socket option value
	char buffer[BUFFSIZE] = {0};
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
	
	if (connect(sockfd, (struct sockaddr *) &serv_addr, addrlen)) {
		perror("connection failed");
		return -1;
	} // connect to the server;
	
	printf("Connected to the server!\n");
	read(sockfd, buffer, BUFFSIZE);
	printf("%s", buffer);

	fd_set readfds;
	int max_sd;
	int activity;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	while (1) {
		FD_ZERO(&readfds); // clear the socket set
		FD_SET(sockfd, &readfds);
		max_sd = sockfd;
		
		activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
		
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");	
		}
		
		//if (activity) {
			//printf("Now typing!\n");
			char *message;
			size_t len = 0;
			getline(&message, &len, stdin);
			//printf("%s", message);
			//printf("\33[2K\r");
			//printf("\e[1;1H\e[2J");
			send(sockfd, message, strlen(message), 0);
		//}
		//if (FD_ISSET(sockfd, &readfds)) {
		//else {
		//if (!activity) {
			int valread = read(sockfd, buffer, BUFFSIZE);
			buffer[valread] = '\0';
			printf("\033[1A\r");
			printf("%s", buffer);
		//}
	}
	/*char *message;
	size_t len = 0;
	getline(&message, &len, stdin);
	//printf("%s", message);
	send(sockfd, message, strlen(message), 0);
	while (1) {
		memset(buffer, 0, sizeof(buffer));
		read(sockfd, buffer, BUFFSIZE);
		if (buffer[0]) {
			printf("%s", buffer);	
		}
		char *message;
		size_t len = 0;
		getline(&message, &len, stdin);
		//printf("%s", message);
		send(sockfd, message, strlen(message), 0);
	}*/
	
	return 0;	
}
