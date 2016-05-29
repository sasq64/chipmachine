#ifndef MUSIC_PLAYER_LIST_H
#define MUSIC_PLAYER_LIST_H

#include "SongInfo.h"
#include "MusicPlayer.h"
#ifdef USE_REMOTELISTS
#include "RemoteLists.h"
#endif
#include "RemoteLoader.h"
#include "MusicDatabase.h"
#include "CueSheet.h"

#include <coreutils/thread.h>
#include <cstdint>
#include <deque>

#define SET_STATE(x) (LOGD("STATE: " #x), state = x)

namespace chipmachine {

#ifdef _WIN32
#undef ERROR
#endif

/*

Thread model

* Commands in through queued callbacks
* State out needs to be mutexed on writing and reading

  
  
*/


class MusicPlayerList {
public:
	enum State {
		STOPPED,
		ERROR,
		WAITING,
		LOADING,
		STARTED,
		PLAY_STARTED,
		PLAYING,
		FADING,
		PLAY_NOW,
		PLAY_MULTI
	};

	MusicPlayerList(const std::string &workDir);

	~MusicPlayerList() {
		quitThread = true;
		playerThread.join();
	}

	void addSong(const SongInfo &si, bool shuffle = false); 
	void playSong(const SongInfo &si);
	void clearSongs();
	void nextSong();

	uint16_t *getSpectrum();
	int spectrumSize();
	SongInfo getInfo(int index = 0);
	SongInfo getDBInfo();
	int getLength();
	int getPosition();
	int listSize();

	bool playing() { return mp.playing(); }

	int getTune() {
		if(multiSongs.size())
			return multiSongNo;
		return mp.getTune();
	}

	void pause(bool dopause = true) {
		if(!(permissions & CAN_PAUSE))
			return;
		mp.pause(dopause);
	}
	bool isPaused() { return mp.isPaused(); }

	void seek(int song, int seconds = -1);

	std::string getMeta(const std::string &what) {
		if(what == "sub_title" && cueTitle != "")
			return cueTitle;
		return mp.getMeta(what);
	}

	State getState() {
		// LOCK_GUARD(plMutex);
		State rc = state;
		if(rc == PLAY_STARTED)
			SET_STATE(PLAYING);
		return rc;
	}

	bool getAllowed() {
		bool rc = wasAllowed;
		wasAllowed = true;
		return rc;
	}

	bool hasError() { return errors.size() > 0; }

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

	void setPermissions(uint32_t p) { permissions = p; }

	void setPartyMode(bool on, int lockSeconds = 60, int graceSec = 3);

	void setReportSongs(bool on) { reportSongs = on; }

	void setVolume(float volume) { mp.setVolume(volume); }

	float getVolume() const { return mp.getVolume(); }

	void stop() {
		SET_STATE(STOPPED);
		mp.stop();
	}

	bool wasFromQueue() const { return playedNext; }
	
	const std::vector<utils::File> &getSongFiles() const { return songFiles; }

private:
	
	void onThisThread(std::function<void()> f) {
		LOCK_GUARD(plMutex);
		funcs.push_back(f);
	}
	
	std::vector<std::function<void()>> funcs;
	
	
	bool handlePlaylist(const std::string &fileName);
	void playCurrent();
	bool playFile(const std::string &fileName);

	void update();
	void updateInfo();

	bool checkPermission(int flags);

	std::deque<std::string> errors;

	MusicPlayer mp;
	std::mutex plMutex;
	
	struct PlayQueue {
		std::deque<SongInfo> songs;
		std::deque<SongInfo> psongs;
		std::string prodScreenshot;
		int size() { return songs.size() + psongs.size(); }
		void push_back(const SongInfo &s) { songs.push_back(s); }
		//void push_font(const SongInfo &s) { songs.push_front(s); }
		void clear() { psongs.clear(); songs.clear(); }
		void pop_front() { 
			if(psongs.size() > 0)
				psongs.pop_front();
			else
				songs.pop_front(); 
		}
		SongInfo& front() {
			if(psongs.size() > 0)
				return psongs.front();
			return songs.front(); 
		}
		SongInfo& getSong(int i) { 
			if(i < psongs.size())
				return psongs[i];
			return songs[i - psongs.size()]; 
		}
		void insertAt(int i, const SongInfo &s) { songs.insert( songs.begin() + i, s); }
	};
	
	PlayQueue playList;

	std::atomic<bool> wasAllowed;
	std::atomic<bool> quitThread;

	std::atomic<int> files;
	std::string loadedFile;

	std::atomic<State> state; // = STOPPED;
	SongInfo currentInfo;
	SongInfo dbInfo;

	std::thread playerThread;

	bool changedSong = false;

	bool reportSongs = true;

	std::atomic<uint32_t> permissions;

	bool partyMode = false;
	bool partyLockDown = false;
	int graceSeconds = 3;
	int lockSeconds = 60;

	bool detectSilence = true;

	std::shared_ptr<CueSheet> cueSheet;
	std::string cueTitle;

	int multiSongNo;
	std::vector<std::string> multiSongs;
	//std::deque<SongInfo> productSongs;
	bool changedMulti = false;
	// RemoteLists &tracker;
	bool playedNext;
	
	std::vector<utils::File> songFiles;
	
	//std::string screenshot;
	
};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H
