#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "SongInfo.h"

#include <coreutils/fifo.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <coreutils/thread.h>

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGE("WAITING FOR LOCK"); \
//std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

class Streamer {
public:
	Streamer(std::mutex &m, std::shared_ptr<ChipPlayer> pl) : playerMutex(m), player(pl) {}
	void put(const uint8_t *ptr, int size);

private:
	std::mutex &playerMutex;
	std::shared_ptr<ChipPlayer> player;
};

// MUST BE THREAD SAFE
class MusicPlayer {
public:
	MusicPlayer(const std::string &workDir);
	~MusicPlayer();
	bool playFile(const std::string &fileName);
	std::shared_ptr<Streamer> streamFile(const std::string &fileName);
	bool playing() { return !playEnded && player != nullptr; }
	void stop() {
		LOCK_GUARD(playerMutex);
		player = nullptr;
	}
	uint32_t getPosition() { return pos / 44100; };
	uint32_t getLength() { return length; }

	// void addStreamData(uint8_t *ptr, int size);

	// Asks the plugin if the given file requires secondary files.
	// Can be called several times, normally first with non-existing
	// file, and later with the loaded file
	std::vector<std::string> getSecondaryFiles(const std::string &name);

	void pause(bool dopause = true);

	bool isPaused() { return paused; }

	void seek(int song, int seconds = -1);

	int getTune() { return currentTune; }

	SongInfo getPlayingInfo() {
		LOCK_GUARD(infoMutex);
		return playingInfo;
	}

	uint16_t *getSpectrum();

	std::string getMeta(const std::string &what);

	int spectrumSize() {
		LOCK_GUARD(fftMutex);
		return fft.eq_slots;
	}

	// Returns silence (from now) in seconds
	int getSilence();

	void setVolume(float v);
	float getVolume();

	// Fadeout music
	void fadeOut(float secs);
	float getFadeVolume() { return fifo.getVolume(); }

	void update();

private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	std::shared_ptr<ChipPlayer> fromStream(const std::string &fileName);
	void updatePlayingInfo();

	AudioFifo<int16_t> fifo;
	SongInfo playingInfo;
	// Fifo fifo;
	SpectrumAnalyzer fft;
	std::vector<std::shared_ptr<ChipPlugin>> plugins;
	bool paused = false;
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;
	// std::string toPlay;
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
	float volume;
	// bool changedSong = false;
	std::atomic<bool> dontPlay;
	std::atomic<bool> playEnded;
};
}

#endif // MUSIC_PLAYER_H
