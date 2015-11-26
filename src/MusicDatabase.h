#ifndef MUSIC_DATABASE_H
#define MUSIC_DATABASE_H

#include "SongInfo.h"
#include "SearchIndex.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>

#include <unordered_set>
#include <coreutils/thread.h>
#include <vector>
#include <string>
#include <future>
#include <mutex>

namespace chipmachine {

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};

// console -- sid -- tracker -- amiga
enum Formats {

	NOT_SET,

	UNKNOWN_FORMAT,
	NO_FORMAT,
	PLAYLIST,

	CONSOLE,

	HES,

	NINTENDO,

	GAMEBOY,
	NES,
	SNES,
	NINTENDO64,
	GBA,
	NDS,

	SEGA,

	SEGAMS,
	MEGADRIVE,
	DREAMCAST,

	SONY,

	PLAYSTATION,
	PLAYSTATION2,

	COMPUTER,
	C64,
	SID,

	SPECTRUM,

	ATARI,

	MP3,

	OGG,

	YOUTUBE,

	PC,

	ADPLUG,
	TRACKER = 0x30,
	SCREAMTRACKER,
	IMPULSETRACKER,
	FASTTRACKER,

	AMIGA,
	PROTRACKER,

	UADE,
};

class MusicDatabase : public SearchProvider {
public:
	MusicDatabase() : db(utils::File::getCacheDir() / "music.db"), reindexNeeded(false) {
		db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, localdir STRING, "
		        "description STRING, id UNIQUE, version INTEGER)");
		db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, "
		        "format STRING, path STRING, collection INTEGER, metadata STRING)");
	}

	bool initFromLua(const utils::File &workDir);
	void initFromLuaAsync(const utils::File &workDir);

	int search(const std::string &query, std::vector<int> &result,
	           unsigned int searchLimit) override;
	// Lookup internal string for index
	std::string getString(int index) const override {
		std::lock_guard<std::mutex>{dbMutex};
		return utils::format("%s %s", getTitle(index), getComposer(index));
	}

	std::string getFullString(int index) const override {
		std::lock_guard<std::mutex>{dbMutex};
		return utils::format("%s\t%s\t%d\t%d", getTitle(index), getComposer(index), index,
		                     formats[index]);
	}
	// Get full data, may require SQL query
	SongInfo getSongInfo(int index) const; // { return getString(index); }

	std::string getTitle(int index) const {
		std::lock_guard<std::mutex>{dbMutex};
	   	return titleIndex.getString(index);
	}

	std::string getComposer(int index) const {
		std::lock_guard<std::mutex>{dbMutex};
		return composerIndex.getString(titleToComposer[index]);
	}

	std::shared_ptr<IncrementalQuery> createQuery() {
		std::lock_guard<std::mutex>{dbMutex};
		return std::make_shared<IncrementalQuery>(this);
	}

	int getSongs(std::vector<SongInfo> &target, const SongInfo &match, int limit, bool random);

	bool busy() {
		std::lock_guard<std::mutex>{chkMutex};
		if(initFuture.valid()) {
			if(initFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
				initFuture.get();
				return false;
			}
			return true;
		}

		if(dbMutex.try_lock()) {
			dbMutex.unlock();
			return false;
		}
		return true;
	}

	SongInfo lookup(const std::string &path);

	static MusicDatabase &getInstance() {
		static MusicDatabase mdb;
		return mdb;
	}

private:
	void initDatabase(const std::string &workDir,
	                  std::unordered_map<std::string, std::string> &vars);
	void generateIndex();

	struct Collection {
		Collection(int id = -1, const std::string &name = "", const std::string url = "",
		           const std::string local_dir = "")
		    : id(id), name(name), url(url), local_dir(local_dir) {}
		int id;
		std::string name;
		std::string url;
		std::string local_dir;
	};

	bool parseModlandPath(SongInfo &song);
	void writeIndex(utils::File &f);
	void readIndex(utils::File &f);

	bool reindexNeeded;

	SearchIndex composerIndex;
	SearchIndex titleIndex;

	std::vector<uint32_t> titleToComposer;
	std::vector<uint32_t> composerToTitle;
	std::vector<uint32_t> composerTitleStart;
	std::vector<uint16_t> formats;

	mutable std::mutex chkMutex;
	mutable std::mutex dbMutex;
	sqlite3db::Database db;

	uint16_t dbVersion;
	uint16_t indexVersion;

	std::future<void> initFuture;
	std::atomic<bool> indexing;
};
}

#endif // MUSIC_DATABASE_H
