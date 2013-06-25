#include "log.h"

#include "SongDb.h"

#include "sqlite3/sqlite3.h"
#include "utils.h"

#include <cstring>
#include <algorithm>
#include <set>

#include <iconv.h>

using namespace std;
using namespace utils;


SongDatabase::SongDatabase(const string &name) {
	db = nullptr;


	int rc = sqlite3_open(name.c_str(), &db);//, SQLITE_READONLY, NULL);
	if(rc != SQLITE_OK) {	
		throw database_exception(format("%s: %s", name, sqlite3_errstr(rc)).c_str());
	};
}

SongDatabase::~SongDatabase() {
	if(db)
		sqlite3_close(db);
}


string SongDatabase::getFullString(int id) {

	id++;
	LOGD("ID %d", id);

	sqlite3_stmt *s;
	const char *tail;
	int rc = sqlite3_prepare_v2(db, "SELECT title, composer, path, metadata FROM songs WHERE _id == ?", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc != SQLITE_OK)
		throw database_exception("Select failed");
	sqlite3_bind_int(s, 1, id);
	int ok = sqlite3_step(s);
	
	if(ok == SQLITE_ROW) {
		const char *title = (const char *)sqlite3_column_text(s, 0);
		const char *composer = (const char *)sqlite3_column_text(s, 1);
		const char *path = (const char *)sqlite3_column_text(s, 2);
		const char *metadata = (const char *)sqlite3_column_text(s, 3);

		LOGD("Result %s %s %s\n", title, composer, path);
		string r = format("%s\t%s\t%s\t%s", title, composer, path, metadata);
		sqlite3_finalize(s);
		return r;
	} else {
		sqlite3_finalize(s);
		throw not_found_exception();
	}
	//return "";
}
enum {
	UNKNOWN = 0,
	C64_SID = 1,
	TRACKER_ANY = 0x10,
	TRACKER_MOD,
	TRACKER_XM,
	TRACKER_S3M,
	TRACKER_FT,
	TRACKER_IT,

	TRACKER_END = 0x2f,
	GM_ANY = 0x30,
	GM_NES,
	GM_SNES,
	GM_GAMEBOY,
	GM_VGM,
	GM_END = 0x4f,

	AUDIO_ANY = 0x50,

	AMI_ANY = 0x80,
	AMI_TFMX,
	AMI_CUSTOM,
	AMI_FC,
	AMI_AHX,
	AMI_END = 0xff
};



unordered_map<string, int> formatMap {
	{ "sid", C64_SID },
	{ "rsid", C64_SID },
	{ "psid", C64_SID },
	{ "mod", TRACKER_MOD },
	{ "xm", TRACKER_XM },
	{ "it", TRACKER_IT },
	{ "s3m", TRACKER_S3M },
	{ "nsf", GM_NES },
	{ "smc", GM_SNES },
	{ "spc", GM_SNES },
	{ "gbs", GM_GAMEBOY },
	{ "vgz", GM_VGM },
	{ "ahx", AMI_AHX },
};	


void SongDatabase::generateIndex() {

	sqlite3_stmt *s;
	const char *tail;
	char oldComposer[256] = "";

	int rc = sqlite3_prepare_v2(db, "SELECT title, composer, path FROM songs;", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc != SQLITE_OK)
		throw database_exception("Select failed");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;
	while(count < 1000000) {
		count++;
		int ok = sqlite3_step(s);
		if(ok == SQLITE_ROW) {
			const char *title = (const char *)sqlite3_column_text(s, 0);
			const char *composer = (const char *)sqlite3_column_text(s, 1);
			const char *path = (const char *)sqlite3_column_text(s, 2);

			string ext = path_extention(path);

			int fmt = formatMap[ext];

			formats.push_back(fmt);


			int tindex = titleIndex.add(title);

			if(strcmp(composer, oldComposer) != 0) {
				strcpy(oldComposer, composer);
				cindex = composerIndex.add(composer);
				composerToTitle.push_back(tindex);
			}

			titleToComposer.push_back(cindex);

		} else if(ok == SQLITE_DONE) {
			break;
		}
	}

	sqlite3_finalize(s);

	//LOGD("%d titles by %d composer generated %d + %d 3 letter indexes\n", titles.size(), composers.size(), titleMap.size(), composerMap.size());
}
int SongDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

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

#ifdef UNIT_TEST

#include "catch.hpp"
#include <sys/time.h>

TEST_CASE("db::index", "Generate index") {

	SongDatabase db {"hvsc.db"};
	db.generateIndex();

	string search_string = "tune tel fre";
	vector<int> results { 155, 1, 2944, 2694, 2694, 1954, 524, 11, 11, 1, 1, 1 };
	IncrementalQuery q = db.find();
	int i = 0;
	for(char c : search_string) {		
		q.addLetter(c);
		//LOGD("%s %d", q.getString(), q.numHits());
		REQUIRE(q.numHits() == results[i]);
		i++;
	}
	string res = q.getResult(0, 10)[0];
	REQUIRE(res.find("Freaky Tune") != string::npos);
}

/*
TEST_CASE("db::tlcode", "Codes") {

	SongDatabase db {"hvsc.db"};
	unordered_map<uint16_t, std::vector<int>> stringMap;
	logging::setLevel(logging::VERBOSE);
	db.addSubStrings("Testing (4-Mat)", stringMap, 0);
	logging::setLevel(logging::DEBUG);
}*/


TEST_CASE("db::find", "Search Benchmark") {

	timeval tv;
	gettimeofday(&tv, NULL);
	long d0 = tv.tv_sec * 1000000 + tv.tv_usec;

	logging::setLevel(logging::INFO);

	SongDatabase db {"hvsc.db"};
	db.generateIndex();


	vector<IncrementalQuery> iqs;

	for(int i=0; i<20; i++) {
		iqs.push_back(db.find());
	}

	string search_string = "tune tel";
	int i = 0;

	gettimeofday(&tv, NULL);
	long d1 = tv.tv_sec * 1000000 + tv.tv_usec;

	for(char c : search_string) {		
		for(auto &q : iqs) {
			q.addLetter(c);			
			//REQUIRE(q.numHits() == results[i]);
		}
		i++;
	}

	gettimeofday(&tv, NULL);
	long d2 = tv.tv_sec * 1000000 + tv.tv_usec;

	LOGI("Search took %dms", (d2-d1)/1000);
}

#endif
