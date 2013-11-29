#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#elif defined LINUX
#include <linux/soundcard.h>
#include <alsa/asoundlib.h>
#endif
#include <vector>
#include <stdint.h>


class AudioPlayer {
public:
	AudioPlayer();
	virtual void writeAudio(int16_t *samples, int sampleCount);

#ifdef WIN32
	CRITICAL_SECTION lock;
	volatile int blockCounter;
	volatile int blockPosition;

	HWAVEOUT hWaveOut;

	int bufSize;
	int bufCount;

	std::vector<std::vector<int16_t>> buffer;
	std::vector<WAVEHDR> header;
	static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

#elif defined LINUX
	int dspFD;
	snd_pcm_t *playback_handle;

#endif

};

#endif // AUDIOPLAYER_H