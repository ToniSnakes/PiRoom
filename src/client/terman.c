/* Terminal Management */

#include "terman.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

enum CTRL {
    FAILED = -1,
    PARSING = 0,
    CTRL_UP = 1,
    CTRL_DOWN = 2,
    CTRL_FORWARD = 3,
    CTRL_BACK = 4
};

struct terman_struct {
    struct termios originalState;
    char messageBuffer[MESSAGE_BUFFER_LEN];
    char ctrlBuffer[CTRL_BUFFER_LEN];
    int messageLen;
    int cursorOffset;
    int messageComplete;
    int ctrlCodeParseStep;
};

#pragma region streamRaw
int terman_streamMakeraw(FILE* const stream, struct termios* const state)
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

int terman_streamRestore(FILE* const stream, const struct termios* const state)
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
#pragma endregion

#pragma region setup_cleanup
Terman* terman_constructor()
{
    Terman* self = malloc(sizeof(Terman));

    memset(self->messageBuffer, 0, MESSAGE_BUFFER_LEN);
    memset(self->ctrlBuffer, 0, CTRL_BUFFER_LEN);
    self->messageLen = 0;
    self->messageComplete = 0;
    self->ctrlCodeParseStep = 0;
    self->cursorOffset = 0;

    terman_streamMakeraw(stdin, &self->originalState);
    return self;
}
void terman_destructor(Terman** self)
{
    terman_streamRestore(stdin, &(*self)->originalState);
    free(*self);
    *self = NULL;
}
#pragma endregion

enum CTRL parseCtrlCode(Terman* self, char c)
{
    // parse controll code see https://en.wikipedia.org/wiki/ANSI_escape_code
    if (self->ctrlCodeParseStep == 1) {
        if (c == '[') {
            self->ctrlCodeParseStep = 2;
            return PARSING;
        } else {
            self->ctrlCodeParseStep = 0;
            return FAILED;
        }
    }

    // self->ctrlCodeParseStep == 2
    if (0x20 <= c && c <= 0x7E) {
        strncat(self->ctrlBuffer, &c, 1);
        if (c < 0x40) {
            return PARSING;
        } else {
            enum CTRL rtrn = FAILED;

            if (strcmp(self->ctrlBuffer, "A") == 0)
                rtrn = CTRL_UP;
            else if (strcmp(self->ctrlBuffer, "B") == 0)
                rtrn = CTRL_DOWN;
            else if (strcmp(self->ctrlBuffer, "C") == 0)
                rtrn = CTRL_FORWARD;
            else if (strcmp(self->ctrlBuffer, "D") == 0)
                rtrn = CTRL_BACK;

            self->ctrlCodeParseStep = 0;
            memset(self->ctrlBuffer, 0, CTRL_BUFFER_LEN);
            return rtrn;
        }
    }

    self->ctrlCodeParseStep = 0;
    return FAILED;
}

int terman_pushLine(const Terman* self, const char* line)
{
    printf("\r%s\n%s", line, self->messageBuffer);
    fflush(stdout);
}

int checkStdIn()
{
    // timeout is undefined after select (see 'man 2 select')
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    fd_set stdinfds;
    FD_ZERO(&stdinfds);
    FD_SET(STDIN_FILENO, &stdinfds);
    return select(STDIN_FILENO + 1, &stdinfds, NULL, NULL, &timeout);
}

int strninsert(char* str, char insert, int offset, int length)
{
    int i;
    for (i = length; i > offset; i--) {
        str[i] = str[i - 1];
    }
    str[i] = insert;
}

int strnremove(char* str, int offset, int length)
{
    for (int i = offset; i < length; i++) {
        str[i] = str[i + 1];
    }
}

int terman_pollMessage(Terman* self, char** msg)
{
    if (self->messageComplete) {
        self->messageComplete = 0;
        self->messageLen = 0;
        memset(self->messageBuffer, 0, MESSAGE_BUFFER_LEN);
    }

    if (checkStdIn()) {
        fflush(stdin);
        char c = getchar();

        switch (c) {
        case '\x7F': // backspace
            if (self->cursorOffset < self->messageLen) {
                strnremove(self->messageBuffer, self->messageLen - self->cursorOffset - 1, self->messageLen);
                self->messageLen--;
                printf("\033[D%s \033[%dD", self->messageBuffer + self->messageLen - self->cursorOffset, self->cursorOffset + 1);
                fflush(stdout);
            }
            return terman_pollMessage(self, msg);
        case '\n':
            if (self->messageLen > 0) {
                printf("\r\033[2K"); // move to start, clear entire line
                fflush(stdout);

                *msg = self->messageBuffer;
                self->messageComplete = 1;

                self->cursorOffset = 0;
                return self->messageLen;
            }
            return terman_pollMessage(self, msg);
        case '\003': // Ctrl + C
            printf("\n");
            fflush(stdout);
            return -1;
        case '\033': // see https://en.wikipedia.org/wiki/ANSI_escape_code
            self->ctrlCodeParseStep = 1;
            break;
        default:
            if (self->ctrlCodeParseStep) {
                enum CTRL ctrl = parseCtrlCode(self, c);
                if (ctrl == PARSING)
                    break;
                if (ctrl != FAILED) {
                    if (ctrl == CTRL_BACK && self->cursorOffset < self->messageLen) {
                        self->cursorOffset++;
                        printf("\033[D");
                    } else if (ctrl == CTRL_FORWARD && self->cursorOffset > 0) {
                        self->cursorOffset--;
                        printf("\033[C");
                    }
                    fflush(stdout);
                    break;
                }
            }

            // normal character (add to messageBuffer)
            if (self->messageLen < MESSAGE_BUFFER_LEN - 1 && c >= 32 && c <= 126) {
                strninsert(self->messageBuffer, c, self->messageLen - self->cursorOffset, self->messageLen);
                self->messageLen++;
                printf("%c%s", c, self->messageBuffer + self->messageLen - self->cursorOffset);
                if (self->cursorOffset != 0)
                    printf("\033[%dD", self->cursorOffset); // go left self->cursorOffset times
                fflush(stdout);
            }
            break;
        }
    }

    return 0;
}