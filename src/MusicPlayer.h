
#include "SongInfo.h"

#include <musicplayer/common/fifo.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGE("WAITING FOR LOCK"); std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

// MUST BE THREAD SAFE
class MusicPlayer {
public:
	MusicPlayer();
	~MusicPlayer();
	bool playFile(const std::string &fileName);
	bool playing() { return !playEnded && player != nullptr; }
	void stop() { LOCK_GUARD(playerMutex); player = nullptr; }
	uint32_t getPosition() { return pos/44100; };
	uint32_t getLength() { return length; }

	void pause(bool dopause = true);

	bool isPaused() {
		return paused;
	}

	void seek(int song, int seconds = -1);

	int getTune() { return currentTune; }

	SongInfo getPlayingInfo() {
		LOCK_GUARD(infoMutex);
		return playingInfo;
	}

	uint16_t *getSpectrum();

	std::string getMeta(const std::string &what);

	int spectrumSize() { LOCK_GUARD(fftMutex); return fft.eq_slots; }

	// Returns silence (from now) in seconds
	int getSilence();

	// Fadeout music
	void fadeOut(float secs);
	float getVolume() {
		return fifo.getVolume();
	}

private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	void updatePlayingInfo();

	Fifo fifo;
	SongInfo playingInfo;
	//Fifo fifo;
	SpectrumAnalyzer fft;
	std::vector<std::shared_ptr<ChipPlugin>> plugins;
	bool paused = false;
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;
	//std::string toPlay;
	std::mutex playerMutex;
	std::mutex fftMutex;
	std::mutex infoMutex;
	std::shared_ptr<ChipPlayer> player;
	std::string message;
	std::string sub_title;
	int pos;
	int length;
	int fadeLength;
	int fadeOutPos;
	int silentFrames;
	int currentTune;
	//bool changedSong = false;
	std::atomic<bool> dontPlay;
	std::atomic<bool> playEnded;
};

}