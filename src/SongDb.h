#ifndef SONG_DB_H
#define SONG_DB_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <coreutils/format.h>

#include "SearchIndex.h"

struct sqlite3;
class SongDatabase;

class database_exception : public std::exception {
public:
	database_exception(const char *txt) : msg(txt) {
	}
	virtual const char *what() const throw() { return msg.c_str(); }
private:
	std::string msg;
};

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};


class SongDatabase : public SearchProvider {
public:

	SongDatabase(const std::string &name);
	~SongDatabase();
	void generateIndex();
	IncrementalQuery find() {
		std::lock_guard<std::mutex>{dbLock};
		return IncrementalQuery(this);
	}

	int search(const std::string &query, std::vector<int> &result, unsigned int searchLimit);

	std::string getString(int index) const {
		//std::lock_guard<std::mutex>{dbLock};
		return utils::format("%s\t%s\t%d", getTitle(index), getComposer(index), index);
	}

	std::string getTitle(int index) const { 
		return titleIndex.getString(index);
	}

	std::string getComposer(int index) const {
		return composerIndex.getString(titleToComposer[index]);
	}

	int getFormat(int index) {
		//std::lock_guard<std::mutex>{dbLock};
		return formats[index];
	}

	void addSubStrings(const std::string &words, std::unordered_map<uint16_t, std::vector<int>> &stringMap, int index);

	virtual std::string getFullString(int index) const override;

private:

	sqlite3 *db;

	SearchIndex composerIndex;
	SearchIndex titleIndex;
	std::vector<int> titleToComposer;
	std::vector<int> composerToTitle;
	std::vector<uint8_t> formats;

	std::mutex dbLock;
};

#endif // SONG_DB_H