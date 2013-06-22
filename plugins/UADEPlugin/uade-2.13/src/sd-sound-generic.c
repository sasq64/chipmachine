/* 
 * UADE
 * 
 * Support for sound
 * 
 * Copyright 2000 - 2005 Heikki Orsila <heikki.orsila@iki.fi>
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "gensound.h"
#include "sd-sound.h"
#include "audio.h"
#include "uade.h"

uae_u16 sndbuffer[MAX_SOUND_BUF_SIZE / 2];
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_second;

void close_sound (void)
{
}

/* Try to determine whether sound is available.  */
int setup_sound (void)
{
   sound_available = 1;
   return 1;
}


void set_sound_freq(int x)
{
  /* Validation is done later in init_sound() */
  currprefs.sound_freq = x;
  init_sound();
}


void init_sound (void)
{
  int channels;
  int dspbits;
  unsigned int rate;
  
  if (currprefs.sound_maxbsiz < 128 || currprefs.sound_maxbsiz > 16384) {
    fprintf (stderr, "Sound buffer size %d out of range.\n", currprefs.sound_maxbsiz);
    currprefs.sound_maxbsiz = 8192;
  }
  sndbufsize = 8192;
  
  dspbits = currprefs.sound_bits;
  rate    = currprefs.sound_freq;
  channels = currprefs.stereo ? 2 : 1;

  if (dspbits != (UADE_BYTES_PER_SAMPLE * 8)) {
    fprintf(stderr, "Only 16 bit sounds supported.\n");
    exit(-1);
  }
  if (rate < 1 || rate > SOUNDTICKS_NTSC) {
    fprintf(stderr, "Too small or high a rate: %u\n", rate);
    exit(-1);
  }
  if (channels != UADE_CHANNELS) {
    fprintf(stderr, "Only stereo supported.\n");
    exit(-1);
  }

  sound_bytes_per_second = (dspbits / 8) *  channels * rate;

  audio_set_rate(rate);

  sound_available = 1;
  
  sndbufpt = sndbuffer;
}

/* this should be called between subsongs when remote slave changes subsong */
void flush_sound (void)
{
  sndbufpt = sndbuffer;
}
