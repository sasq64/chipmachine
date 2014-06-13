
#include "SongInfo.h"

#include <musicplayer/common/fifo.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGD("WAITING FOR LOCK"); std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

#ifdef RASPBERRYPI
#define AUDIO_DELAY 12
#else
#define AUDIO_DELAY 20
#endif

// MUST BE THREAD SAFE
class MusicPlayer {
public:
	MusicPlayer();
	void playFile(const std::string &fileName);
	bool playing() { return player != nullptr; }
	void stop() { LOCK_GUARD(playerMutex); player = nullptr; }
	uint32_t getPosition() { return pos/44100; };
	uint32_t getLength() { return playingInfo.length; }

	void pause(bool dopause = true);

	bool isPaused() {
		return paused;
	}

	void seek(int song, int seconds = -1);

	SongInfo getPlayingInfo() {
		LOCK_GUARD(infoMutex);
		return playingInfo;
	}

	uint16_t *getSpectrum() {
		LOCK_GUARD(fftMutex);
		if(fft.size() > AUDIO_DELAY) {
			while(fft.size() > AUDIO_DELAY*2)
				fft.popLevels();
			//LOGD("GET");
			spectrum = fft.getLevels();
			fft.popLevels();

		} //else LOGD("WAIT");
		return &spectrum[0];

	}

	std::string getMeta(const std::string &what);

	int spectrumSize() { LOCK_GUARD(fftMutex); return fft.eq_slots; }

private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	void updatePlayingInfo();

	Fifo fifo;
	SongInfo playingInfo;
	//Fifo fifo;
	SpectrumAnalyzer fft;
	std::vector<ChipPlugin*> plugins;
	bool paused = false;
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;
	//std::string toPlay;
	std::mutex playerMutex;
	std::mutex fftMutex;
	std::mutex infoMutex;
	std::shared_ptr<ChipPlayer> player;
	std::string message;
	int pos;
	//int length;
	int fadeOut;
	bool changedSong = false;
	std::atomic<bool> dontPlay;
};

}