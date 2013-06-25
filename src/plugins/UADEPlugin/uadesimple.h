#ifndef _UADE123_H_
#define _UADE123_H_

#include <limits.h>
#include <stdio.h>

#include <effects.h>


#define debug(verbose, fmt, args...) if (verbose) { fprintf(stderr, fmt, ## args); }
#define tprintf(fmt, args...) do {fprintf(stdout, fmt, ## args); } while (0)

extern int uade_song_end_trigger;

#endif
