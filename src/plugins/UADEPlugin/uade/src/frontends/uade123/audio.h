#ifndef _UADE123_AUDIO_H_
#define _UADE123_AUDIO_H_

#include <uade/uade.h>

void audio_close(void);
int audio_init(const struct uade_state *us, char **opts);
int audio_play(char *samples, int bytes);

#endif
