#ifndef _UADESIMPLE_AUDIO_H_
#define _UADESIMPLE_AUDIO_H_

void audio_close(void);
int audio_init(int frequency);
int audio_play(char *samples, int bytes);

#endif
