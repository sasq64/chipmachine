#include "terminal.h"
#include "uade123.h"

#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static struct termios old_terminal;

int terminal_fd = -1;

static int cursormode;

static void restore_terminal(void)
{
	if (terminal_fd >= 0)
		tcsetattr(terminal_fd, TCSANOW, &old_terminal);
}

void pause_terminal(void)
{
	char c;
	int ret;
	fd_set rfds;

	if (terminal_fd < 0)
		return;

	tprintf("\nPaused. Press any key to continue...\n");

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(terminal_fd, &rfds);

		ret = select(terminal_fd + 1, &rfds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("\nuade123: poll error");
			exit(1);
		}

		if (ret == 0)
			continue;

		ret = read(terminal_fd, &c, 1);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
		}

		break;
	}

	tprintf("\n");
}


int read_terminal(void)
{
	char buf;
	int c;
	int ret = read(terminal_fd, &buf, 1);
	if (ret <= 0)
		buf = 0;
	c = buf;

	switch (cursormode) {
	case 0:
		if (c == 0x1b) {
			/* ESC */
			cursormode = 1;
			c = 0;
		}
		break;
	case 1:
		if (c == '[') {
			cursormode = 2;
		} else {
			cursormode = 0;
		}
		c = 0;
		break;

	case 2:
		switch (c) {
		case 'A':
			c = UADE_CURSOR_UP;
			break;
		case 'B':
			c = UADE_CURSOR_DOWN;
			break;
		case 'C':
			c = UADE_CURSOR_RIGHT;
			break;
		case 'D':
			c = UADE_CURSOR_LEFT;
			break;
		default:
			fprintf(stderr, "Unknown terminal escape code: %.2x\n", c);
			c = 0;
			break;
		}
		cursormode = 0;
		break;
	}

	return c;
}


void setup_terminal(void)
{
	struct termios tp;
	int fd;

	fd = open("/dev/tty", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can not use /dev/tty for control. Trying to use stdin.\n");
		fd = 0;
	}

	if (tcgetattr(fd, &old_terminal)) {
		perror("uade123: can't setup interactive mode");
		return;
	}

	terminal_fd = fd;

	atexit(restore_terminal);

	tp = old_terminal;
	tp.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
	if (tcsetattr(fd, TCSAFLUSH, &tp)) {
		perror("uade123: can't setup interactive mode (tcsetattr())");
		return;
	}
}
