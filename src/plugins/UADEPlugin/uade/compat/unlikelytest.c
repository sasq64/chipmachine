#include <stdio.h>

int main(int argc, char *argv[])
{
	if (__builtin_expect(!!(argc % 2), 1))
		printf("\n");
	return 0;
}
