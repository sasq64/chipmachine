#include "log.h"

#include "SongDb.h"

#include "sqlite3/sqlite3.h"
#include "utils.h"

#include <cstring>
#include <algorithm>

using namespace std;
using namespace utils;

void IncrementalQuery::addLetter(char c) {
	query.push_back(c);	
	if(query.size() > 2) {
		search();
	}
}

void IncrementalQuery::removeLast() {
	if(!query.empty()) {
		query.pop_back();
		if(query.size() > 2) {
			search();
		}
	}
}

void IncrementalQuery::clear() {
	query.resize(0);
}

const string IncrementalQuery::getString() { 
	return string(&query[0], query.size());
}

const vector<string> &IncrementalQuery::getResult() const {
	return finalResult;
}
int IncrementalQuery::numHits() const {
	return finalResult.size();
}

void IncrementalQuery::search() {

	string q = string(&query[0], query.size());

	auto parts = split(q, " ");
	// Remove empty strings
	parts.erase(remove_if(parts.begin(), parts.end(), [&](const string &a) { return a.size() == 0; }), parts.end());
	LOGD("Parts: [%s]", parts);

	if(oldParts.size() == 0 || string(oldParts[0], 0, 3) != string(parts[0], 0, 3)) {
		sdb->search(string(parts[0], 0, 3), firstResult, searchLimit);
	}
	oldParts = parts;

	finalResult.resize(0);
	for(auto &r : firstResult) {
		string rc = r;
		makeLower(rc);
		bool found = true;
		for(auto p : parts) {
			if(rc.find(p) == string::npos) {
				found = false;
				break;
			}
		}
		if(found)
			finalResult.push_back(r);
	}
}

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

void SongDatabase::addSubStrings(const char *words, unordered_map<string, std::vector<int>> &stringMap, int index) {
	//auto parts = ansplit(title)
	const char *ptr = words;
	char tl[4];
	tl[3] = 0;
	unsigned int sl = strlen(words);
	if(sl >= 3) {
		for(unsigned int i=0; i<sl; i++) {
			char c = words[i];
			const char *pc = &words[i];
			if(!isalnum(c)) {
				ptr = &words[i+1];
				continue;
			}

			if(pc - ptr == 2) {
				// TODO: 3 letters could be encoded to one unsigned short
				//string tl = string(ptr, 3);
				//makeLower(tl);
				tl[0] = tolower(ptr[0]);
				tl[1] = tolower(ptr[1]);
				tl[2] = tolower(ptr[2]);
				stringMap[tl].push_back(index);
				ptr++;
			}
		}
	}
}
/*

Logic read all titles and composers into 2 arrays
At the same time create 2 3L-maps referring those arrays

*/

string IncrementalQuery::getFull(int pos) {

	string r = finalResult[pos];
	auto parts = split(r, "\t");
	int id = atoi(parts[2].c_str());

	LOGD("ID %d", id);

	id++;

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
		sqlite3_finalize(s);
		return format("%s\t%s\t%s\t%s", title, composer, path, metadata);
	} else {
		sqlite3_finalize(s);
		throw not_found_exception();
	}
	//return "";
}

void SongDatabase::generateIndex() {

	sqlite3_stmt *s;
	const char *tail;
	char oldComposer[256] = "";

	int rc = sqlite3_prepare_v2(db, "SELECT title, composer FROM songs;", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc != SQLITE_OK)
		throw database_exception("Select failed");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;
	while(count < 100000) {
		count++;
		int ok = sqlite3_step(s);
		if(ok == SQLITE_ROW) {
			const char *title = (const char *)sqlite3_column_text(s, 0);
			const char *composer = (const char *)sqlite3_column_text(s, 1);

			int tindex = titles.size();

			if(strcmp(composer, oldComposer) != 0) {
				composers.push_back(make_pair(composer, tindex));
				cindex = composers.size()-1;
				strcpy(oldComposer, composer);

				addSubStrings(composer, composerMap, cindex);
			}

			titles.push_back(make_pair(title, cindex));


			addSubStrings(title, titleMap, tindex);


		} else if(ok == SQLITE_DONE) {
			break;
		}
	}

	sqlite3_finalize(s);

	LOGD("%d titles by %d composer generated %d + %d 3 letter indexes\n", titles.size(), composers.size(), titleMap.size(), composerMap.size());
}
/*
static const vector<const string>> t9array { "", "", "abc", "def", "ghi", "jkl", "mno", "prqs", "tuv", "wxyz" };

vector<string> makeT9(const string &numbers) {

	for(auto s : numbers) {
		int n = atoi(s.c_str());
		for(auto letters : t9array[n]) {

		}
	}
}

void SongDatabase::t9search(const string &query, vector<string> &result, unsigned int searchLimit) {



}
*/

void SongDatabase::search(const string &query, vector<string> &result, unsigned int searchLimit) {

	result.resize(0);
	//if(query.size() < 3)
	//	return;
	//string t = string(query, 0, 3);
	//makeLower(t);

	LOGD("Checking '%s' among %d+%d sub strings", query, titleMap.size(), composerMap.size());

	const auto &tv = titleMap[query];
	const auto &cv = composerMap[query];

	LOGD("Searching %d candidates", tv.size());
	for(int index : tv) {
		result.push_back(format("%s\t%s\t%d", titles[index].first, composers[titles[index].second].first, index));
		if(result.size() >= searchLimit)
			break;
	}

	LOGD("Searching %d candidates", cv.size());
	for(int index : cv) {
		int title_index = composers[index].second;
		while(true) {
			auto title = titles[title_index];
			if(title.second != index)
				break;
			//LOGD("%s / %s", title.first, composers[index].first);
			result.push_back(format("%s\t%s\t%d", title.first, composers[index].first, title_index));
			title_index++;
			if(result.size() >= searchLimit)
				break;
		}
	}
}

void SongDatabase::search(const char *query) {
}
