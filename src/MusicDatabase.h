#ifndef DATABASE_H
#define DATABASE_H

#include "SongInfo.h"

#include "SearchIndex.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <string>

namespace chipmachine {

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};

class MusicDatabase : public SearchProvider {
public:
	MusicDatabase() : db("music.db") {}

	void init();

	void initDatabase(std::string name, std::unordered_map<std::string, std::string> &vars);

	void modlandInit(const std::string &source, const std::string &song_list, int id);
	void hvscInit(const std::string &source, int id);

	void generateIndex();

	int search(const std::string &query, std::vector<int> &result, unsigned int searchLimit) override;
	// Lookup internal string for index
	virtual std::string getString(int index) const override {
		return utils::format("%s\t%s\t%d", getTitle(index), getComposer(index), index);
	}
	// Get full data, may require SQL query
	virtual std::string getFullString(int index) const override;// { return getString(index); }

	std::string getTitle(int index) const { 
		return titleIndex.getString(index);
	}

	std::string getComposer(int index) const {
		return composerIndex.getString(titleToComposer[index]);
	}

	virtual std::vector<SongInfo> find(const std::string &pattern);

	IncrementalQuery createQuery() {
		std::lock_guard<std::mutex>{dbMutex};
		return IncrementalQuery(this);
	}

private:
	SearchIndex composerIndex;
	SearchIndex titleIndex;
	std::vector<int> titleToComposer;
	std::vector<int> composerToTitle;

	std::mutex dbMutex;
	sqlite3db::Database db;
};

}

#endif // DATABASE_H
