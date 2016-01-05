#ifndef CHIP_INTERFACE_H
#define CHIP_INTERFACE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"

#include <coreutils/utils.h>
#include <luainterpreter/luainterpreter.h>

#include <vector>
#include <string>
#include <functional>

namespace chipmachine {

class ChipInterface {
public:
	ChipInterface(const std::string &wd) : workDir(wd), player(wd) {}

	int init() {

		MusicDatabase::getInstance().initFromLua(this->workDir);
		iquery = MusicDatabase::getInstance().createQuery();
		return 0;
	}

	int search(const std::string &text) {
		iquery->setString(text);
		return 0;
	}

	const std::vector<std::string> &getResult(int start, int len) {
		return iquery->getResult(start, len);
	}

	SongInfo getSongInfo(int i) { return mdb.getSongInfo(i); }

	int play(const SongInfo &song) {
		player.playSong(song);
		return 0;
	}

	int numHits() { return iquery->numHits(); }

	void addSong(const SongInfo &song);
	void nextSong();
	void clearSongs();

	void update() {
		// player.update();
	}

	void onMeta(std::function<void(const SongInfo &)> callback);

private:
	utils::File workDir;

	std::shared_ptr<IncrementalQuery> iquery;
	MusicDatabase mdb;
	MusicPlayerList player;
	MusicPlayerList::State playerState;

	void setupRules();
	void updateKeys();
};

} // namespace

#endif // CHIP_INTERFACE_H
