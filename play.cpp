
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include <vector>
#include <string>
#include <memory>
#include <cstdlib>


#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "PlayerFactory.h"

#include "utils.h"
#include "log.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "SexyPSFPlugin.h"
#include "GMEPlugin.h"


#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include <unistd.h>


typedef unsigned int uint;
using namespace std;
using namespace utils;
using namespace logging;

class PlayerSystem : public PlayerFactory {
public:
	virtual ChipPlayer *fromFile(File &file) override {

		string name = file.getName();
		makeLower(name);
		LOGD("Handling %s\n", name);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file.getName());
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		LOGD("Factory checking: %s\n", lname);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

private:
	vector<ChipPlugin*> plugins;
};

int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {});
	psys.registerPlugin(new SexyPSFPlugin {});
	psys.registerPlugin(new GMEPlugin {});

	File file { argv[1] };
	ChipPlayer *player = psys.fromFile(file);

	string songTitle = player->getMetaDataString("title");
	int length player->getMetaDataInt("length");

	int frameCount = 0;
	string songName;

	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	while(true) {
		int rc = player->getSamples(&buffer[0], bufSize);
		if(rc > 0) {
			ap.writeAudio(&buffer[0], rc);
			frameCount += rc/2;
		} else
			sleepms(250);
	}
	return 0;
}