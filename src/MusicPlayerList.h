#ifndef MUSIC_PLAYER_LIST_H
#define MUSIC_PLAYER_LIST_H

#include "SongInfo.h"
//#include "MusicPlayer.h"
#ifdef USE_REMOTELISTS
#include "RemoteLists.h"
#endif
#include "RemoteLoader.h"
#include "MusicDatabase.h"
#include "CueSheet.h"
#include <coreutils/fifo.h>
#include <coreutils/thread.h>
#include <cstdint>
#include <deque>

#define SET_STATE(x) (LOGD("STATE: " #x), state = x)

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGE("WAITING FOR LOCK"); \
//std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace chipmachine {

#ifdef _WIN32
#undef ERROR
#endif

/*

Thread model

* Commands in through queued callbacks
* State out needs to be mutexed on writing and reading
*/

class MusicPlayer;

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

	bool updateSong = false;

	// Copies that can be read from outside thread
	struct Shared {
		std::atomic<bool> playing;
		std::atomic<int> tune;
		std::atomic<bool> paused;
		std::atomic<float> volume;
		std::atomic<uint32_t> position;
		std::atomic<int32_t> length;
		std::atomic<int> playlist_size;

		// Mutexed before reading/writing
		SongInfo info[3];
		std::vector<utils::File> songFiles;
		std::string sub_title;
		std::string message;
	};
	Shared shared;

	bool playing() { return shared.playing; } //mp.playing(); }

	int getTune() {
		return shared.tune;
		//if(multiSongs.size())
		//	return multiSongNo;
		//return mp.getTune();
	}

	void pause(bool dopause = true);
	bool isPaused() { return shared.paused; } //mp.isPaused(); }

	void seek(int song, int seconds = -1);

	std::string getMeta(const std::string &what) {
		LOCK_GUARD(plMutex);
		if(what == "sub_title") {
			if(cueTitle != "")
				return cueTitle;
		   	else
			   	return shared.sub_title;
		} else if(what == "message")
			return shared.message;
		return "";//mp.getMeta(what);
	}

	State getState() {
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

	void setVolume(float volume); // { mp.setVolume(volume); }

	float getVolume() const { return shared.volume; } //mp.getVolume(); }

	void stop();

	bool wasFromQueue() const { return playedNext; }
	
	std::vector<utils::File> getSongFiles() { 
		LOCK_GUARD(plMutex);
		return shared.songFiles;
	}


private:
	
	void onThisThread(std::function<void()> f) {
		LOCK_GUARD(plMutex);
		funcs.push_back(f);
	}
	void putError(const std::string &error);
	
	std::vector<std::function<void()>> funcs;
	
	
	bool handlePlaylist(const std::string &fileName);
	void playCurrent();
	bool playFile(const std::string &fileName);

	void update();
	void updateInfo();

	bool checkPermission(int flags);

	std::deque<std::string> errors;

	std::shared_ptr<MusicPlayer> mp;
	std::mutex plMutex;
	
	struct PlayQueue {
		std::deque<SongInfo> songs;
		std::deque<SongInfo> psongs;
		std::string prodScreenshot;
		int size() const { return songs.size() + psongs.size(); }
		void push_back(const SongInfo &s) { songs.push_back(s); dirty = true; }
		//void push_font(const SongInfo &s) { songs.push_front(s); }
		void clear() { psongs.clear(); songs.clear(); dirty = true; }
		void pop_front() { 
			dirty = true;
			if(psongs.size() > 0)
				psongs.pop_front();
			else
				songs.pop_front(); 
		}
		const SongInfo& front() const {
			if(psongs.size() > 0)
				return psongs.front();
			return songs.front(); 
		}
		SongInfo& front() {
			if(psongs.size() > 0)
				return psongs.front();
			return songs.front(); 
		}
		const SongInfo& getSong(int i) const { 
			if(i < (int)psongs.size())
				return psongs[i];
			return songs[i - psongs.size()]; 
		}
		void insertAt(int i, const SongInfo &s) {
			songs.insert( songs.begin() + i, s);
			dirty = true;
		}

		bool dirty = false;
	};
	
	PlayQueue playList;
	// Current playing song, with updated info from player if possible
	SongInfo currentInfo;
	// Currently playing song as it was fetched from the database
	SongInfo dbInfo;


	std::atomic<bool> wasAllowed;
	std::atomic<bool> quitThread;

	std::atomic<int> files;
	std::string loadedFile;

	std::atomic<State> state; // = STOPPED;

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
	
};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H
