#ifndef PLAYER_H
#define PLAYER_H

#include "SharedState.h"
#include "SongState.h"
#include "PlayerFactory.h"
#include "PlayerInterface.h"
#include "ChipPlayer.h"

#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class Player : public PlayerInterface {
public:
	Player();
	void run();
	void runThread();

	virtual std::queue<std::string> &getPlayList() {
		playMutex.lock();
		return playQueue;
	}

	virtual void releasePlayList() {
		playMutex.unlock();
	}

	virtual int sendCommand(Command cmd) {
		return 0;
	}

	virtual const PlayState& getPlayState() {
		return playState;
	}
	virtual const SongState& getSongState() {
		return songState;
	}


private:
	PlayerFactory *factory;
	AudioPlayerNative ap;
	std::unique_ptr<ChipPlayer> player;
	std::vector<int16_t> buffer;
	int oldSeconds;
	int frameCount;
	std::queue<std::string> playQueue;
	std::mutex playMutex;
	std::thread mainThread;
	PlayState playState;
	SongState songState;

	SharedState &globalState;
};

#endif // PLAYER_H