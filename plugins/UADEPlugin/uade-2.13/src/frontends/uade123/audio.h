#ifndef _UADE123_AUDIO_H_
#define _UADE123_AUDIO_H_

#include <uadeconfstructure.h>

void audio_close(void);
int audio_init(const struct uade_config *uc);
int audio_play(unsigned char *samples, int bytes);

#endif
