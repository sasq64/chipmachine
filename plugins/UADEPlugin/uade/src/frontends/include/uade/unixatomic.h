#ifndef _UADE_UNIXATOMIC_H_
#define _UADE_UNIXATOMIC_H_

#include <stdio.h>
#include <unistd.h>

int uade_atomic_close(int fd);
int uade_atomic_dup2(int oldfd, int newfd);
ssize_t uade_atomic_read(int fd, const void *buf, size_t count);
ssize_t uade_atomic_write(int fd, const void *buf, size_t count);

#endif
