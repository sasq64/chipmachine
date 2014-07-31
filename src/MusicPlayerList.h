#ifndef MUSIC_PLAYER_LIST_H
#define MUSIC_PLAYER_LIST_H

#include "SongInfo.h"
#include "MusicPlayer.h"
#include "PlayTracker.h"

#include <webutils/webgetter.h>

#include <mutex>
#include <cstdint>
#include <deque>

namespace chipmachine {

class ChipMachine;

class MusicPlayerList {
public:
	enum State {
		STOPPED,
		WAITING,
		LOADING,
		STARTED,
		PLAY_STARTED,
		PLAYING,
		FADING,
		PLAY_NOW
	};

	MusicPlayerList();

	~MusicPlayerList() {
		quitThread = true;
		playerThread.join();
	}

	bool addSong(const SongInfo &si, int pos = -1);
	void playSong(const SongInfo &si);
	void clearSongs();
	void nextSong();
	
	uint16_t *getSpectrum();
	int spectrumSize();
	SongInfo getInfo(int index = 0);
	int getLength();
	int getPosition();
	int listSize();

	bool playing() { return mp.playing(); }

	int getTune() { return mp.getTune(); }

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

	bool getAllowed() {
		bool rc = wasAllowed;
		wasAllowed = true;
		return rc;
	}

	bool hasError() {
		return errors.size() > 0;
	}

	std::string getError() {
		LOCK_GUARD(plMutex);
		auto e = errors.front();
		errors.pop_front();
		return e;
	}

	enum {
		CAN_SWITCH_SONG = 1,
		CAN_SEEK = 2,
		CAN_PAUSE = 4,
		CAN_ADD_SONG = 8,
		CAN_CLEAR_SONGS = 16,
		PARTYMODE = 0x10000000
	};

	uint32_t getPermissions() { return permissions; }

	void setPermissions(uint32_t p) {
		permissions = p;
	}

	void setPartyMode(bool on, int lockSeconds = 60, int graceSec = 3);

private:
	void playCurrent();
	bool playFile(const std::string &fileName);

	void update();
	void updateInfo();

	bool checkPermission(int flags);

	std::deque<std::string> errors;

	MusicPlayer mp;
	std::mutex plMutex;
	std::deque<SongInfo> playList;

	std::atomic<bool> wasAllowed;
	std::atomic<bool> quitThread;

	std::atomic<int> files;
	std::string loadedFile;

	WebGetter webgetter;

	std::atomic<State> state;// = STOPPED;
	SongInfo currentInfo;

	std::thread playerThread;

	bool changedSong = false;

	std::atomic<uint32_t> permissions;

	bool partyMode = false;
	bool partyLockDown = false;
	int graceSeconds = 3;
	int lockSeconds = 60;

	//PlayTracker &tracker;

};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H

