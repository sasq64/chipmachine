
#include "AudioPlayerLinux.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#include <linux/soundcard.h>
#include <alsa/asoundlib.h>

AudioPlayerLinux::AudioPlayerLinux() {
	int err;
	if((err = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "cannot open audio device (%s)\n", snd_strerror(err));
		exit(1);
	}
	if((err = snd_pcm_set_params(playback_handle,
	          SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1, 500000)) < 0) {
		fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
		exit(1);
	}
}

void AudioPlayerLinux::writeAudio(short *samples, int sampleCount) {
	int frames = snd_pcm_writei(playback_handle, (char*)samples, sampleCount/2);
	if (frames < 0) {
		snd_pcm_recover(playback_handle, frames, 0);
	}
}
