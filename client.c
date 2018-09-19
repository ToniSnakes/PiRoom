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
#include <termios.h>
#include <fcntl.h>
#define PORT 8888
#define BUFFSIZE 1024

/* Call this to change the terminal related to the stream to "raw" state.
 * (Usually you call this with stdin).
 * This means you get all keystrokes, and special keypresses like CTRL-C
 * no longer generate interrupts.
 *
 * You must restore the state before your program exits, or your user wi$
 * frantically have to figure out how to type 'reset' blind, to get thei$
 * back to a sane state.
 *
 * The function returns 0 if success, errno error code otherwise.
*/

static int stream_makeraw(FILE* const stream, struct termios* const state)
{
    struct termios old, raw, actual;
    int fd;

    if (!stream)
        return errno = EINVAL;

    /* Tell the C library not to buffer any data from/to the stream. */
    if (setvbuf(stream, NULL, _IONBF, 0))
        return errno = EIO;

    /* Write/discard already buffered data in the stream. */
    fflush(stream);

    /* Termios functions use the file descriptor. */
    fd = fileno(stream);
    if (fd == -1)
        return errno = EINVAL;

    /* Discard all unread input and untransmitted output. */
    tcflush(fd, TCIOFLUSH);

    /* Get current terminal settings. */
    if (tcgetattr(fd, &old))
        return errno;

    /* Store state, if requested. */
    if (state)
        *state = old; /* Structures can be assigned! */
	
	/* New terminal settings are based on current ones. */
    raw = old;

    /* Because the terminal needs to be restored to the original state,
     * you want to ignore CTRL-C (break). */
    raw.c_iflag |= IGNBRK; /* do ignore break, */
    raw.c_iflag &= ~BRKINT; /* do not generate INT signal at break. */

    /* Make sure we are enabled to receive data. */
    raw.c_cflag |= CREAD;

    /* Do not generate signals from special keypresses. */
    raw.c_lflag &= ~ISIG;

    /* Do not echo characters. */
    raw.c_lflag &= ~ECHO;

    /* Most importantly, disable "canonical" mode. */
    raw.c_lflag &= ~ICANON;

    /* In non-canonical mode, we can set whether getc() returns immediately
     * when there is no data, or whether it waits until there is data.
     * You can even set the wait timeout in tenths of a second.
     * This sets indefinite wait mode. */
    raw.c_cc[VMIN] = 1; /* Wait until at least one character available, */
    raw.c_cc[VTIME] = 0; /* Wait indefinitely. */

    /* Set the new terminal settings. */
    if (tcsetattr(fd, TCSAFLUSH, &raw))
        return errno;
	
	/* tcsetattr() is happy even if it did not set *all* settings.
     * We need to verify. */
    if (tcgetattr(fd, &actual)) {
        const int saved_errno = errno;
        /* Try restoring the old settings! */
        tcsetattr(fd, TCSANOW, &old);
        return errno = saved_errno;
    }

    if (actual.c_iflag != raw.c_iflag || actual.c_oflag != raw.c_oflag || actual.c_cflag != raw.c_cflag || actual.c_lflag != raw.c_lflag) {
        /* Try restoring the old settings! */
        tcsetattr(fd, TCSANOW, &old);
        return errno = EIO;
    }

    /* Success! */
    return 0;
}

/* Call this to restore the saved state.
 *
 * The function returns 0 if success, errno error code otherwise.
*/
static int stream_restore(FILE* const stream, const struct termios* const state)
{
    int fd, result;

    if (!stream || !state)
        return errno = EINVAL;

    /* Write/discard all buffered data in the stream. Ignores errors. */
    fflush(stream);

    /* Termios functions use the file descriptor. */
    fd = fileno(stream);
    if (fd == -1)
        return errno = EINVAL;

	/* Discard all unread input and untransmitted output. */
    do {
        result = tcflush(fd, TCIOFLUSH);
    } while (result == -1 && errno == EINTR);

    /* Restore terminal state. */
    do {
        result = tcsetattr(fd, TCSAFLUSH, state);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        return errno;

    /* Success. */
    return 0;
}

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
