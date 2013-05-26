#ifndef VICE_PLAYER_H
#define VICE_PLAYER_H

#include "ChipPlayer.h"
#include "ChipPlugin.h"
#include "utils.h"

#include <string>

class VicePlayer : public ChipPlayer {
public:
	static bool init(const std::string &c64Dir);
	VicePlayer(const std::string &sidFile);
	virtual int getSamples(short *target, int size);
};


class SidPlugin : public ChipPlugin {
public:
	SidPlugin() {
		VicePlayer::init("c64");
	}

	virtual bool canHandle(const std::string &name) override {
		return utils::endsWith(name, ".sid");
	}

	virtual ChipPlayer *fromFile(utils::File &file) override {
		return new VicePlayer { file.getName() };
	}
};

#endif // VICE_PLAYER_H