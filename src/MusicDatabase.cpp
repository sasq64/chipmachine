#include "MusicDatabase.h"
#include "SongFileIdentifier.h"
#include "RemoteLoader.h"

#include <luainterpreter/luainterpreter.h>
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>
#include <chrono>

using namespace std;
using namespace utils;

namespace chipmachine {

static const std::set<std::string> secondary = { "smpl", "sam", "ins", "smp" };

#ifdef RASPBERRYPI
static std::unordered_set<std::string> exclude = {
	"Gameboy Sound Format", "Dreamcast Sound Format", "Saturn Sound Format"
};
#else
static std::unordered_set<std::string> exclude = { "Gameboy Sound Format", "Saturn Sound Format" };
#endif


void MusicDatabase::initDatabase(const std::string &workDir, unordered_map<string, string> &vars) {

	auto type = vars["type"];
	auto name = vars["name"];
	auto source = vars["source"];
	auto local_dir = vars["local_dir"];
	auto song_list = vars["song_list"];
	auto description = vars["description"];
	auto xformats = vars["exclude_formats"];
	
	// Return if this collection has already been indexed in this version
	auto cq = db.query<uint64_t>("SELECT ROWID FROM collection WHERE id = ?", type);
	if(cq.step()) {
		return;
	}
	cq.finalize();

	reindexNeeded = true;

	auto ex_copy = exclude;
	auto parts = split(xformats, ";");
	for(const auto &p : parts) {
		if(p.length())
			ex_copy.insert(p);
	}

	if(local_dir != "" && !endsWith(local_dir, "/"))
		local_dir += "/";

	print_fmt("Creating '%s' database\n", name);

	db.exec("BEGIN TRANSACTION");
	db.exec("INSERT INTO collection (name, id, url, localdir, description) VALUES (?, ?, ?, ?, ?)",
		name, type, source, local_dir, description);
	auto collection_id = db.last_rowid();

	File listFile = File::findFile(workDir, song_list);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	if(listFile.exists()) {

		bool isModland = (type == "modland");
		bool isRKO = (type == "rko");
		bool isAmiRemix = (type == "amigaremix");

		for(const auto &s : listFile.getLines()) {
			auto parts = split(s, "\t");
			if(parts.size() >= 2) {
				if(isModland) {
					SongInfo song(parts[1]);
					if(!parseModlandPath(song))
						continue;
					if(ex_copy.count(song.format) > 0)
						continue;
					query.bind(song.title, song.game, song.composer, song.format, song.path, collection_id);
				} else if(isRKO) {
					parts[3] = htmldecode(parts[3]);
					parts[4] = htmldecode(parts[4]);
					SongInfo song(parts[0], "", parts[3], parts[4], "MP3");
					query.bind(song.title, song.game, song.composer, song.format, song.path, collection_id);
				} else if(isAmiRemix) {
					if(parts[0].find(source) == 0)
						parts[0] = parts[0].substr(source.length());
					SongInfo song(parts[0], "", parts[2], parts[3], "MP3");
					query.bind(song.title, song.game, song.composer, song.format, song.path, collection_id);
				} else {
					SongInfo song(parts[4], parts[1], parts[0], parts[2], parts[3]);
					query.bind(song.title, song.game, song.composer, song.format, song.path, collection_id);
				}
				query.step();
			}
		}
	} else if(local_dir != "") {

		makedir(".rsntemp");
		function<void(File &)> checkDir;
		checkDir = [&](File &root) {
			LOGD("DIR %s", root.getName());
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

						query.bind(songInfo.title, songInfo.game, songInfo.composer, songInfo.format, name, collection_id);
						query.step();
					}
				}
			}
		};

		if(local_dir[0] != '/')
			local_dir = workDir + "/" + local_dir;
		File root { local_dir };

		LOGD("Checking local dir '%s'", root.getName());

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

	//titleIndex.setFilter([&](int index) {
	//	return ((formats[index] & 0xff) != C64);
	//});

	titleIndex.search(title_query, result, searchLimit);

	if(result.size() >= searchLimit)
		return searchLimit;

	searchLimit -= result.size();

	vector<int> cresult;
	composerIndex.search(composer_query, cresult, searchLimit);
	for(int index : cresult) {
		int offset = composerTitleStart[index];
		while(composerToTitle[offset] != -1) {
			if(result.size() >= searchLimit)
				break;
			int songindex = composerToTitle[offset++];
			//if((formats[songindex] & 0xff) != C64)
			//	continue;
			result.push_back(songindex);
		}
		if(result.size() >= searchLimit)
			break;
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

	auto q = db.query<string, string, string, string, string>("SELECT title, game, composer, format, collection.id FROM song, collection WHERE song.path=? AND song.collection = collection.ROWID", path);

	SongInfo song;
	if(q.step()) {
		string coll;
		tie(song.title, song.game, song.composer, song.format, coll) = q.get_tuple();
		LOGD("Found %s from %s", song.title, path);
		song.path = coll + "::"+ path;
	}
	return song;
}

SongInfo MusicDatabase::getSongInfo(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string, string, string>("SELECT title, game, composer, format, song.path, collection.id FROM song, collection WHERE song.ROWID = ? AND song.collection = collection.ROWID", id);
	if(q.step()) {
		//string title, game, composer, format, path, collection;
		SongInfo song;
		string collection;
		tie(song.title, song.game, song.composer, song.format, song.path, collection) = q.get_tuple();

		song.path = collection + "::" + song.path;

		if(song.game != "")
			song.title = utils::format("%s [%s]", song.game, song.title);

		//string r = utils::format("%s\t%s\t%s\t%s", path, title, composer, format);
		//LOGD("RESULT %s", r);
		//return r;
		return song;
	}
	throw not_found_exception();
}

#include "formats.h"

static std::unordered_map<std::string, uint8_t> format_map;

void initFormats() {
	for(const char *f : uade_formats) {
		format_map[f] = UADE;
	}
	for(const char *f : adlib_formats) {
		format_map[f] = ADPLUG;
	}

	format_map["commodore 64"] = C64;
	format_map["cyber tracker"] = C64;
	format_map["super nintendo"] = SNES;
	format_map["hes"] = HES;
	format_map["mp3"] = MP3;
	format_map["sc68"] = ATARI;
	format_map["ultra64 sound format"] = NINTENDO64;
	format_map["nintendo ds sound format"] = NDS;
	format_map["nintendo sound format"] = NES;
	format_map["sega master system"] = SEGAMS;
	format_map["sega game gear"] = SEGAMS;
	format_map["playstation sound format"] = PLAYSTATION;
	format_map["dreamcast sound format"] = DREAMCAST;
	format_map["playlist"] = PLAYLIST;
	format_map["c64 demo"] = PLAYLIST;
	format_map["c64 event"] = PLAYLIST;
}

static uint8_t formatToByte(const std::string &fmt) {

	static bool init = false;
	if(!init) {
		initFormats();
		init = true;
	}

	string f = toLower(fmt);
	uint8_t l = format_map[f];
	if(l == 0) {

		l = UNKNOWN_FORMAT;

		if(endsWith(f, "tracker"))
			l = TRACKER;
		if(startsWith(f, "protracker"))
			l = PROTRACKER;
		else
		if(startsWith(f, "fasttracker"))
			l = FASTTRACKER;
		else
		if(startsWith(f, "impulsetracker"))
			l = IMPULSETRACKER;
		else
		if(startsWith(f, "screamtracker"))
			l = SCREAMTRACKER;
		else
		if(startsWith(f, "atari"))
			l = ATARI;
		else
		if(startsWith(f, "ay ") || startsWith(f, "spectrum "))
			l = SPECTRUM;
		else
		if(startsWith(f, "gameboy"))
			l = GAMEBOY;
		if(f.find("megadrive") != string::npos)
			l = MEGADRIVE;
		format_map[f] = l;
		//fprintf(stderr, "%s\n", f.c_str());
	}
	return l;
}

/*
emul
ct cyber tracker
hes
atari digi mix
kss .. really gme?
*/

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

	indexVersion = 0;
	auto marker = f.read<uint16_t>();
	if(marker == 0xFEDC)
		indexVersion = f.read<uint16_t>();
	else
		f.seek(0);
	readVector(titleToComposer, f);
	readVector(composerToTitle, f);
	readVector(composerTitleStart, f);
	readVector(formats, f);

	titleIndex.load(f);
	composerIndex.load(f);
}

void MusicDatabase::writeIndex(File &f) {
	f.write<uint16_t>(0xFEDC);
	f.write<uint16_t>(dbVersion);
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
	auto q = db.query<int, string, string, string>("SELECT ROWID,id,url,localdir FROM collection");
	while(q.step()) {
		auto c = q.get<Collection>();
		// NOTE c.name is really c.id
		loader.registerSource(c.name, c.url, c.local_dir);
	}

	File f { File::getCacheDir() + "index.dat" };

	if(!reindexNeeded && f.exists()) {
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

	reindexNeeded = false;
}

void MusicDatabase::initFromLuaAsync(const string &workDir) {
	indexing = true;
	initFuture = std::async(std::launch::async, [&]() {
		std::lock_guard<std::mutex>{dbMutex};
		initFromLua(workDir);
		indexing = false;
	});
}


void MusicDatabase::initFromLua(const string &workDir) {

	reindexNeeded = false;

	File fi { File::getCacheDir() + "index.dat" };

	indexVersion = 0;
	if(fi.exists()) {
		auto marker = fi.read<uint16_t>();
		if(marker == 0xFEDC)
			indexVersion = fi.read<uint16_t>();
	}

	LuaInterpreter lua;

	lua.registerFunction<void, string, string>("set_db_var", [&](string name, string val) {
		static unordered_map<string, string> dbmap;
		if(val == "start") {
		} else if(val == "end") {
			initDatabase(workDir, dbmap);
			dbmap.clear();
		} else {
			dbmap[name] = val;
		}
	});

	File f = File::findFile(workDir, "lua/db.lua");

	lua.loadFile(f.getName());

	dbVersion = lua.getGlobal<int>("VERSION");
	LOGD("DBVERSION %d INDEXVERSION %d", dbVersion, indexVersion);
	if(dbVersion != indexVersion) {
		db.exec("DELETE FROM collection");
		db.exec("DELETE FROM song");
		reindexNeeded = true;
	}

	lua.load(R"(
		for a,b in pairs(DB) do
			if type(b) == 'table' then
				set_db_var(a, 'start')
				for a1,b1 in pairs(b) do
					set_db_var(a1, b1)
				end
				set_db_var(a, 'end')
			end
		end
	)");
	generateIndex();
}


}
