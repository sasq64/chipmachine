
#include "AudioPlayer.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

static void CALLBACK AudioPlayer::waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

	if(uMsg != WOM_DONE)
		return;

	AudioPlayerWindows *ap = (AudioPlayerWindows*)dwInstance;
	EnterCriticalSection(&ap->lock);
	ap->blockCounter--;
	LeaveCriticalSection(&ap->lock);
}

AudioPlayer::AudioPlayer() : blockCounter(0), blockPosition(0), bufSize(32768), bufCount(4) {

		WAVEFORMATEX wfx;
		//MMRESULT result;
		wfx.nSamplesPerSec = 44100;
		wfx.wBitsPerSample = 16;
		wfx.nChannels = 2;
		wfx.cbSize = 0; /* size of _extra_ info */
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

		if(waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD)AudioPlayer::waveOutProc, (DWORD)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
			fprintf(stderr, "unable to open WAVE_MAPPER device\n");
			ExitProcess(1);
		}

		InitializeCriticalSection(&lock);

		header.resize(bufCount);
		buffer.resize(bufCount);

		for(int i=0; i<bufCount; i++) {
			buffer[i].resize(bufSize);
			ZeroMemory(&header[i], sizeof(WAVEHDR));
			header[i].lpData = (LPSTR) &buffer[i][0];
		}

	}

AudioPlayer::~AudioPlayer() {
	waveOutClose(hWaveOut);
}

void AudioPlayer::writeAudio(int16_t *samples, int sampleCount) {

		//printf("Writing block %d\n", blockPosition);

		WAVEHDR &h = header[blockPosition];
		h.dwBufferLength = sampleCount * 2;
		memcpy(h.lpData, samples, sampleCount * 2);

		if(h.dwFlags & WHDR_PREPARED) 
			waveOutUnprepareHeader(hWaveOut, &h, sizeof(WAVEHDR));

		waveOutPrepareHeader(hWaveOut, &h, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, &h, sizeof(WAVEHDR));

		EnterCriticalSection(&lock);
		blockCounter++;
		LeaveCriticalSection(&lock);

		blockPosition++;
		blockPosition %= bufCount;

		while(true) {
			EnterCriticalSection(&lock);
			int bc = blockCounter;
			LeaveCriticalSection(&lock);
			if(bc < bufCount)
				break;
			Sleep(100);
		}
	}



#elif defined LINUX

#include <linux/soundcard.h>
#include <alsa/asoundlib.h>

AudioPlayer::AudioPlayer() {
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

void AudioPlayer::writeAudio(int16_t *samples, int sampleCount) {
	int frames = snd_pcm_writei(playback_handle, (char*)samples, sampleCount/2);
	if (frames < 0) {
		snd_pcm_recover(playback_handle, frames, 0);
	}
}
#else
#error "Unsupported";
#endif