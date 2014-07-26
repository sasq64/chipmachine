#include "PlaylistDatabase.h"

#include <coreutils/utils.h>
#include <archive/archive.h>

#include <set>
#include <algorithm>

using namespace std;
using namespace utils;

namespace chipmachine {



// void PlaylistDatabase::init() {
PlaylistDatabase::PlaylistDatabase() : db("play.db") {

	db.exec("CREATE TABLE IF NOT EXISTS playlist (title STRING PRIMARY KEY UNIQUE)");
	db.exec("CREATE TABLE IF NOT EXISTS song (playlist INT, title STRING, game STRING, composer STRING, format STRING, path STRING, collection INTEGER)");

	createPlaylist("Favorites");

	string title, path;
	int rowid, id;

	// Create playlists vector from database
	auto q = db.query<int, string>("SELECT rowid, title FROM playlist");
	while(q.step()) {
		tie(rowid, title) = q.get_tuple();
		if(rowid > playlists.size())
			playlists.resize(rowid);
		playlists[rowid-1] = Playlist(title);
	}

	// Populate playlists from database
	id = 1;
	for(auto &pl : playlists) {
		auto q = db.query<string, string, string, string, string>("SELECT path,game,title,composer,format FROM song WHERE playlist=?", id);
		while(q.step()) {
			pl.songs.push_back(q.get<SongInfo>());
		}
		id++;
	}

}

void PlaylistDatabase::createPlaylist(const string &name) {
	db.exec("INSERT INTO playlist (title) VALUES (?)", name);
	playlists.push_back(Playlist(name));
}

void PlaylistDatabase::renamePlaylist(const string &oldName, const string &newName) {
	db.exec("UPDATE playlist SET title=? WHERE title=?", newName, oldName); 
	for(auto &p : playlists) {
		if(p.name == oldName) {
			p.name = newName;
			break;
		}
	}
}

void PlaylistDatabase::addToPlaylist(const std::string &name, const SongInfo &song) {
	int id = 1;
	for(auto &p : playlists) {
		if(p.name == name) {
			db.exec("INSERT INTO song (playlist, title, game, composer, format, path) VALUES (?, ?, ?, ?, ?, ?)", id, song.title, song.game, song.composer, song.format, song.path);
			p.songs.push_back(song);
		}
		id++;
	}
}


void PlaylistDatabase::removeFromPlaylist(const std::string &name, const SongInfo &info) {
	int id = 1;
	LOGD("%s", info.path);
	for(auto &p : playlists) {
		if(p.name == name) {
			db.exec("DELETE FROM song WHERE path=?", info.path);
			auto newend = remove(p.begin(), p.end(), info);
			p.songs.erase(newend, p.end());
		}
		id++;
	}
}

Playlist PlaylistDatabase::getPlaylist(const std::string &name) {
	for(auto &p : playlists) {
		if(p.name == name) {
			return p;
		}
	}
	throw exception();
}

int PlaylistDatabase::search(const string &query, vector<string> &result) {

	auto q = toLower(query);
	if(query == "") {
		for(const auto &p : playlists)
			result.push_back(p.name);
		//transform(playlists.begin(), playlists.end(), result.begin(), [](const Playlist &pl) -> string { return pl.name; });
	} else {
		for(const auto &p : playlists) {
			auto n = toLower(p.name);
			if(n.find(query) != string::npos)
				result.push_back(p.name);
		}
	}
	return result.size();
}

}
