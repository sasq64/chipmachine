#include "log.h"

#include "SongDb.h"

#include "sqlite3/sqlite3.h"
#include "utils.h"

#include <cstring>

using namespace std;
using namespace utils;

void IncrementalQuery::addLetter(char c) {		
	query.push_back(c);	
	//parts = split(query, " ");	
	if(query.size() > 2) {
		search();
	}
}

void IncrementalQuery::removeLast() {
	query.pop_back();
	if(query.size() > 2) {
		search();
	}		
}
const string IncrementalQuery::getString() { 
	return string(&query[0], query.size());
}

const vector<string> &IncrementalQuery::getResult() const {
	return result;
}
int IncrementalQuery::numHits() const {
	return result.size();
}

void IncrementalQuery::search() {
	sqlite3_stmt *s;
	char temp[128] = "%%";
	//sprintf(temp, "%%%s%%", &query[0]);
	int sz = query.size();
	memcpy(&temp[1], &query[0], sz);
	temp[sz+1] = '%';
	temp[sz+2] = 0;
	const char *tail;

	result.resize(0);

	LOGD("Searching for '%s'\n", temp);
	int rc = sqlite3_prepare_v2(db, "SELECT path, title, composer, metadata FROM songs, composers WHERE songs.composer_id == composers._id AND title LIKE ? OR composer LIKE ?;", -1, &s, &tail);
	//"select PATH,TITLE,COMPOSER,METADATA from songs where (TITLE like ? or COMPOSER like ?) and where ID in (select ID from sings where (TITLE like ? or COMPOSER like ?));"
	LOGD("Result '%d'\n", rc);
	if(rc == SQLITE_OK) {
		sqlite3_bind_text(s, 1, temp, -1, SQLITE_STATIC);
		sqlite3_bind_text(s, 2, temp, -1, SQLITE_STATIC);

		while(true) {
			int ok = sqlite3_step(s);
			if(ok == SQLITE_ROW) {
				const char *path = (const char *)sqlite3_column_text(s, 0);
				const char *title = (const char *)sqlite3_column_text(s, 1);
				const char *composer = (const char *)sqlite3_column_text(s, 2);
				const char *metadata = (const char *)sqlite3_column_text(s, 3);
				//printf("%p %p %p %p\n", path, title, composer, metadata);
				//printf("%s %s %s %s\n", path, title, composer, metadata);
				result.push_back(format("%s\t%s\t%s\t%s", path, title, composer, metadata));
			} else if(ok == SQLITE_DONE) {
				break;
			}
			if(result.size() >= searchLimit)
				break;
		}
	}
}

SongDatabase::SongDatabase(const string &name) {
	db = nullptr;
	int rc = sqlite3_open(name.c_str(), &db);
	if(rc == SQLITE_OK) {

	};
}

void SongDatabase::addSubStrings(const char *words, unordered_map<string, std::vector<int>> &stringMap, int index) {
	//auto parts = ansplit(title)
	const char *ptr = words;
	unsigned int sl = strlen(words);
	if(sl >= 3) {
		for(unsigned int i=0; i<sl-2; i++) {
			char c = words[i];
			const char *pc = &words[i];
			if(!isalnum(c)) {
				ptr = &words[i+1];
				continue;
			}

			if(pc - ptr == 3) {
				// TODO: 3 letters could be encoded to one unsigned short
				string tl = string(ptr, 3);
				makeLower(tl);
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
void SongDatabase::generateIndex() {

	sqlite3_stmt *s;
	const char *tail;
	char oldComposer[256] = "";

	int rc = sqlite3_prepare_v2(db, "select TITLE, COMPOSER from SONGS order by COMPOSER;", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc == SQLITE_OK) {
		int count = 0;
		int maxTotal = 3;
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
		LOGD("%d titles by %d composer generated %d + %d 3 letter indexes\n", titles.size(), composers.size(), titleMap.size(), composerMap.size());
		//for(auto entry : tlmap) {
		//	LOGD("%s : %d times\n", entry.first, entry.second);
		//}
	}
}

void SongDatabase::search(const char *query) {

	if(strlen(query) < 3)
		return;

	string t = string(query, 3);
	makeLower(t);

	const auto &tv = titleMap[t];

	LOGD("Searching %d candidates", tv.size());
	for(auto index : tv) {
		string title = titles[index].first;
		makeLower(title);
		if(title.find(query) != string::npos) {

			LOGD("%s - %s", titles[index].first, composers[titles[index].second].first);
		}
	}



}
