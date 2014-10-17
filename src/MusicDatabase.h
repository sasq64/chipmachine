#ifndef MUSIC_DATABASE_H
#define MUSIC_DATABASE_H

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
	MusicDatabase() : db(utils::File::getCacheDir() + "music.db") {
		db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, localdir STRING, description STRING, id INTEGER, version INTEGER)");
		db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, format STRING, path STRING, collection INTEGER)");
	}

	void initDatabase(std::unordered_map<std::string, std::string> &vars);

	void generateIndex();

	int search(const std::string &query, std::vector<int> &result, unsigned int searchLimit) override;
	// Lookup internal string for index
	std::string getString(int index) const override {
		return utils::format("%s\t%s\t%d\t%d", getTitle(index), getComposer(index), index, formats[index]);
	}
	// Get full data, may require SQL query
	std::string getFullString(int index) const override;// { return getString(index); }

	std::string getTitle(int index) const {
		return titleIndex.getString(index);
	}

	std::string getComposer(int index) const {
		return composerIndex.getString(titleToComposer[index]);
	}

	//virtual std::vector<SongInfo> find(const std::string &pattern);

	IncrementalQuery createQuery() {
		std::lock_guard<std::mutex>{dbMutex};
		return IncrementalQuery(this);
	}


	SongInfo lookup(const std::string &path);

	static MusicDatabase& getInstance() {
		static MusicDatabase mdb;
		return mdb;
	}

private:

	struct Collection {
		Collection(int id = -1, const std::string &name = "", const std::string url = "", const std::string local_dir = "") : id(id), name(name), url(url), local_dir(local_dir) {}
		int id;
		std::string name;
		std::string url;
		std::string local_dir;
	};

	bool parseModlandPath(SongInfo &song);
	void writeIndex(utils::File &f);
	void readIndex(utils::File &f);


	SearchIndex composerIndex;
	SearchIndex titleIndex;

	std::vector<uint32_t> titleToComposer;
	std::vector<uint32_t> composerToTitle;
	std::vector<uint32_t> composerTitleStart;
	std::vector<uint16_t> formats;

	std::mutex dbMutex;
	sqlite3db::Database db;
};

}

#endif // MUSIC_DATABASE_H
