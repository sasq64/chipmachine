
#include "ChipPlayer.h"

#include <ModPlugin/ModPlugin.h>
#include <VicePlugin/VicePlugin.h>
#include <SexyPSFPlugin/SexyPSFPlugin.h>
#include <GMEPlugin/GMEPlugin.h>
#include <SC68Plugin/SC68Plugin.h>
#include <UADEPlugin/UADEPlugin.h>

#include <coreutils/utils.h>
#include <audioplayer/audioplayer.h>
#include <cstdio>
#include <vector>
#include <string>

using namespace chipmachine;
using namespace std;
using namespace utils;

int main(int argc, char* argv[]) {

	if(argc < 2) {
		printf("%s [musicfiles...]\n", argv[0]);
		return 0;
	}

	vector<ChipPlugin*> plugins = {
		new ModPlugin {},
		new VicePlugin {"data/c64"},
		new SexyPSFPlugin {},
		new GMEPlugin {},
		new SC68Plugin {"data/sc68"},
		new UADEPlugin {}
	};

	File file { argv[1] };
	ChipPlayer *player = nullptr;

	string name = file.getName();
	makeLower(name);
	for(auto *plugin : plugins) {
		if(plugin->canHandle(name)) {
			printf("Playing with %s\n", plugin->name().c_str());
			player = plugin->fromFile(file.getName());
		}
	}

	if(player) {
		AudioPlayer ap ([&](int16_t *ptr, int size) {
			player->getSamples(ptr, size);
		});
		while(true)
			sleepms(100);
	} else
		printf("%s FAILED\n", argv[1]);
	return 0;
}