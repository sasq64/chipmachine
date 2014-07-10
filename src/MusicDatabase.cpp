#include "MusicDatabase.h"
//#include "secondary.h"
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>

using namespace std;
using namespace utils;

namespace chipmachine {

static const std::set<std::string> secondary = { "smpl", "sam", "ins", "smp" };

void MusicDatabase::initDatabase(unordered_map<string, string> &vars) {

	auto name = vars["type"];
	LOGD("Init db '%s'", name);
	if(name == "modland")
		modlandInit(vars["source"], vars["song_list"], vars["exclude_formats"], stol(vars["id"]));
	else if(name == "hvsc")
		hvscInit(vars["source"], stol(vars["id"]));
	else if(name == "rsn")
		rsnInit(vars["source"], stol(vars["id"]));
}

void MusicDatabase::rsnInit(const string &source, int id) {


	if(db.query("SELECT 1 FROM collection WHERE name = 'RSNSET'").step())
		return;

	//LOGD("Indexing RSN");
	print_fmt("Creating RSN database\n");

	db.exec("BEGIN TRANSACTION");

	string rsnPath = source;
	if(!endsWith(rsnPath, "/"))
		rsnPath += "/";

	db.exec("INSERT INTO collection (name, url, description, id) VALUES (?, ?, ?, ?)",
		"RSNSET", rsnPath, "SNES RSN set", id);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	vector<uint8_t> buffer(0xd8);

	makedir(".rsntemp");

	File root { source };

	for(const auto &rf : root.listFiles()) {
		auto name = rf.getName();
		//LOGD("########### NAME:%s", name);
		if(path_extension(name) == "rsn") {
			auto *a = Archive::open(name, ".rsntemp", Archive::TYPE_RAR);
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
						auto game = string((const char*)&buffer[0x4e], 0x20);
						auto composer = string((const char*)&buffer[0xb1], 0x20);

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

						auto pos = name.find(rsnPath);
						if(pos != string::npos) {
							name = name.substr(pos + rsnPath.length());
						}
						LOGD("### %s : %s - %s", name, composer, game);

						query.bind("", game, composer, "Super Nintendo", name, id);
						query.step();

						break;
						//done = true;
					}
				}
			}
			delete a;
		}
	}
	db.exec("COMMIT");

}

void MusicDatabase::hvscInit(const string &source, int id) {

	if(db.query("SELECT 1 FROM collection WHERE name = 'HVSC'").step())
		return;

	//db.exec("CREATE TABLE IF NOT EXISTS hvscstil (title STRING)");

	//LOGD("Indexing HVSC");
	print_fmt("Creating HVSC database\n");

	db.exec("BEGIN TRANSACTION");

	string hvscPath = source;
	if(!endsWith(hvscPath, "/"))
		hvscPath += "/";

	db.exec("INSERT INTO collection (name, url, description, id) VALUES (?, ?, ?, ?)",
		"HVSC", hvscPath, "HVSC database", id);

	vector<uint8_t> buffer(128);
	//char temp[33];
	//temp[32] = 0;

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	function<void(File &)> checkDir;
	checkDir = [&](File &root) {
		LOGD("Checking %s", root.getName());
		for(auto f : root.listFiles()) {
			auto name = f.getName();
			if(path_extension(f.getName()) == "sid") {
				f.read(&buffer[0], buffer.size());
				auto title = string((const char*)&buffer[0x16], 0x20);
				auto composer = string((const char*)&buffer[0x36], 0x20);
				//auto copyright = string((const char*)&buffer[0x56], 0x20);

				auto pos = name.find(hvscPath);
				if(pos != string::npos) {
					name = name.substr(pos + hvscPath.length());
				}

				query.bind(title, "", composer, "Commodore 64", name, id);
				query.step();
				f.close();
			} else if(f.isDir())
				checkDir(f);
		}
	};
	auto hf = File(hvscPath);
	checkDir(hf);

	db.exec("COMMIT");
}


#ifdef RASPBERRYPI
static std::unordered_set<std::string> exclude = {
	"Nintendo DS Sound Format", "Gameboy Sound Format", "Dreamcast Sound Format", "Ultra64 Sound Format",
	"Saturn Sound Format" /*, "RealSID", "PlaySID" */
};
#else
static std::unordered_set<std::string> exclude = { /*"RealSID", "PlaySID",*/ "Saturn Sound Format" };
#endif


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
	auto l = parts.size();
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

void MusicDatabase::modlandInit(const string &source, const string &song_list, const string &xformats, int id) {

	if(db.query("SELECT 1 FROM collection WHERE name = 'modland'").step())
		return;

	//LOGD("Creating modland collection");
	print_fmt("Creating Modland database\n");

	auto ex_copy = exclude;
	auto parts = split(xformats, ";");
	for(const auto &p : parts) {
		if(p.length())
			ex_copy.insert(p);
	}

	string url = source;
	if(!endsWith(url, "/"))
		url += "/";

	db.exec("BEGIN TRANSACTION");

	db.exec("INSERT INTO collection (name, url, description, id) VALUES (?, ?, ?, ?)",
		"modland", url, "Modland database", id);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	File file { song_list };
	for(const auto &s : file.getLines()) {

		SongInfo song(split(s, "\t")[1]);

		if(!parseModlandPath(song))
			continue;
		if(ex_copy.count(song.format) > 0)
			continue;

		query.bind(song.title, song.game, song.composer, song.format, song.path, id);
		query.step();
	}
	db.exec("COMMIT");

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

string MusicDatabase::getFullString(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string, string, string>("SELECT title, game, composer, format, song.path, collection.url FROM song, collection WHERE song.ROWID = ? AND song.collection = collection.id", id);
	if(q.step()) {
		auto t = q.get_tuple();
		auto title = get<0>(t);
		auto game = get<1>(t);
		if(game != "")
			title = format("%s [%s]", game, title);

		string r = format("%s\t%s\t%s\t%s", get<5>(t) + get<4>(t), title, get<2>(t), get<3>(t));
		LOGD("RESULT %s", r);
		return r;
	}
	throw not_found_exception();
}

SongInfo MusicDatabase::pathToSongInfo(const std::string &path) {
	SongInfo song(path);
	auto coll = stripCollectionPath(song.path);
	song.path = path;
	if(coll == "modland")
		parseModlandPath(song);
	return song;
}

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
/*
enum Formats {

	GAME = 0x20,
	NINTENDO,

	ATARI

	ADPLUG

	PLAYSTATION
	DREAMCAST
	NINTENDO64

	SID = 0x30

	TRACKER = 0x10,
	PROTRACKER,
	SCREAMTRACKER,
	IMPULSETRACKER,
	FASTTRACKER,

	UADE,
};
*/
static uint8_t formatToByte(const std::string &f) {
	return 0;
}


string MusicDatabase::stripCollectionPath(string &path) {
	vector<Collection> colls;
	auto q = db.query<string, string>("SELECT name,url FROM collection");
	while(q.step()) {
		colls.push_back(q.get<Collection>());
	}
	for(const auto &c : colls) {
		if(startsWith(path, c.url)) {
			path = path.substr(c.url.length());
			return c.name;
		}
	}
	return "";
}

void MusicDatabase::generateIndex() {

	lock_guard<mutex>{dbMutex};

	File f { "index.dat" };

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
		for(int i=1; i<p.second.size(); i++)
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
