#ifndef AUDIOPLAYER_LINUX_H
#define AUDIOPLAYER_LINUX_H

#include "AudioPlayer.h"

#include <linux/soundcard.h>
#include <alsa/asoundlib.h>

#include <vector>


class AudioPlayerLinux : public AudioPlayer {
public:
	AudioPlayerLinux();
	void writeAudio(short *samples, int sampleCount);
private:
	int dspFD;
	snd_pcm_t *playback_handle;
};

typedef AudioPlayerLinux AudioPlayerNative;

#endif // AUDIOPLAYER_LINUX_H