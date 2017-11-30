#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "SongInfo.h"

#include <coreutils/fifo.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

class MusicPlayer {
public:
	MusicPlayer(const std::string &workDir);
	~MusicPlayer();
	bool playFile(const std::string &fileName);
	bool streamFile(const std::string &fileName);
	bool playing() { return !playEnded && player != nullptr; }
	void stop() { player = nullptr; }
	uint32_t getPosition() { return pos / 44100; };
	uint32_t getLength() { return length; }

	void putStream(const uint8_t* ptr, int size);
	void clearStreamFifo() {
		LOGD("Clearing stream fifo");
		streamFifo.clear();
	}

	void setParameter(const std::string &what, int v);

	// Asks the plugin if the given file requires secondary files.
	// Can be called several times, normally first with non-existing
	// file, and later with the loaded file
	std::vector<std::string> getSecondaryFiles(const std::string &name);

	void pause(bool dopause = true);

	bool isPaused() { return paused; }

	void seek(int song, int seconds = -1);

	int getTune() { return currentTune; }

	SongInfo getPlayingInfo() {
		return playingInfo;
	}

	uint16_t *getSpectrum();

	std::string getMeta(const std::string &what);

	int spectrumSize() {
		return fft.eq_slots;
	}

	// Returns silence (from now) in seconds
	int getSilence();

	void setVolume(float v);
	float getVolume() const;

	// Fadeout music
	void fadeOut(float secs);
	float getFadeVolume() { return fifo.getVolume(); }

	void update();
	
private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	//std::shared_ptr<ChipPlayer> fromStream(const std::string &fileName);
	void updatePlayingInfo();

	utils::AudioFifo<int16_t> fifo;
	SongInfo playingInfo;
	// Fifo fifo;
	SpectrumAnalyzer fft;
	//std::vector<std::shared_ptr<ChipPlugin>> plugins;
	std::atomic<bool> paused{false};
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;

	// Should be held when accessing FFT data
	std::shared_ptr<ChipPlayer> player;
	std::string message;
	std::string sub_title;
	std::atomic<int> pos{0};
	std::atomic<int> length{0};
    int fadeLength = 0;
    int fadeOutPos = 0;
    int silentFrames = 0;
    int currentTune = 0;
    float volume = 1.0f;
	// bool changedSong = false;
	std::atomic<bool> dontPlay;
	std::atomic<bool> playEnded;
	bool checkSilence = true;

	utils::Fifo<uint8_t> streamFifo;
};
}

#endif // MUSIC_PLAYER_H
