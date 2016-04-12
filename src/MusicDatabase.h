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

	M3U,
	PLS,

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
	using Variables = std::unordered_map<std::string, std::string>;

	MusicDatabase() : db(utils::File::getCacheDir() / "music.db"), reindexNeeded(false) {
		createTables();
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
		int f;
		if(index >= PLAYLIST_INDEX)
			f = PLAYLIST;
		else
			f = formats[index];
		return utils::format("%s\t%s\t%d\t%d", getTitle(index), getComposer(index), index, f);
	}
	// Get full data, may require SQL query
	SongInfo getSongInfo(int index) const;

	std::string getTitle(int index) const {
		std::lock_guard<std::mutex>{dbMutex};
		if(index >= PLAYLIST_INDEX)
			return playLists[index - PLAYLIST_INDEX].name;
		return titleIndex.getString(index);
	}

	std::string getComposer(int index) const {
		std::lock_guard<std::mutex>{dbMutex};
		if(index >= PLAYLIST_INDEX)
			return "";
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

	SongInfo& lookup(SongInfo &song);

	static MusicDatabase &getInstance() {
		static MusicDatabase mdb;
		return mdb;
	}

	struct Playlist {
		Playlist(utils::File f) : fileName(f.getName()) {
			if(f.exists()) {
				for(const auto &l : f.getLines()) {
					LOGD("SONG:'%s'", l);
					if(l != "")
						songs.emplace_back(l);
				}
			}
			name = f.getFileName();
		}
		std::string name;
		std::string fileName;
		std::vector<SongInfo> songs;
		void save() {
			utils::File f{fileName};
			for(const auto &s : songs) {
				if(s.starttune >= 0)
					f.writeln(utils::format("%s;%d", s.path, s.starttune));
				else
					f.writeln(s.path);
			}
		}
	};

	void addToPlaylist(const std::string &plist, const SongInfo &song);
	void removeFromPlaylist(const std::string &plist, const SongInfo &song);
	std::vector<SongInfo> &getPlaylist(const std::string &plist);

	void setFilter(const std::string &filter);

private:
	void initDatabase(const std::string &workDir, Variables &vars);
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

	typedef bool (MusicDatabase::*MemFun)(Variables &, const std::string &,
	                                      std::function<void(const SongInfo &)>);

	bool parseCsdb(Variables &vars, const std::string &listFile,
	               std::function<void(const SongInfo &)> callback);
	bool parsePouet(Variables &vars, const std::string &listFile,
	                std::function<void(const SongInfo &)> callback);
	bool parseRss(Variables &vars, const std::string &listFile,
	              std::function<void(const SongInfo &)> callback);
	bool parseModland(Variables &vars, const std::string &listFile,
	                  std::function<void(const SongInfo &)> callback);
	bool parseStandard(Variables &vars, const std::string &listFile,
	                   std::function<void(const SongInfo &)> callback);

	void writeIndex(utils::File &f);
	void readIndex(utils::File &f);

	void createTables();

	static constexpr int PLAYLIST_INDEX = 0x10000000;

	SearchIndex composerIndex;
	SearchIndex titleIndex;

	std::vector<uint32_t> titleToComposer;
	std::vector<uint32_t> composerToTitle;
	std::vector<uint32_t> composerTitleStart;
	std::vector<uint16_t> formats;

	mutable std::mutex chkMutex;
	mutable std::mutex dbMutex;
	sqlite3db::Database db;
	bool reindexNeeded;

	uint16_t dbVersion;
	uint16_t indexVersion;

	int collectionFilter = -1;

	std::future<void> initFuture;
	std::atomic<bool> indexing;

	std::vector<Playlist> playLists;
};
}

#endif // MUSIC_DATABASE_H
