#ifndef VICE_PLAYER_H
#define VICE_PLAYER_H

#include <string>
#include "ChipPlayer.h"

class VicePlayer : public ChipPlayer {
public:
	static bool init(const std::string &c64Dir);
	VicePlayer(const std::string &sidFile);
	virtual int getSamples(short *target, int size);
};

#endif // VICE_PLAYER_H