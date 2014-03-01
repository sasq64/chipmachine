#include "ModPlugin.h"
#include "VicePlugin.h"
#include "SexyPSFPlugin.h"
#include "GMEPlugin.h"
#include "SC68Plugin.h"
#include "UADEPlugin.h"

#include "ChipPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <audioplayer/audioplayer.h>
#include <cstdio>
#include <vector>
#include <string>

using namespace chipmachine;
using namespace std;
using namespace utils;

class PlayerSystem  {
public:
	ChipPlayer *fromFile(File &file) {

		string name = file.getName();
		makeLower(name);
		for(auto *plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file.getName());
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) {

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

private:
	vector<ChipPlugin*> plugins;
};

int main(int argc, char* argv[]) {

	if(argc < 2) {
		printf("%s [musicfile]\n", argv[0]);
		return 0;
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {"data/c64"});
	psys.registerPlugin(new SexyPSFPlugin {});
	psys.registerPlugin(new GMEPlugin {});
	psys.registerPlugin(new SC68Plugin {"data/sc68"});
	psys.registerPlugin(new UADEPlugin {});

	File file { argv[1] };
	auto *player = psys.fromFile(file);
	
	AudioPlayer ap ([&](int16_t *ptr, int size) {
		if(player)
			player->getSamples(ptr, size);
	});

	while(true)
		sleepms(250);

	return 0;
}