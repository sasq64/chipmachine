
#include "ChipPlayer.h"

#include <memory>

namespace chipmachine {

class ChipPlugin;

class DelegatingChipPlayer : public ChipPlayer {
public:
	DelegatingChipPlayer(chipmachine::ChipPlayer *cp) : player(cp) {}
	~DelegatingChipPlayer() {}

	int getSamples(int16_t *target, int size) override {
		return player->getSamples(target, size);
	}

	virtual std::string getMeta(const std::string &what) override { 
		return player->getMeta(what);
	};

	int getMetaInt(const std::string &what) override {
		return player->getMetaInt(what);
	}

private:
	std::shared_ptr<ChipPlayer> player;
};

class MusicPlayer {
public:
	MusicPlayer();
	DelegatingChipPlayer fromFile(const std::string &fileName);
private:
	std::vector<ChipPlugin*> plugins;
};

}