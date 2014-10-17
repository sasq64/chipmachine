#include "MusicDatabase.h"
#include "SongFileIdentifier.h"
#include "RemoteLoader.h"

#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>

using namespace std;
using namespace utils;

namespace chipmachine {

static const std::set<std::string> secondary = { "smpl", "sam", "ins", "smp" };

#ifdef RASPBERRYPI
static std::unordered_set<std::string> exclude = {
	"Nintendo DS Sound Format", "Gameboy Sound Format", "Dreamcast Sound Format", "Ultra64 Sound Format",
	"Saturn Sound Format" /*, "RealSID", "PlaySID" */
};
#else
static std::unordered_set<std::string> exclude = { /*"RealSID", "PlaySID",*/ "Saturn Sound Format" };
#endif


void MusicDatabase::initDatabase(unordered_map<string, string> &vars) {

	auto type = vars["type"];
	LOGD("Init db '%s'", type);
	auto source = vars["source"];
	auto local_dir = vars["local_dir"];
	auto song_list = vars["song_list"];
	auto description = vars["description"];
	auto xformats = vars["exclude_formats"];
	auto id = stol(vars["id"]);

	// Return if this collection has already been indexed
	if(db.query("SELECT 1 FROM collection WHERE name = ?", type).step())
		return;

	auto ex_copy = exclude;
	auto parts = split(xformats, ";");
	for(const auto &p : parts) {
		if(p.length())
			ex_copy.insert(p);
	}

	print_fmt("Creating '%s' database\n", type);

	db.exec("BEGIN TRANSACTION");

	if(!endsWith(local_dir, "/"))
		local_dir += "/";

	db.exec("INSERT INTO collection (name, url, localdir, description, id) VALUES (?, ?, ?, ?, ?)",
		type, source, local_dir, description, id);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	auto path = current_exe_path() + ":" + File::getAppDir();
	File listFile = File::findFile(path, song_list);
	//File listFile(song_list);

	if(listFile.exists()) {

		bool isModland = (type == "modland");
		bool isRKO = (type == "rko");

		for(const auto &s : listFile.getLines()) {
			auto parts = split(s, "\t");
			if(parts.size() >= 2) {
				if(isModland) {
					SongInfo song(parts[1]);
					if(!parseModlandPath(song))
						continue;
					if(ex_copy.count(song.format) > 0)
						continue;
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				} else if(isRKO) {
					SongInfo song(parts[0], "", parts[3], parts[4], "MP3");
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				} else {
					SongInfo song(parts[4], parts[1], parts[0], parts[2], parts[3]);
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				}
				query.step();
			}
		}
	} else {

		makedir(".rsntemp");
		function<void(File &)> checkDir;
		checkDir = [&](File &root) {
			for(auto &rf : root.listFiles()) {
				if(rf.isDir()) {
					checkDir(rf);
				} else {
					auto name = rf.getName();
					SongInfo songInfo(name);
					if(identify_song(songInfo)) {

						auto pos = name.find(local_dir);
						if(pos != string::npos) {
							name = name.substr(pos + local_dir.length());
						}

						query.bind(songInfo.title, songInfo.game, songInfo.composer, songInfo.format, name, id);
						query.step();
					}
				}
			}
		};
		File root { local_dir };
		checkDir(root);
	}
	db.exec("COMMIT");
}

bool MusicDatabase::parseModlandPath(SongInfo &song) {

	static const unordered_set<string> hasSubFormats = {
		"Ad Lib",
		"Video Game Music"
	};

	string ext = path_extension(song.path);
	if(secondary.count(ext) > 0) {
		return false;
	}

	auto parts = split(song.path, "/");
	int l = parts.size();
	if(l < 3) {
		LOGD("%s", song.path);
		return false;
	}

	int i = 0;
	song.format = parts[i++];
	if(hasSubFormats.count(song.format) > 0)
		song.format = parts[i++];

	song.composer = parts[i++];

	if(song.format == "MDX") {
		i--;
		song.composer = "?";
	}

	if(song.composer == "- unknown")
		song.composer = "?";

	if(parts[i].substr(0, 5) == "coop-")
		song.composer = song.composer + "+" + parts[i++].substr(5);

	//string game;
	if(l-i >= 2)
		song.game = parts[i++];

	if(i == l) {
		LOGD("Bad file %s", song.path);
		return false;
	}

	if(endsWith(parts[i], ".rar"))
		parts[i] = parts[i].substr(0, parts[i].length()-4);

	song.title = path_basename(parts[i++]);

	return true;
}

int MusicDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

	lock_guard<mutex>{dbMutex};

	result.resize(0);
	//if(query.size() < 3)
	//	return 0;
	//bool q3 = (q.size() <= 3);
	string title_query = query;
	string composer_query = query;

	auto p = split(query, "/");
	if(p.size() > 1) {
		title_query = p[0];
		composer_query = p[1];
	}

	titleIndex.search(title_query, result, searchLimit);

	vector<int> cresult;
	composerIndex.search(composer_query, cresult, searchLimit);
	for(int index : cresult) {
		int offset = composerTitleStart[index];
		while(composerToTitle[offset] != -1)
			result.push_back(composerToTitle[offset++]);
	}

	int sz = result.size();
	int j = sz;
	/*
	for(int i=0, j=0; i<sz; i++) {
		if(formats[result[i]] == 0x600) {
			result[j++] = result[i];
		}
	}
	result.resize(j);
	*/
	return j;
}

SongInfo MusicDatabase::lookup(const std::string &p) {
	auto path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1)
		path = parts[1];
	else {
		parts = split(path, ":");
		if(parts[1] == "modland" || parts[1] == "hvsc")
			path = parts[0];
	}

	auto q = db.query<string, string, string, string, string>("SELECT title, game, composer, format, collection.name FROM song, collection WHERE song.path=? AND song.collection = collection.id", path);

	SongInfo song;
	if(q.step()) {
		string coll;
		LOGD("Found %s in %s", song.title, path);
		tie(song.title, song.game, song.composer, song.format, coll) = q.get_tuple();
		song.path = coll + "::"+ path;
	}
	return song;
}

string MusicDatabase::getFullString(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string, string, string>("SELECT title, game, composer, format, song.path, collection.name FROM song, collection WHERE song.ROWID = ? AND song.collection = collection.id", id);
	if(q.step()) {
		string title, game, composer, format, path, collection;

		tie(title, game, composer, format, path, collection) = q.get_tuple();

		path = collection + "::" + path;

		if(game != "")
			title = utils::format("%s [%s]", game, title);

		string r = utils::format("%s\t%s\t%s\t%s", path, title, composer, format);
		LOGD("RESULT %s", r);
		return r;
	}
	throw not_found_exception();
}

// console -- sid -- tracker -- amiga

enum Formats {

	UNKNOWN_FORMAT,
	NO_FORMAT,

	CONSOLE,

	NINTENDO,

	GAMEBOY,
	SNES,
	NINTENDO64,
	NITENDODS,

	SEGA,

	SEGAMS,
	MEGADRIVE,
	DREAMCAST,

	SONY,

	PLAYSTATION,
	PLAYSTATION2,

	COMPUTER,
	C64,
	SID,

	ATARI,

	PC,

	ADPLUG,
	TRACKER = 0x30,
	SCREAMTRACKER,
	IMPULSETRACKER,
	FASTTRACKER,

	AMIGA,
	PROTRACKER,

	UADE,
};

static uint8_t formatToByte(const std::string &f) {
	return 0;
}

template <typename T> static void readVector(std::vector<T> &v, File &f) {
	auto sz = f.read<uint32_t>();
	v.resize(sz);
	f.read((uint8_t*)&v[0], v.size()*sizeof(T));

}

template <typename T> static void writeVector(std::vector<T> &v, File &f) {
	f.write<uint32_t>(v.size());
	f.write((uint8_t*)&v[0], v.size()*sizeof(T));
}

void MusicDatabase::readIndex(File &f) {
	readVector(titleToComposer, f);
	readVector(composerToTitle, f);
	readVector(composerTitleStart, f);
	readVector(formats, f);

	titleIndex.load(f);
	composerIndex.load(f);
}

void MusicDatabase::writeIndex(File &f) {
	writeVector(titleToComposer, f);
	writeVector(composerToTitle, f);
	writeVector(composerTitleStart, f);
	writeVector(formats, f);

	titleIndex.dump(f);
	composerIndex.dump(f);
}

void MusicDatabase::generateIndex() {

	lock_guard<mutex>{dbMutex};

	RemoteLoader &loader = RemoteLoader::getInstance();
	auto q = db.query<int, string, string, string>("SELECT ROWID,name,url,localdir FROM collection");
	while(q.step()) {
		auto c = q.get<Collection>();
		loader.registerSource(c.name, c.url, c.local_dir);
	}

	File f { File::getCacheDir() + "index.dat" };

	if(f.exists()) {
		readIndex(f);
		f.close();
		return;
	}

	print_fmt("Creating Search Index...\n");

	string oldComposer;
	auto query = db.query<string, string, string, string, string, int>("SELECT title, game, format, composer, path, collection FROM song");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;

	titleToComposer.reserve(438000);
	composerToTitle.reserve(37000);
	titleIndex.reserve(438000);
	composerIndex.reserve(37000);
	formats.reserve(438000);

	int step = 438000 / 20;


	unordered_map<string, vector<uint32_t>> composers;

	string title, game, fmt, composer, path;
	int collection;

	while(count < 1000000) {
		count++;
		if(!query.step())
			break;

		if(count % step == 0) {
			LOGD("%d songs indexed", count);
		}

		tie(title, game, fmt, composer, path, collection) = query.get_tuple();

		uint8_t b = formatToByte(fmt);
		formats.push_back(b | (collection<<8));

		if(game != "") {
			if(title != "")
				title = format("%s [%s]", game, title);
			else title = game;
		}

		// The title index maps one-to-one with the database
		int tindex = titleIndex.add(title);


		auto &v = composers[composer];
		if(v.size() == 0) {
			cindex = composerIndex.add(composer);
			composers[composer].push_back(cindex);
		} else
			cindex = composers[composer][0];

		composers[composer].push_back(tindex);

		// We also need to find the composer for a give title
		titleToComposer.push_back(cindex);
	}

	// composers[name] -> vector of titleindexes for each composer.

	LOGD("Found %d composers and %d titles", composers.size(), titleToComposer.size());

	composerTitleStart.resize(composers.size());
	for(const auto &p : composers) {
		// p,first == composer, p.second == vector
		auto cindex = p.second[0];
		composerTitleStart[cindex] = composerToTitle.size();
		for(int i=1; i<(int)p.second.size(); i++)
			composerToTitle.push_back(p.second[i]);
		composerToTitle.push_back(-1);
	}

	writeIndex(f);
	f.close();
}

}
