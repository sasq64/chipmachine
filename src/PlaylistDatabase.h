#ifndef PLAYLIST_DATABASE_H
#define PLAYLIST_DATABASE_H

#include "SongInfo.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>

#include <unordered_set>
#include <mutex>
#include <vector>
#include <string>

namespace chipmachine {

struct Playlist {
	Playlist() {}
	Playlist(const std::string &name, bool remote = false) : name(name), isRemote(remote) {}
	std::string name;
	bool isRemote;

	std::vector<SongInfo> songs;
	std::vector<SongInfo>::iterator begin() { return songs.begin(); }
	std::vector<SongInfo>::const_iterator begin() const { return songs.cbegin(); }
	std::vector<SongInfo>::iterator end() { return songs.end(); }
	std::vector<SongInfo>::const_iterator end() const { return songs.cend(); }
};

class PlaylistDatabase {
public:
	PlaylistDatabase();
	void createPlaylist(const std::string &name);
	void renamePlaylist(const std::string &oldName, const std::string &newName);
	void addToPlaylist(const std::string &name, const SongInfo &info);
	void removeFromPlaylist(const std::string &name, const SongInfo &info);
	void getPlaylist(const std::string &name, std::function<void(const Playlist &)> cb);
	Playlist getPlaylist(const std::string &name);

	int search(const std::string &query, std::vector<std::string> &result);

	static PlaylistDatabase& getInstance() {
		static PlaylistDatabase mdb;
		return mdb;
	}

private:
	std::vector<Playlist> playlists;
	sqlite3db::Database db;
};

}

#endif // PLAYLIST_DATABASE_H
