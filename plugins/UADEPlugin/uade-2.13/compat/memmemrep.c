#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ossupport.h"

/* Slow implementation */
void *memmem(const void *haystack, size_t haystacklen,
	     const void *needle, size_t needlelen)
{
  size_t offs;
  unsigned char *s = (unsigned char *) haystack;

  if (needlelen == 0)
    return (void *) haystack;

  for (offs = 0; (offs + needlelen) <= haystacklen; offs++) {
    if (memcmp(s + offs, needle, needlelen) == 0)
      return s + offs;
  }

  return NULL;
}
