
#include "MusicPlayer.h"
#include <coreutils/utils.h>

#include <ModPlugin/ModPlugin.h>
#include <VicePlugin/VicePlugin.h>
#include <SexyPSFPlugin/SexyPSFPlugin.h>
#include <GMEPlugin/GMEPlugin.h>
#include <SC68Plugin/SC68Plugin.h>
#include <UADEPlugin/UADEPlugin.h>

using namespace std;

namespace chipmachine {

MusicPlayer::MusicPlayer() : plugins {
		new ModPlugin {},
		new VicePlugin {"data/c64"},
		new SexyPSFPlugin {},
		new GMEPlugin {},
		new SC68Plugin {"data/sc68"},
		new UADEPlugin {}
	}
{
}

shared_ptr<ChipPlayer> MusicPlayer::fromFile(const std::string &fileName) {
	shared_ptr<ChipPlayer> player;
	string name = fileName;
	utils::makeLower(name);
	for(auto *plugin : plugins) {
		if(plugin->canHandle(name)) {
			printf("Playing with %s\n", plugin->name().c_str());
			player = shared_ptr<ChipPlayer>(plugin->fromFile(fileName));
			break;
		}
	}
	return player;
}

}