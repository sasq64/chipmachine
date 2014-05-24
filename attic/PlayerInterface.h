#ifndef PLAYER_INTERFACE_H
#define PLAYER_INTERFACE_H

#include "SongState.h"

#include <string>
#include <queue>

class PlayerInterface {
public:
	enum Command {
		PLAY,
		PAUSE,
		STOP,
		NEXT_SONG,
		PREV_SONG,
		NEXT_SUBTUNE,
		PREV_SUBTUNE,
		SHUFFLE_ON,
		SHUFFLE_OFF,
		PING
	};

	virtual std::queue<std::string> &getPlayList() = 0;
	virtual void releasePlayList() = 0;
	virtual int sendCommand(Command cmd) = 0;

	virtual const PlayState& getPlayState() = 0;
	virtual const SongState& getSongState() = 0;

};

#endif // PLAYER_INTERFACE_H