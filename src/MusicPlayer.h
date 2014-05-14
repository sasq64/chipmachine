
//#include "ChipPlayer.h"
#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

class MusicPlayer {
public:
	MusicPlayer(SpectrumAnalyzer &fft);
	void playFile(const std::string &fileName);
	bool playing() { return player != nullptr; }
	void stop() { player = nullptr; }
	uint32_t getPosition() { return pos/44100; };
	uint32_t getLength() { return length; }
private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	SpectrumAnalyzer &fft;
	std::vector<ChipPlugin*> plugins;
	std::string toPlay;
	std::mutex m;
	std::shared_ptr<ChipPlayer> player;
	int pos;
	int length;
};

}