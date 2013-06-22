#include <limits.h>
#include <stdlib.h>

char *canonicalize_file_name(const char *path)
{
	char *s = malloc(PATH_MAX);
	if (s == NULL)
		return NULL;

	if (realpath(path, s) == NULL) {
		free(s);
		return NULL;
	}

	return s;
}

