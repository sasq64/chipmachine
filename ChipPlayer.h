#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include <string>
#include <stdint.h>

class ChipPlayer {
public:
	virtual ~ChipPlayer() {}
	virtual int getSamples(int16_t *target, int size) = 0;
	virtual std::string getMetaData(const std::string &what) { return ""; }
	virtual void seekTo(int song, int seconds) {}
};

#endif // CHIP_PLAYER_H