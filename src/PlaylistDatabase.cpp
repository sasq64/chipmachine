#include "PlaylistDatabase.h"
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>

using namespace std;
using namespace utils;

namespace chipmachine {



// void PlaylistDatabase::init() {
PlaylistDatabase::PlaylistDatabase() : db("play.db") {

	db.exec("CREATE TABLE IF NOT EXISTS playlist (title STRING)");
	db.exec("CREATE TABLE IF NOT EXISTS plentry (playlist INT, pos INT, path STRING)");

	//createPlaylist("Favorites");

	string title, path;
	int rowid, id, pos;

	auto q = db.query<int, string>("SELECT rowid, title FROM playlist");
	if(q.step()) {
		tie(rowid, title) = q.get_tuple();
		if(rowid >= playlists.size())
			playlists.resize(rowid+1);
		playlists[rowid] = Playlist(title);
	}

	auto q2 = db.query<int, int, string>("SELECT playlist, pos, path FROM plentry");
	if(q2.step()) {
		tie(id, pos, path) = q2.get_tuple();
		vector<SongInfo> &pl = playlists[id].songs;
		if(pos >= pl.size())
			playlists.resize(pos+1);
		pl[pos] = SongInfo(path);
	}

}

void PlaylistDatabase::createPlaylist(const std::string &name) {
	db.exec("INSERT INTO playlist (title) VALUES (?)", name);
	playlists.push_back(Playlist(name));
}



int PlaylistDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {


	auto pls = playlists.size();

	titleIndex.search(title_query, result, searchLimit);
	for(auto &i : result)
		 i += pls;

}
}
