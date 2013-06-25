#ifndef _UADE123_TERMINAL_H_
#define _UADE123_TERMINAL_H_

extern int terminal_fd;

#define UADE_CURSOR_LEFT  0x1000
#define UADE_CURSOR_RIGHT 0x1001
#define UADE_CURSOR_DOWN  0x1002
#define UADE_CURSOR_UP    0x1003

void pause_terminal(void);
int read_terminal(void);
void setup_terminal(void);

#endif
