#ifndef _UADE_MEMMEM_H_
#define _UADE_MEMMEM_H_

#include <stdio.h>

void *memmem(const void *haystack, size_t haystacklen,
	     const void *needle, size_t needlelen);

#endif

