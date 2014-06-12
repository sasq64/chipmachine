
#include "SongInfo.h"

#include <musicplayer/common/fifo.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

#ifdef RASPBERRYPI
#define AUDIO_DELAY 12
#else
#define AUDIO_DELAY 20
#endif

class MusicPlayer {
public:
	MusicPlayer();
	void playFile(const std::string &fileName);
	bool playing() { return player != nullptr; }
	void stop() { std::lock_guard<std::mutex> guard(m); player = nullptr; }
	uint32_t getPosition() { return pos/44100; };
	uint32_t getLength() { return length; }

	void pause(bool dopause = true);

	bool isPaused() {
		return paused;
	}

	void seek(int song, int seconds = -1);

	SongInfo getPlayingInfo();

	uint16_t *getSpectrum() {
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

	int spectrumSize() { return fft.eq_slots; }

private:
	Fifo fifo;
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);

	//Fifo fifo;
	SpectrumAnalyzer fft;
	std::vector<ChipPlugin*> plugins;
	bool paused = false;
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;
	//std::string toPlay;
	std::mutex m;
	std::shared_ptr<ChipPlayer> player;
	int pos;
	int length;
	int fadeOut;
	bool changedSong = false;
};

}