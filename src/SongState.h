#ifndef SONG_STATE_H
#define SONG_STATE_H

#include <string>

// Information about the currently playing song file
struct SongState {
	std::string title;
	std::string composer;
	std::string format;
	int length;
	int totalSongs;
};

// Current playing state
struct PlayState {
	int seconds;
	int currentSong;
};

#endif // SONG_STATE_H