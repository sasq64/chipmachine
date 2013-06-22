#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "uade123.h"
#include "terminal.h"


static struct termios old_terminal;
static int terminal_is_set;
static int terminal_fd;


static void uade_restore_terminal(void)
{
  if (terminal_is_set)
    tcsetattr(terminal_fd, TCSANOW, &old_terminal);
}


void pause_terminal(void)
{
  char c;
  int ret;
  fd_set rfds;

  if (!terminal_is_set)
    return;

  tprintf("\nPaused. Press any key to continue...\n");

  while (uade_terminated == 0) {
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


int poll_terminal(void)
{
  fd_set rfds;
  char c = 0;
  int ret;

  if (!terminal_is_set)
    return 0;

  FD_ZERO(&rfds);
  FD_SET(terminal_fd, &rfds);
  ret = select(terminal_fd + 1, &rfds, NULL, NULL, & (struct timeval) {.tv_sec = 0});

  if (ret > 0) {
    ret = read(terminal_fd, &c, 1);
    if (ret <= 0)
      c = 0;
  }

  return c;
}


void setup_terminal(void)
{
  struct termios tp;
  int fd;

  terminal_is_set = 0;

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
  terminal_is_set = 1;

  atexit(uade_restore_terminal);

  tp = old_terminal;
  tp.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
  if (tcsetattr(fd, TCSAFLUSH, &tp)) {
    perror("uade123: can't setup interactive mode (tcsetattr())");
    return;
  }
}
