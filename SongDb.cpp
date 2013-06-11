#include "log.h"
/*
	telnet.addCommand("play", [&](vector<string> args) {
		ChipPlayer *player = psys.play(name);
	});

	sqlite3 *db = nullptr;

	int rc = sqlite3_open("hvsc.db", &db);
	if(rc == SQLITE_OK) {
		printf("DB opened\n");
		sqlite3_stmt *s;
		const char *tail;
		rc = sqlite3_prepare(db, "select * from songs;", -1, &s, &tail);
		if(rc == SQLITE_OK) {
			printf("Statement created\n");
			while(true) {
				sqlite3_step(s);
				const char *title = (const char *)sqlite3_column_text(s, 2);
				printf("title %s\n", title);
			}
		} else
			printf("%s\n", sqlite3_errmsg(db));
	} else
		printf("%s\n", sqlite3_errmsg(db));

Runtime interface. Constucted from db-file
   DB FORMAT
   _index, PATH, COMPOSER, TITLE, METADATA
	Return format
PATH\tCOMPOSER\tTITLE\tMETADATA
*/

#include <string>
#include <vector>

#include "sqlite3/sqlite3.h"
#include "utils.h"


class IncrementalQuery {
public:

	IncrementalQuery(sqlite3 *db) : db(db), searchLimit(40) {
	}

	void addLetter(char c) {		
		query.push_back(c);		
		if(query.size() > 2) {
			search();
		}
	}

	void removeLast() {
		query.pop_back();
		if(query.size() > 2) {
			search();
		}		
	}
	const std::string getString() { 
		return string(&query[0], query.size());
	}

	const std::vector<std::string> &getResult() const {
		return result;
	}
	int numHits() {
		return result.size();
	}
private:

	
	void search() {
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
		int rc = sqlite3_prepare_v2(db, "select PATH,TITLE,COMPOSER,METADATA from songs where TITLE like ? or COMPOSER like ?;", -1, &s, &tail);
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
					result.push_back(utils::format("%s\t%s\t%s\t%s", path, title, composer, metadata));
				} else if(ok == SQLITE_DONE) {
					break;
				}
				if(result.size() >= searchLimit)
					break;
			}
		}
	}
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> result;
	sqlite3 *db;
};



class SongDatabase {
public:
	SongDatabase(const std::string &name) {
		db = nullptr;
		int rc = sqlite3_open(name.c_str(), &db);
		if(rc == SQLITE_OK) {

		};
	}
	// Simple search. Checks if TITLE or COMPOSER contains 'pattern'
	//string [] query(const string &pattern, int limit = 100) {
	//}

	// Incremental query
	IncrementalQuery find() {
		return IncrementalQuery(db);
	}
private:
	sqlite3 *db;
};