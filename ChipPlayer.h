#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include <string>

class ChipPlayer {
public:
	virtual int getSamples(short *target, int size) = 0;
	virtual std::string getMetaData(const std::string &what) { return ""; }
};

#endif // CHIP_PLAYER_H