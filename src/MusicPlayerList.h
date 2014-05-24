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
		STARTED,
		PLAY_STARTED,
		PLAYING
	};

	void addSong(const SongInfo &si);
	void clearSongs();
	void nextSong();
	void playFile(const std::string &fileName);
	State update();
	uint16_t *getSpectrum();
	int spectrumSize();
	SongInfo getInfo(int index = 0);
	int getLength();
	int getPosition();
	int listSize();

private:
	MusicPlayer mp;
	std::mutex plMutex;
	std::deque<SongInfo> playList;

	WebGetter webgetter { "_files" };

	State state = STOPPED;
	int length;
	SongInfo currentInfo;

};

} // namespace chipmachine

#endif // MUSIC_PLAYER_LIST_H

