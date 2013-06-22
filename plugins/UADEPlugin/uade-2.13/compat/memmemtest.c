#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
  char b = 1, c = 1;
  return memmem(&b, 1, &c, 1) == NULL ? 1 : 0;
}
