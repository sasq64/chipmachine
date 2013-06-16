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

	sdb->search(string(&query[0], query.size()), result, searchLimit);

/*	sqlite3_stmt *s;
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
	}*/
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
void SongDatabase::generateIndex() {

	sqlite3_stmt *s;
	const char *tail;
	char oldComposer[256] = "";

	int rc = sqlite3_prepare_v2(db, "select TITLE, COMPOSER from SONGS order by COMPOSER;", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc == SQLITE_OK) {
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
		LOGD("%d titles by %d composer generated %d + %d 3 letter indexes\n", titles.size(), composers.size(), titleMap.size(), composerMap.size());
		//for(auto entry : tlmap) {
		//	LOGD("%s : %d times\n", entry.first, entry.second);
		//}
	}
}


void SongDatabase::search(const string &query, vector<string> &result, int searchLimit) {

	result.resize(0);
	if(query.size() < 3)
		return;
	string t = string(query, 0, 3);
	makeLower(t);

	LOGD("Checking '%s/%s' among %d+%d sub strings", query, t, titleMap.size(), composerMap.size());

	const auto &tv = titleMap[t];
	const auto &cv = composerMap[t];
	LOGD("Searching %d candidates", cv.size());
	for(int index : cv) {
		string composer = composers[index].first;
		makeLower(composer);
		if(composer.find(query) != string::npos) {
			int title_index = composers[index].second;
			while(true) {
				auto title = titles[title_index];
				if(title.second != index)
					break;
				//LOGD("%s / %s", title.first, composers[index].first);
				result.push_back(format(".\t%s\t%s\t", title.first, composers[index].first));
				title_index++;
				if(result.size() >= searchLimit)
					break;
			}
		}
	}

	LOGD("Searching %d candidates", tv.size());
	for(int index : tv) {
		string title = titles[index].first;
		makeLower(title);
		if(title.find(query) != string::npos) {
			result.push_back(format(".\t%s\t%s\t", titles[index].first, composers[titles[index].second].first));
			//LOGD("%s / %s", titles[index].first, composers[titles[index].second].first);
		}
		if(result.size() >= searchLimit)
			break;
	}
}

void SongDatabase::search(const char *query) {

	//if(strlen(query) < 3)
	//	return;

	auto parts = split(query, " ");
	for(auto &p : parts) {
		makeLower(p);
	}
	sort(parts.begin(), parts.end(), [](const string &a, const string &b) {
		return a.length() > b.length();
	});

	if(parts[0].length() < 3)
		return;

	string t = string(parts[0], 3);
	//makeLower(t);

	const auto &tv = titleMap[t];
	const auto &cv = composerMap[t];
	LOGD("Searching %d candidates", cv.size());
	for(int index : cv) {
		string composer = composers[index].first;
		makeLower(composer);
		bool found = true;
		for(p : parts) {
			if(composer.find(p) == string::npos) {
				found = false;
				break;
			}
		}
		if(!found)
			break;
		int title_index = composers[index].second;
		while(true) {
			auto title = titles[title_index];
			if(title.second != index)
				break;
			LOGD("%s / %s", title.first, composers[index].first);
			title_index++;
		}
	}

	LOGD("Searching %d candidates", tv.size());
	for(int index : tv) {
		string title = titles[index].first;
		makeLower(title);
		if(title.find(query) != string::npos) {

			LOGD("%s / %s", titles[index].first, composers[titles[index].second].first);
		}
	}



}
