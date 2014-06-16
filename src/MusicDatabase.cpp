#include "MusicDatabase.h"
#include "secondary.h"
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>

using namespace std;
using namespace utils;

namespace chipmachine {

static const std::set<std::string> secondary = { "smpl", "sam", "ins", "smp" };


void MusicDatabase::init() {


	//if(db.has_table("song")) {
	//	generateIndex();
	//	return;
	//}

	db.exec("CREATE TABLE IF NOT EXISTS playlist (title STRING)");
	db.exec("CREATE TABLE IF NOT EXISTS plmap (playlist INT, pos INT, path STRING)");
	db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, description STRING, id INTEGER, version INTEGER)");
	db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, format STRING, path STRING, collection INTEGER)");
	//db.exec("CREATE VIRTUAL TABLE song USING fts4(title, game, composer, format, path,)");

	//hvscInit();
	//modlandInit();
	//generateIndex();

}

void MusicDatabase::initDatabase(string name, unordered_map<string, string> &vars) {
	LOGD("Init db '%s'", name);
	if(name == "modland")
		modlandInit(vars["source"], vars["song_list"], stol(vars["id"]));
	else if(name == "hvsc")
		hvscInit(vars["source"], stol(vars["id"]));
	else if(name == "rsn")
		rsnInit(vars["source"], stol(vars["id"]));
}

void MusicDatabase::rsnInit(const string &source, int id) {


	if(db.query("SELECT 1 FROM collection WHERE name = 'RSNSET'").step())
		return;

	LOGD("Indexing RSN");

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

	LOGD("Indexing HVSC");

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
	"Saturn Sound Format", "RealSID", "PlaySID"
};
#else
static std::unordered_set<std::string> exclude = { "RealSID", "PlaySID" };
#endif

void MusicDatabase::modlandInit(const string &source, const string &song_list, int id) {

	if(db.query("SELECT 1 FROM collection WHERE name = 'modland'").step())
		return;

	LOGD("Creating modland collection");

	LOGD("Indexing...\n");

	static const unordered_set<string> hasSubFormats = {
		"Ad Lib",
		"Video Game Music"
	};

	string url = source;
	if(!endsWith(url, "/"))
		url += "/";

	db.exec("BEGIN TRANSACTION");

	db.exec("INSERT INTO collection (name, url, description, id) VALUES (?, ?, ?, ?)",
		"modland", url, "Modland database", id);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	File file { song_list };
	for(const auto &s : file.getLines()) {

		auto path = split(s, "\t")[1];

		string ext = path_extension(path);
		if(secondary.count(ext) > 0) {
			continue;
		}

		auto parts = split(path, "/");
		auto l = parts.size();
		if(l < 3) {
			LOGD("%s", path);
			continue;
		}

		int i = 0;
		string fmt = parts[i++];
		if(hasSubFormats.count(fmt) > 0)
			i++;//fmt = fmt + "/" + parts[i++];

		if(exclude.count(fmt) > 0)
			continue;

		string composer = parts[i++];

		if(fmt == "MDX") {
			i--;
			composer = "?";
		}

		if(composer == "- unknown")
			composer = "?";

		if(parts[i].substr(0, 5) == "coop-")
			composer = composer + "+" + parts[i++].substr(5);

		string game;
		if(l-i == 2)
			game = parts[i++];

		if(endsWith(parts[i], ".rar"))
			parts[i] = parts[i].substr(0, parts[i].length()-4);

		string title = path_basename(parts[i++]);

		query.bind(title, game, composer, fmt, path, id);
		
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

	titleIndex.search(query, result, searchLimit);

	vector<int> cresult;
	composerIndex.search(query, cresult, searchLimit);
	for(int index : cresult) {
		int title_index = composerToTitle[index];

		while(titleToComposer[title_index] == index) {
			result.push_back(title_index++);
		}
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

vector<SongInfo> MusicDatabase::find(const string &pattern) {

	lock_guard<mutex> guard(dbMutex);

	vector<SongInfo> songs;

	auto parts = split(pattern, " ");
	string qs = "SELECT path,game,title,composer,format FROM song WHERE ";
	for(int i=0; i<(int)parts.size(); i++) {
		if(i > 0) {
			qs.append("AND path LIKE ?");
			parts[i] = format("%%%s%%",parts[i]);
		} else
			qs.append("path MATCH ?");
	}
	//qs.append(" COLLATE NOCASE");
	LOGD("Query: %s", qs);
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

		titleIndex.load(f);
		composerIndex.load(f);
		f.close();
		return;
	}

	string oldComposer;
	auto query = db.query<string, string, string, string>("SELECT title, game, composer, path FROM song");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;

	titleToComposer.reserve(438000);
	composerToTitle.reserve(37000);
	titleIndex.reserve(438000);
	composerIndex.reserve(37000);

	int step = 438000 / 20;

	while(count < 1000000) {
		count++;
		if(!query.step())
			break;

		if(count % step == 0) {
			LOGD("%d songs indexed", count);
		}

		string title, game, composer, path;
		tie(title, game, composer, path) = query.get_tuple();
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

		if(composer != oldComposer) {
			oldComposer = composer;
			cindex = composerIndex.add(composer);
			// The composer index does not map to the database, but for each composer
			// we save the first index in the database for that composer
			composerToTitle.push_back(tindex);
		}

		// We also need to find the composer for a give title
		titleToComposer.push_back(cindex);
	}
	LOGD("INDEX CREATED (%d) (%d)", titleToComposer.size(), composerToTitle.size());

	//return;

	//File f { "index.dat" };

	f.write<uint32_t>(titleToComposer.size());
	f.write((uint8_t*)&titleToComposer[0], titleToComposer.size()*sizeof(uint32_t));

	f.write<uint32_t>(composerToTitle.size());
	f.write((uint8_t*)&composerToTitle[0], composerToTitle.size()*sizeof(uint32_t));

	titleIndex.dump(f);
	composerIndex.dump(f);
	f.close();
}
}
