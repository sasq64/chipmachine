#ifndef DATABASE_H
#define DATABASE_H

#include "SongInfo.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>

#include <unordered_set>
#include <mutex>
#include <vector>
#include <string>

namespace chipmachine {

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};

struct Playlist {
	Playlist() {}
	Playlist(const std::string &name) : name(name) {}
	std::string name;
	std::vector<SongInfo> songs;
};

class PlaylistDatabase {
public:
	PlaylistDatabase();
	void createPlaylist(const std::string &name);

	int search(const std::string &query, std::vector<int> &result, unsigned int searchLimit) override;

	static PlaylistDatabase& getInstance() {
		static PlaylistDatabase mdb;
		return mdb;
	}

private:
	std::vector<Playlist> playlists;
	sqlite3db::Database db;
};

}

#endif // DATABASE_H
