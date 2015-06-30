#include "PlaylistDatabase.h"
#ifdef USE_REMOTELISTS
#include "RemoteLists.h"
#endif
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <archive/archive.h>

#include <set>
#include <algorithm>

using namespace std;
using namespace utils;

namespace chipmachine {



// void PlaylistDatabase::init() {
PlaylistDatabase::PlaylistDatabase() : db(File::getCacheDir() / "play.db") {

	db.exec("CREATE TABLE IF NOT EXISTS playlist (title STRING PRIMARY KEY UNIQUE)");
	db.exec("CREATE TABLE IF NOT EXISTS song (playlist INT, title STRING, game STRING, composer STRING, format STRING, path STRING)");

	createPlaylist("Favorites");
/*
	RemoteLists::getInstance().getLists([=](vector<string> lists) {
		for(auto l : lists) {
			playlists.emplace_back(l, true);
		}
	});
*/

	string title, path;
	int rowid, id;

	// Create playlists vector from database
	auto q = db.query<int, string>("SELECT rowid, title FROM playlist");
	while(q.step()) {
		tie(rowid, title) = q.get_tuple();
		if(rowid > (int)playlists.size())
			playlists.resize(rowid);
		playlists[rowid-1] = Playlist(title);
	}

	// Populate playlists from database
	id = 1;
	for(auto &pl : playlists) {		
		auto q = db.query<string, string, string, string, string>("SELECT path,game,title,composer,format FROM song WHERE playlist=?", id);
		while(q.step()) {
			SongInfo song;
			//int cid;
			//string path;
			tie(song.path, song.game, song.title, song.composer, song.format) = q.get_tuple();
			//auto collection = MusicDatabase::getInstance().getCollection(cid);

			//song.path = collection.local_dir + path;
			//LOGD("LOCAL PATH: %s", song.path);
			//if(!File::exists(song.path))
			//	song.path = collection.url + path;

			pl.songs.push_back(song);
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

void PlaylistDatabase::dumpPlaylist(const std::string &name, const std::string &dirName) {
	int id = 1;
	for(const auto &p : playlists) {
		if(p.name == name) {
			File f { dirName + "/" + name };
			for(const auto &song : p.songs) {
				f.write(format("%s\n", song.path));
			}
			f.close();
		}
	}
}

void PlaylistDatabase::addToPlaylist(const std::string &name, const SongInfo &song) {
	int id = 1;
	for(auto &p : playlists) {
		if(p.name == name) {
			//auto path = song.path;
			//auto collection = MusicDatabase::getInstance().stripCollectionPath(path);
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

/*Playlist */ void PlaylistDatabase::getPlaylist(const std::string &name, std::function<void(const Playlist &)> cb) {
	for(auto &p : playlists) {
		if(p.name == name) {
#ifdef USE_REMOTELISTS
			if(p.isRemote) {
				RemoteLists::getInstance().getList(name, [=](const string &name, const vector<string> &list) {

					Playlist *pl = nullptr;
					for(auto &p : playlists) {
						if(p.name == name) {
							pl = &p;
							pl->isRemote = false;
							break;
						}
					}
					if(!pl) return;
					for(const auto &l : list) {
						LOGD("LISTSONG %s", l);
						//auto parts = split(l, ":");
						SongInfo song = MusicDatabase::getInstance().lookup(l);
						pl->songs.push_back(song);
						//auto collection = MusicDatabase::getInstance().getCollection(parts[1]);
						//pl->songs.emplace_back(collection.url + parts[0]);
					}
					cb(*pl);
				});
			} else
#endif
				cb(p);
		}
	}
	//throw exception();
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
