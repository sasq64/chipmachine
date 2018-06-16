#ifndef CHIP_INTERFACE_H
#define CHIP_INTERFACE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"

#include <coreutils/utils.h>

#include <vector>
#include <string>
#include <functional>
#include <algorithm>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace chipmachine {

class ChipInterface {
public:
	ChipInterface(const fs::path &wd) : workDir(wd), player(wd) {
		MusicDatabase::getInstance().initFromLua(this->workDir);
	}
	
	std::shared_ptr<IncrementalQuery> createQuery() {
		std::lock_guard<std::mutex> lg(m);
		return MusicDatabase::getInstance().createQuery();
	}

	SongInfo getSongInfo(int i) { return MusicDatabase::getInstance().getSongInfo(i); }

	int play(const SongInfo &song) {
		player.playSong(song);
		return 0;
	}

	void addSong(const SongInfo &song) {
		player.addSong(song);
	}
	void nextSong() {
		player.nextSong();
	}
	void clearSongs();
	
	void setTune(int t) {
		player.seek(t);
	}
	
	bool playing() { return player.isPlaying(); }
	void pause(bool p) { return player.pause(p); }

	void update() {
		playerState = player.getState();
		if(playerState == MusicPlayerList::Playstarted) {
			info = player.getInfo();
			for(auto &cb : meta_callbacks)
				(*cb)(info);
		}
	}
	
	using MetaCallback = std::function<void(const SongInfo&)>;
	using MetaHolder = std::shared_ptr<std::function<void(std::nullptr_t)>>;
	
	MetaHolder onMeta(const MetaCallback &callback) {
		std::lock_guard<std::mutex> lg(m);
		meta_callbacks.push_back(std::make_shared<MetaCallback>(callback));
		auto mc = meta_callbacks.back();
		(*mc)(info);
		return MetaHolder(nullptr, [=](std::nullptr_t) {
			std::lock_guard<std::mutex> lg(m);
			meta_callbacks.erase(std::remove(meta_callbacks.begin(), meta_callbacks.end(), mc));
		});
	}

	int seconds() {
		return player.getPosition();
	}

private:
    fs::path workDir;
	std::mutex m;
	MusicDatabase mdb;
	SongInfo info;
	MusicPlayerList player;
	MusicPlayerList::State playerState;
	std::vector<std::shared_ptr<MetaCallback>> meta_callbacks;
	void setupRules();
	void updateKeys();
};

} // namespace

#endif // CHIP_INTERFACE_H
