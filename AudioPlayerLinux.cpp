
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

	int i;
	int err;
	short buf[128];
	snd_pcm_hw_params_t *hw_params;

	if((err = snd_pcm_open_preferred(&playback_handle, argv[1], nullptr, nullptr, SND_PCM_OPEN_PLAYBACK)) < 0) {
		fprintf(stderr, "cannot open audio device (%s)\n", snd_strerror(err));
		exit(1);
	}
	   
	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
		exit (1);
	}
			 
	if((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		exit (1);
	}

	if((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror (err));
		exit (1);
	}

	if((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror (err));
		exit (1);
	}

	if((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, 44100, 0)) < 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
		exit (1);
	}

	if((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2)) < 0) {
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror (err));
		exit (1);
	}

	if((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror (err));
		exit (1);
	}

	snd_pcm_hw_params_free(hw_params);

	if((err = snd_pcm_prepare(playback_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		exit(1);
	}
	

}

void AudioPlayerLinux::writeAudio(short *samples, int sampleCount) {
	//int status = write(dspFD, samples, sampleCount*2);
	snd_pcm_writei(playback_handle, (char*)samples, sampleCount*2);
}
