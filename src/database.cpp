#include "database.h"

using namespace std;
using namespace utils;

void ModlandDatabase::init() {

	if(db.has_table("song")) {
		generateIndex();
		return;
	}

	LOGD("Indexing...\n");

	static const unordered_set<string> ownDirFormats = {
		"Dreamcast Sound Format",
		"Gameboy Sound Format",
		"Nintendo Sound Format",
		"Nintendo SPC",
		"Video Game Music",
		"Ultra64 Sound Format",
		"Playstation Sound Format"
	};

	//db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, format STRING, path STRING)");
	db.exec("CREATE VIRTUAL TABLE song USING fts4(title, game, composer, format, path)");

	auto query = db.query("INSERT INTO song (title, game, composer, format, path) VALUES (?, ?, ?, ?, ?)");

	db.exec("BEGIN TRANSACTION");
	File file { "allmods.txt" };
	for(const auto &s : file.getLines()) {

		auto path = split(s, "\t")[1];
		auto parts = split(path, "/");
		auto l = parts.size();
		if(l < 3) {
			LOGD("%s", path);
			continue;
		}

		int i = l-1;
		bool ownDir = (ownDirFormats.count(parts[0]) > 0);

		string title = path_basename(parts[i--]);
		string game;

		if(ownDir && l >= 4) {
			game = parts[i--];
		}

		string composer = parts[i--];

		if(composer == "- unknown")
			composer = "?";

		if(composer.substr(0, 5) == "coop-")
			composer = parts[i--] + "+" + composer.substr(5);

		string format = parts[i--];

		query.bind(title, game, composer, format, path);
		
		query.step();
	}
	db.exec("COMMIT");

	generateIndex();
}


int ModlandDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

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

string ModlandDatabase::getFullString(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string>("SELECT title, composer, path FROM song WHERE ROWID = ?", id);
	if(q.step()) {
		auto t = q.get_tuple();
		string r = format("%s\t%s\t%s", get<0>(t), get<1>(t), get<2>(t));
		LOGD("RESULT %s", r);
		return r;
	}
	throw not_found_exception();
}

vector<SongInfo> ModlandDatabase::find(const string &pattern) {

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

void ModlandDatabase::generateIndex() {

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
	auto query = db.query<string, string, string>("SELECT title, composer, path FROM song");

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

		string title, composer, path;
		tie(title, composer, path) = query.get_tuple();
		//string ext = path_extention(path);
		//makeLower(ext);
		//int fmt = formatMap[ext];
		//formats.push_back(fmt);

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
