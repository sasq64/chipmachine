
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>

#include <sqlite3.h>

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "utils.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "PSXPlugin.h"

#include "TelnetServer.h"


#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include "Archive.h"

typedef unsigned int uint;
using namespace std;
using namespace utils;



class PlayerSystem : public PlayerFactory {
public:
	virtual ChipPlayer *fromFile(File &file) override {

		string name = file.getName();
		makeLower(name);
		printf("Handling %s\n", name.c_str());

		for(auto *plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file);
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

	ChipPlayer *play(const string &url) {
		return new URLPlayer {url, this};
	}

private:
	vector<ChipPlugin*> plugins;
};

int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("Modplayer test\n");

	mutex playMutex;
	queue<string> playQueue;


	TelnetServer telnet { 12345 };
	telnet.addCommand("play", [&](TelnetServer::User &user, const vector<string> &args) {
		//printf("Play '%s'\n", args[1].c_str());
		playMutex.lock();
		playQueue.push(args[1]);
		playMutex.unlock();
	});

	telnet.setConnectCallback([&](TelnetServer::User &user) {
		user.write("Chipmachine v0.1\n");
	});

	telnet.runThread();

	if(argc > 1) {
		playQueue.push(argv[1]);
	}
	//else
		//name = "ftp://modland.ziphoid.com/pub/modules/Protracker/Heatbeat/cheeseburger.mod";
		//name = "http://swimsuitboys.com/droidmusic/C64%20Demo/Amplifire.sid";



	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {});
	psys.registerPlugin(new PSFPlugin {});

	ChipPlayer *player = nullptr; //psys.play(name);
	//if(name.length() > 0)
	//	player = psys.play(name);

	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	while(true) {

		playMutex.lock();
		if(!playQueue.empty()) {
			if(player)
				delete player;
			player = psys.play(playQueue.front());
			playQueue.pop();
		}
		playMutex.unlock();

		if(player) {
			int rc = player->getSamples(&buffer[0], bufSize);
			if(rc > 0)
				ap.writeAudio(&buffer[0], rc);
		} else
			sleepms(250);
	}
	return 0;
}