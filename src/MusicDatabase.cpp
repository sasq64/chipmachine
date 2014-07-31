#include "MusicDatabase.h"
//#include "secondary.h"
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


static string get_string(uint8_t *ptr, int size) {

	auto end = ptr;
	while(*end && end-ptr < size) end++;
	return string((const char*)ptr, end-ptr);
}

bool parseSid(SongInfo &info) {
	static vector<uint8_t> buffer(0xd8);
	File f { info.path };
	info.format = "Commodore 64";
	f.read(&buffer[0], buffer.size());
	info.title = get_string(&buffer[0x16], 0x20);
	info.composer = get_string(&buffer[0x36], 0x20);
	//auto copyright = string((const char*)&buffer[0x56], 0x20);
	f.close();
	return true;
}

bool parseSnes(SongInfo &info) {

	static vector<uint8_t> buffer(0xd8);

	info.format = "Super Nintendo";

	auto *a = Archive::open(info.path, ".rsntemp", Archive::TYPE_RAR);
	//LOGD("ARCHIVE %p", a);
	bool done = false;
	for(auto s : *a) {
		//LOGD("FILE %s", s);
		if(done) continue;
		if(path_extension(s) == "spc") {
			a->extract(s);
			File f { ".rsntemp/" + s };
			f.read(&buffer[0], buffer.size());
			f.close();
			if(buffer[0x23] == 0x1a) {
				//auto title = string((const char*)&buffer[0x2e], 0x20);
				auto ptr = (const char*)&buffer[0x4e];
				auto end = ptr;
				while(*end) end++;
				auto game = get_string(&buffer[0x4e], 0x20);
				auto composer = get_string(&buffer[0xb1], 0x20);

				f.seek(0x10200);
				int rc = f.read(&buffer[0], buffer.size());
				if(rc > 12) {
					auto id = string((const char*)&buffer[0], 4);
					if(id == "xid6") {
						//int i = 0;
						if(buffer[8] == 0x2) {
							int l = buffer[10];
							game = string((const char*)&buffer[12], l);
						} else if(buffer[8] == 0x3) {
							int l = buffer[10];
							composer = string((const char*)&buffer[12], l);
						}
					}
				}

				info.composer = composer;
				info.game = game;
				info.title = "";
				done = true;
			}
		}
	}
	delete a;
	return done;
}

bool identifyFile(SongInfo &info, string ext = "") {

	if(ext == "")
		ext = path_extension(info.path);

	if(ext == "rsn")
		return parseSnes(info);
	if(ext == "sid")
		return parseSid(info);
	return false;
}

void MusicDatabase::initDatabase(unordered_map<string, string> &vars) {

	auto type = vars["type"];
	LOGD("Init db '%s'", type);
	auto source = vars["source"];
	auto local_dir = vars["local_dir"];
	auto song_list = vars["song_list"];
	auto description = vars["description"];
	auto xformats = vars["exclude_formats"];
	auto id = stol(vars["id"]);

	auto ex_copy = exclude;
	auto parts = split(xformats, ";");
	for(const auto &p : parts) {
		if(p.length())
			ex_copy.insert(p);
	}

	if(db.query("SELECT 1 FROM collection WHERE name = ?", type).step())
		return;

	//LOGD("Indexing RSN");
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

		for(const auto &s : listFile.getLines()) {
			auto parts = split(s, "\t");
			if(isModland) {
				SongInfo song(parts[1]);
				if(!parseModlandPath(song))
					continue;
				if(ex_copy.count(song.format) > 0)
					continue;
				query.bind(song.title, song.game, song.composer, song.format, song.path, id);
			} else {
				SongInfo song(parts[4], parts[1], parts[0], parts[2], parts[3]);
				query.bind(song.title, song.game, song.composer, song.format, song.path, id);

			}
			query.step();
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
					if(identifyFile(songInfo)) {

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
		LOGD("##QQ '%s', '%s'", p[0], p[1]);
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
/*
		int title_index = composerToTitle[index];

		while(titleToComposer[title_index] == index) {
			result.push_back(title_index++);
		}
*/
	}

	return result.size();
}

SongInfo MusicDatabase::lookup(const std::string &path) {
	auto q = db.query<string, string, string, string, string, string>("SELECT title, game, composer, format, collection.url, collection.localdir FROM song, collection WHERE song.path=? AND song.collection = collection.id", path);

	SongInfo song;
	if(q.step()) {
		string url, localdir;
		tie(song.title, song.game, song.composer, song.format, url, localdir) = q.get_tuple();
		song.path = localdir + path;
		LOGD("LOCAL FILE: %s", song.path);
		if(!File::exists(song.path))
			song.path = url + path;
	}
	return song;
}

string MusicDatabase::getFullString(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string, string, string, string>("SELECT title, game, composer, format, song.path, collection.url, collection.localdir FROM song, collection WHERE song.ROWID = ? AND song.collection = collection.id", id);
	if(q.step()) {
		auto t = q.get_tuple();
		auto title = get<0>(t);
		auto game = get<1>(t);

		auto path = get<6>(t) + get<4>(t);
		LOGD("LOCAL FILE: %s", path);
		if(!File::exists(path))
			path = get<5>(t) + get<4>(t);

		if(game != "")
			title = format("%s [%s]", game, title);

		string r = format("%s\t%s\t%s\t%s", path, title, get<2>(t), get<3>(t));
		LOGD("RESULT %s", r);
		return r;
	}
	throw not_found_exception();
}
/*
SongInfo MusicDatabase::pathToSongInfo(const std::string &path) {
	SongInfo song(path);
	auto coll = stripCollectionPath(song.path);
	song.path = path;
	if(coll == "modland")
		parseModlandPath(song);
	return song;
}
*/
vector<SongInfo> MusicDatabase::find(const string &pattern) {

	lock_guard<mutex> guard(dbMutex);

	vector<SongInfo> songs;

	auto parts = split(pattern, " ");
	string qs = "SELECT path,game,title,composer,format FROM song WHERE ";
	for(int i=0; i<(int)parts.size(); i++) {
		if(i > 0) {
			qs.append("AND path LIKE ?");
		} else
			qs.append("path LIKE ?");
		parts[i] = string("%") + parts[i] + string("%");
	}
	//qs.append(" COLLATE NOCASE");
	LOGD("Query: %s, [%s]", qs, parts);
	auto q = db.query<string, string, string, string, string>(qs, parts);
	//int i = 0;
	try {
		while(q.step()) {
			auto si = q.get<SongInfo>();
			si.path = "ftp://ftp.modland.com/pub/modules/" + si.path;
			songs.push_back(si);
		}
	} catch(invalid_argument &e) {
		LOGD("Illegal shit");
	};

	return songs;
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


MusicDatabase::Collection MusicDatabase::stripCollectionPath(string &path) {
	vector<Collection> colls;
	auto q = db.query<int, string, string, string>("SELECT ROWID,name,url,localdir FROM collection");
	while(q.step()) {
		colls.push_back(q.get<Collection>());
	}
	for(const auto &c : colls) {
		LOGD("COMPARE %s to %s/%s", path, c.url, c.local_dir);
		if(c.url != "" && startsWith(path, c.url)) {
			path = path.substr(c.url.length());
			return c;
		}
		if(c.local_dir != "" && startsWith(path, c.local_dir)) {
			path = path.substr(c.local_dir.length());
			return c;
		}
	}
	return Collection();
}

MusicDatabase::Collection MusicDatabase::getCollection(int id) {
	auto q = db.query<int, string, string, string>("SELECT ROWID,name,url,localdir FROM collection WHERE ROWID=?", id);
	LOGD("ID %d", id);
	if(q.step()) {
		LOGD("####### FOUND");
		return q.get<Collection>();
	}
	return Collection();
}

MusicDatabase::Collection MusicDatabase::getCollection(const string &name) {
	auto q = db.query<int, string, string>("SELECT ROWID,name,url FROM collection WHERE name=?", name);
	if(q.step()) {
		LOGD("####### FOUND");
		return q.get<Collection>();
	}
	return Collection();
}
/*
string getCollectionPath(const string &name) {
	auto q = db.query<string>("SELECT url FROM collection WHERE name = ?", name);
	if(q.step()) {
		return q.get();
	}
}
*/

void MusicDatabase::generateIndex() {

	lock_guard<mutex>{dbMutex};

	File f { File::getCacheDir() + "index.dat" };

	if(f.exists()) {
		auto sz = f.read<uint32_t>();
		titleToComposer.resize(sz);
		f.read((uint8_t*)&titleToComposer[0], titleToComposer.size()*sizeof(uint32_t));

		sz = f.read<uint32_t>();
		composerToTitle.resize(sz);
		f.read((uint8_t*)&composerToTitle[0], composerToTitle.size()*sizeof(uint32_t));

		sz = f.read<uint32_t>();
		composerTitleStart.resize(sz);
		f.read((uint8_t*)&composerTitleStart[0], composerTitleStart.size()*sizeof(uint32_t));

		titleIndex.load(f);
		composerIndex.load(f);
		f.close();
		return;
	}

	print_fmt("Creating Search Index...\n");

	string oldComposer;
	auto query = db.query<string, string, string, string, string>("SELECT title, game, format, composer, path FROM song");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;

	titleToComposer.reserve(438000);
	composerToTitle.reserve(37000);
	titleIndex.reserve(438000);
	composerIndex.reserve(37000);

	int step = 438000 / 20;

	unordered_map<string, vector<uint32_t>> composers;

	while(count < 1000000) {
		count++;
		if(!query.step())
			break;

		if(count % step == 0) {
			LOGD("%d songs indexed", count);
		}

		string title, game, fmt, composer, path;
		tie(title, game, fmt, composer, path) = query.get_tuple();

		formats.push_back(formatToByte(fmt));

		//LOGD("%s %s", title, path);
		//string ext = path_extension(path);
		//makeLower(ext);
		//int fmt = formatMap[ext];
		//formats.push_back(fmt);
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

	// We need: list of titles for each composer in composerIndex;
	// One big vector of indexes, one smaller f
	composerTitleStart.resize(composers.size());
	for(const auto &p : composers) {
		// p,first == composer, p.second == vector
		auto cindex = p.second[0];
		composerTitleStart[cindex] = composerToTitle.size();
		for(int i=1; i<(int)p.second.size(); i++)
			composerToTitle.push_back(p.second[i]);
		composerToTitle.push_back(-1);
	}

	LOGD("INDEX CREATED (%d) (%d)", titleToComposer.size(), composerToTitle.size());

	//return;

	//File f { "index.dat" };

	f.write<uint32_t>(titleToComposer.size());
	f.write((uint8_t*)&titleToComposer[0], titleToComposer.size()*sizeof(uint32_t));

	f.write<uint32_t>(composerToTitle.size());
	f.write((uint8_t*)&composerToTitle[0], composerToTitle.size()*sizeof(uint32_t));

	f.write<uint32_t>(composerTitleStart.size());
	f.write((uint8_t*)&composerTitleStart[0], composerTitleStart.size()*sizeof(uint32_t));

	titleIndex.dump(f);
	composerIndex.dump(f);
	f.close();
}
}
