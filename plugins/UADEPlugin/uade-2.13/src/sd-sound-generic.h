/* 
 * UADE sound output
 * 
 * Copyright 1997 Bernd Schmidt
 * Copyright 2000-2005 Heikki Orsila <heikki.orsila@iki.fi>
 */

#include <assert.h>
#include <stdint.h>

#include <errno.h>
#include <string.h>

#include "uade.h"
#include "uadeconstants.h"

#define MAX_SOUND_BUF_SIZE (65536)

extern uae_u16 sndbuffer[];
extern uae_u16 *sndbufpt;
extern int sndbufsize;
extern int sound_bytes_per_second;

extern void finish_sound_buffer (void);

#define DEFAULT_SOUND_MAXB 8192
#define DEFAULT_SOUND_MINB 8192
#define DEFAULT_SOUND_BITS (8 * UADE_BYTES_PER_SAMPLE)
#define DEFAULT_SOUND_FREQ UADE_DEFAULT_FREQUENCY
