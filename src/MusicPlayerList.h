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


struct log_guard {
	log_guard(std::mutex &m, const char* f, int l) : m(m) {
		if(!m.try_lock()) {
			logging::log2(logging::xbasename(f), l, logging::DEBUG, "Waiting for lock");
			m.lock();
		}
	}
	~log_guard() {
		m.unlock();
	}
	std::mutex &m;
};

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGE("WAITING FOR LOCK"); \
//std::lock_guard<std::mutex> guard(x)
//#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) log_guard guard(x, __FILE__, __LINE__)
#define SET_STATE(x) (LOGD("STATE: " #x), state = x)

namespace chipmachine {

class ChipMachine;

#ifdef _WIN32
#undef ERROR
#endif

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

	bool isPlaying() { 
		return playing;
	}

	int getTune() {
		return currentTune;
	}

	void pause(bool dopause = true) {
		if(!(permissions & CAN_PAUSE))
			return;
		LOCK_GUARD(plMutex);
		mp.pause(dopause);
	}

	bool isPaused() { 
		return paused;
	}

	void seek(int song, int seconds = -1);

	int getBitrate() {
		return bitRate;
	}

	std::string getMeta(const std::string &what) {
		if(what == "sub_title") {
			std::string sub = std::string(subtitlePtr);
			return sub;
		}
		LOCK_GUARD(plMutex);
		LOGD("META %s", what);
		return mp.getMeta(what);
	}

	State getState() {
		// LOCK_GUARD(plMutex);
		State rc = state;
		if(rc == PLAY_STARTED) {
			onThisThread([=] {
				SET_STATE(PLAYING);
			});
		}
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

	void setVolume(float volume) {
		onThisThread([=] {
			mp.setVolume(volume);
		});
	}

	float getVolume()  {
		LOCK_GUARD(plMutex);
		return mp.getVolume();
	}

	void stop() {
		onThisThread([=] {
			SET_STATE(STOPPED);
			mp.stop();
		});
	}

	bool wasFromQueue() const { return playedNext; }
	
	const std::vector<utils::File> &getSongFiles() const { return songFiles; }

	bool playlistUpdated() {
		return playList.wasUpdated();
	}

private:
	
	void onThisThread(std::function<void()> f) {
		LOCK_GUARD(plMutex);
		funcs.push_back(f);
	}
	
	std::vector<std::function<void()>> funcs;
	
	void cancelStreaming();
	bool handlePlaylist(const std::string &fileName);
	void playCurrent();
	bool playFile(const std::string &fileName);

	void update();
	void updateInfo();

	bool checkPermission(int flags);

	std::deque<std::string> errors;

	MusicPlayer mp;

	// Lock when accessing MusicPlayer
	std::mutex plMutex;
	
	struct PlayQueue {
		std::atomic<bool> updated;
		std::deque<SongInfo> songs;
		std::deque<SongInfo> psongs;
		std::string prodScreenshot;
		int size() { return songs.size() + psongs.size(); }
		void push_back(const SongInfo &s) { 
			songs.push_back(s);
			updated = true;
		}
		//void push_font(const SongInfo &s) { songs.push_front(s); }
		void clear() { 
			psongs.clear(); songs.clear();
			updated = true;
		}
		void pop_front() { 
			if(psongs.size() > 0)
				psongs.pop_front();
			else
				songs.pop_front(); 
			updated = true;
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
		void insertAt(int i, const SongInfo &s) {
			songs.insert( songs.begin() + i, s);
			updated = true;
		}
		bool wasUpdated() {
			bool rc = updated;
			updated = false;
			return rc;
		}
	};
	
	PlayQueue playList;

	std::atomic<bool> wasAllowed;
	std::atomic<bool> quitThread;

	std::atomic<int> currentTune;
	std::atomic<bool> playing;
	std::atomic<bool> paused;
	std::atomic<int> bitRate;
	std::atomic<int> playerPosition;
	std::atomic<int> playerLength;

	std::atomic<int> files;
	std::string loadedFile;

	std::atomic<State> state;
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
	std::string subtitle;
	std::atomic<const char *> subtitlePtr;

	int multiSongNo;
	std::vector<std::string> multiSongs;
	bool changedMulti = false;
	bool playedNext;
	
	std::vector<utils::File> songFiles;
};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H
