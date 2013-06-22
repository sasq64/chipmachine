#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "custom.h"

#include "text_scope.h"

static char *PaulaEventStr[] = {"Vol", "Per", "Dat", "Len", "LCL", "LCH"};


static void print_tab(int v)
{
    int i;
    char spaces[20];

    memset(spaces, ' ', sizeof spaces);

    for (i = 0; i < v; i++)
	fwrite(spaces, sizeof spaces, 1, stdout);
}


void text_scope(unsigned long cycles, int voice, enum PaulaEventType e,
		int value)
{
    unsigned int sec = cycles / SOUNDTICKS;
    unsigned int usec = ((uint64_t) (cycles % SOUNDTICKS) * 1000000) / SOUNDTICKS;

    printf("%6u:%.6u ", sec & 0x1ffff, usec);
    print_tab(voice);
    printf("%s %5d %d\n", PaulaEventStr[e], value, voice);
}
