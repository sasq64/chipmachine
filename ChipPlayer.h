#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include <string>
#include <stdint.h>
#include <stdio.h>

class ChipPlayer {
public:
	virtual ~ChipPlayer() {}
	virtual int getSamples(int16_t *target, int size) = 0;
	virtual std::string getMetaData(const std::string &what) { return ""; }
	virtual void seekTo(int song, int seconds = -1) { printf("NOT IMPLEMENTED\n"); }
};

#endif // CHIP_PLAYER_H