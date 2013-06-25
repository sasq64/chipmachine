#define _GNU_SOURCE
#include <stdlib.h>

int main(void)
{
	canonicalize_file_name("/");
	return 0;
}
