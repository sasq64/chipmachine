#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int i;
	char buf[4096];
	int fd;
	int ret;
	int fail = 0;

	for (i = 1; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			fail = 1;
			continue;
		}
		while (1) {
			ret = read(fd, buf, sizeof buf);
			if (ret == 0)
				break;
			if (ret < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;
				fail = 1;
				break;
			}
		}
		close(fd);
	}

	return fail;
}
