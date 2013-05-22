#ifndef AUDIOPLAYER_WINDOWS_H
#define AUDIOPLAYER_WINDOWS_H

#include "AudioPlayer.h"
#include <windows.h>
#include <vector>
//#include <winmm.h>

class AudioPlayerWindows : public AudioPlayer {
public:
	AudioPlayerWindows();
	~AudioPlayerWindows();
	void writeAudio(short *samples, int sampleCount) override;
private:
	CRITICAL_SECTION lock;
	volatile int blockCounter;
	volatile int blockPosition;

	HWAVEOUT hWaveOut;

	int bufSize;
	int bufCount;

	std::vector<std::vector<short>> buffer;
	std::vector<WAVEHDR> header;

	static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {


		if(uMsg != WOM_DONE)
			return;

		AudioPlayerWindows *ap = (AudioPlayerWindows*)dwInstance;
		EnterCriticalSection(&ap->lock);
		ap->blockCounter--;
		LeaveCriticalSection(&ap->lock);
	}

};

#endif // AUDIOPLAYER_WINDOWS_H