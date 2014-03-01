
#include "ChipPlayer.h"

#include <ModPlugin/ModPlugin.h>
#include <VicePlugin/VicePlugin.h>
#include <SexyPSFPlugin/SexyPSFPlugin.h>
#include <GMEPlugin/GMEPlugin.h>
#include <SC68Plugin/SC68Plugin.h>
#include <UADEPlugin/UADEPlugin.h>

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <bbsutils/console.h>
#include <audioplayer/audioplayer.h>
#include <cstdio>
#include <vector>
#include <string>

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace bbs;

class PlayerSystem  {
public:
	ChipPlayer *fromFile(File &file) {

		string name = file.getName();
		makeLower(name);
		for(auto *plugin : plugins) {
			if(plugin->canHandle(name)) {
				print_fmt("Playing with %s\n", plugin->name());
				fflush(stdout);
				return plugin->fromFile(file.getName());
			}
		}
		return nullptr;
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
	logging::setLevel(logging::ERROR);

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {"data/c64"});
	psys.registerPlugin(new SexyPSFPlugin {});
	psys.registerPlugin(new GMEPlugin {});
	psys.registerPlugin(new SC68Plugin {"data/sc68"});
	psys.registerPlugin(new UADEPlugin {});

	File file { argv[1] };
	auto *player = psys.fromFile(file);

	if(player) {
		mutex m;
		auto *console = bbs::Console::createLocalConsole();
		console->clear();
		console->moveCursor(0,0);

		player->onMeta([&](const vector<string> &meta, ChipPlayer *player) {
			lock_guard<mutex> guard(m);
			for(const auto &m : meta)
				console->write(format("%s:%s\n", m, player->getMeta(m)));
		});

		AudioPlayer ap ([&](int16_t *ptr, int size) {
			if(player)
				player->getSamples(ptr, size);
		});

		int song = 0;

		while(true) {
			int k;
			{ lock_guard<mutex> guard(m);
				k = console->getKey(0);
			}
			switch(k) {
			case Console::KEY_RIGHT:
				player->seekTo(++song);
				break;
			case Console::KEY_LEFT:
				player->seekTo(--song);
				break;
			}
			sleepms(100);
		}

		while(true)
			sleepms(250);
	} else
		print_fmt("FAILED\n");

	return 0;
}