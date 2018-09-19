#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Call this to change the terminal related to the stream to "raw" state.
 * (Usually you call this with stdin).
 * This means you get all keystrokes, and special keypresses like CTRL-C
 * no longer generate interrupts.
 *
 * You must restore the state before your program exits, or your user will
 * frantically have to figure out how to type 'reset' blind, to get their terminal
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

int main(void)
{
    fd_set readfds;
    FD_ZERO(&readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    char message[50];
    memset(message, 0, 50);
    size_t messageLen = 0;

    struct termios saved;
    stream_makeraw(stdin, &saved);

    char i = 0;

    while (1) {
        FD_SET(STDIN_FILENO, &readfds);

        if (select(1, &readfds, NULL, NULL, &timeout)) {
            fflush(stdin);
            char c = getchar();
            printf("%c", c);
            fflush(stdout);
			if (c == 127) {
				//printf("\n\n\nDELETE\n\n\n");	
				//fflush(stdout);
				message[--messageLen] = '\0';
                printf("\r");
				printf("%s ", message);
				printf("\033[1D");
				fflush(stdout);
			}
			else if (c == '\n') {
                printf("\033[1A\r");
                printf("Message: %s\n", message);
                messageLen = 0;
                memset(message, 0, 50);
            } else if (c == 'Q') {
                goto end;
            } else {
                message[messageLen++] = c;
            }
        }

        if (!i++) {
            printf("\r.............................................\n");
            printf("%s", message);
            fflush(stdout);
        }

        usleep(10000);
    }

end:
    stream_restore(stdin, &saved);
    return (0);
}
