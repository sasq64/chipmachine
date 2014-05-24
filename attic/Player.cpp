#include <coreutils/log.h>

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/var.h>

#include "Player.h"

#include "SharedState.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "SexyPSFPlugin.h"
#include "GMEPlugin.h"
#include "SC68Plugin.h"
#include "UADEPlugin.h"
#include "StSoundPlugin.h"

#include "SongState.h"

#include "inject.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <cstdlib>
#include <unistd.h>

typedef unsigned int uint;
using namespace std;
using namespace utils;

//class SharedState playerState;

class PlayerSystem : public PlayerFactory {
public:
	virtual ChipPlayer *fromFile(File &file) override {

		string name = file.getName();
		makeLower(name);
		LOGD("Handling %s", name);

		for(auto &plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file.getName());
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		LOGD("Factory checking: %s", lname);

		for(auto &plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	template<class T, class... Args>
	void addPlugin(Args&& ... args) {
		plugins.push_back(unique_ptr<ChipPlugin>(new T(args...)));
	}

	//void registerPlugin(ChipPlugin *p) {	
	//	plugins.push_back(p);
	//}

	unique_ptr<ChipPlayer> play(const string &url) {
		return unique_ptr<ChipPlayer>(new URLPlayer {url, this});
	}

private:
	vector<unique_ptr<ChipPlugin>> plugins;
};


Player::Player() : player(nullptr), buffer(4096), oldSeconds(-1), frameCount(0), globalState(SharedState::getGlobal("playerState")) {

	globalState["songState"] = ref<SongState>(songState);
	globalState["playState"] = ref<PlayState>(playState);

	globalState.callOnChange("songState", [&](const string &what) {
		const SongState &ss = this->songState;
		LOGD("Setting title to %s", ss.title);
		globalState["songTitle"] = ss.title;
		globalState.touch(ss.title);
		globalState["songComposer"] = ss.composer;
		globalState["songFormat"] = ss.format;
		globalState["songLength"] = ss.length;
		globalState["songSubSongs"] = ss.totalSongs;
	});

	globalState.callOnChange("playState", [&](const string &what) {
		const PlayState &ps = this->playState;//globalState["playState"];
		globalState["playSeconds"] = ps.seconds;
		globalState["playSubSong"] = ps.currentSong;
	});

	globalState.touch("songState");
	globalState.touch("playState");

	auto psys = new PlayerSystem();
	factory = psys;

	psys->addPlugin<ModPlugin>();
	psys->addPlugin<VicePlugin>("data/c64");
	psys->addPlugin<SexyPSFPlugin>();
	psys->addPlugin<GMEPlugin>();
	psys->addPlugin<SC68Plugin>("data/sc68data");
	psys->addPlugin<StSoundPlugin>();
#ifndef WIN32
	psys->addPlugin<UADEPlugin>();
#endif

}

void Player::run() {

	while(true) {

		{
			lock_guard<mutex> guard(playMutex);
			if(!playQueue.empty()) {
				string url = playQueue.front();
				LOGD("Found '%s' in queue", url);
				//player = psys->play(songName);
				player = unique_ptr<ChipPlayer>(new URLPlayer {url, factory});

				songState.title = path_basename(url);
				globalState.touch("songState");
				oldSeconds = 0;
				playQueue.pop();
				frameCount = 0;
			}
			//if(subSong != currentSong) {
			//	player->seekTo(subSong);
			//	currentSong = subSong;
			//	frameCount = 0;
			//}
		}

		if(player) {
			int rc = player->getSamples(&buffer[0], buffer.size());
			if(rc > 0) {
				ap.writeAudio(&buffer[0], rc);
				frameCount += rc/2;

				int seconds = frameCount / 44100;
				if(seconds != oldSeconds) {
					oldSeconds = seconds;
				}

			} else if(rc < 0) {
				player = nullptr;
			}
		} else
			sleepms(250);
	}
}

void Player::runThread() {
	mainThread = thread {&Player::run, this};
}
