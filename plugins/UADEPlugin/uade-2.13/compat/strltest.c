#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
	char a[12];
	strlcpy(a, "666", 3);
	strlcat(a, "6", sizeof(a));
	return 0;
}
