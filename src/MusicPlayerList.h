#ifndef MUSIC_PLAYER_LIST_H
#define MUSIC_PLAYER_LIST_H

#include "SongInfo.h"
#include "MusicPlayer.h"

#include <webutils/webgetter.h>

#include <mutex>
#include <cstdint>
#include <deque>

namespace chipmachine {

class MusicPlayerList {
public:
	enum State {
		STOPPED,
		WAITING,
		LOADING,
		STARTED,
		PLAY_STARTED,
		PLAYING,
		FADING
	};

	MusicPlayerList();

	void addSong(const SongInfo &si);
	void clearSongs();
	void nextSong();
	
	uint16_t *getSpectrum();
	int spectrumSize();
	SongInfo getInfo(int index = 0);
	int getLength();
	int getPosition();
	int listSize();

	bool playing() { return mp.playing(); }
	void pause(bool dopause = true) { 
		if(!(permissions & CAN_PAUSE))
			return;
		mp.pause(dopause);
	}
	bool isPaused() { return mp.isPaused(); }

	void seek(int song, int seconds = -1);

	std::string getMeta(const std::string &what) {
		return mp.getMeta(what);
	}

	State getState() {
		State rc = state;
		if(rc == PLAY_STARTED)
			state = PLAYING;
		return rc;
	}

	enum {
		CAN_SWITCH_SONG = 1,
		CAN_SEEK = 2,
		CAN_PAUSE = 4,
		CAN_ADD_SONG = 8,
		CAN_CLEAR_SONGS = 16
	};


private:
	void playFile(const std::string &fileName);

	void update();
	void updateInfo();

	MusicPlayer mp;
	std::mutex plMutex;
	std::deque<SongInfo> playList;

	std::atomic<int> files;
	std::string loadedFile;

	WebGetter webgetter { "_files" };

	std::atomic<State> state;// = STOPPED;
	SongInfo currentInfo;

	std::thread playerThread;

	bool changedSong = false;

	uint32_t permissions = 0xffff;

};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H

