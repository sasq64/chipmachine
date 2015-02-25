#ifdef LINUX
#define BACKWARD_HAS_BFD 1
#include <backward-cpp/backward.hpp>
namespace backward {
	backward::SignalHandling sh;
}
#endif

#include "ChipMachine.h"
#include "MusicPlayerList.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>
#include <coreutils/format.h>
#include <coreutils/var.h>
#include <bbsutils/console.h>

#include <vector>

using namespace std;
using namespace chipmachine;
using namespace bbs;
using namespace utils;

int main(int argc, char* argv[]) {

#ifdef CM_DEBUG
	logging::setLevel(logging::DEBUG);
#else
	logging::setLevel(logging::WARNING);
#endif

	vector<SongInfo> songs;
	int w = 960;
	int h = 540;
	bool fullScreen = false;
	bool server = false;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'd':
				fullScreen = false;
				logging::setLevel(logging::DEBUG);
				break;
			case 'w':
				fullScreen = false;
				break;
			case 'f':
				fullScreen = true;
				break;
			case 's':
				server = true;
			case 'h':
				w = 1280;
				h = 720;
				break;
			}
		} else {
			songs.emplace_back(argv[i]);
		}
	}

	string path = File::getExeDir() + ":" + File::getExeDir() + "/../Resources:" + File::getAppDir();
	string workDir = File::findFile(path, "data").getDirectory();

	if(workDir == "") {
		fprintf(stderr, "** Error: Could not find data files\n");
		exit(-1);
	}

	LOGD("WorkDir:%s", workDir);

	if(server) {
		MusicPlayerList player(workDir);
		MusicDatabase::getInstance().initFromLua(workDir);
		TelnetInterface telnet(player);
		telnet.start();
		while(true) sleepms(500);
	}

	if(songs.size() > 0) {

		Console *c = Console::createLocalConsole();

		MusicPlayerList mpl(workDir);
		for(auto &s : songs) {
			LOGD("Adding '%s'", s.path);
			mpl.addSong(s);
		}
		mpl.nextSong();
		printf("\n--------------------------\nCHIPMACHINE CONSOLE PLAYER\n--------------------------\n");
		while(true) {
			if(mpl.getState() == MusicPlayerList::PLAY_STARTED) {
				auto si = mpl.getInfo();
				auto l = mpl.getLength();
				print_fmt("%s - %s (%s) [%02d:%02d]\n", si.composer, si.title, si.format, l/60, l%60);
			}
			if(c) {
				auto k = c->getKey(100);
				if(k !=Console::KEY_TIMEOUT) {
					LOGD("KEY %d", k);
					switch(k) {
					case ' ':
						break;
					case Console::KEY_RIGHT:
						break;
					case Console::KEY_ENTER:
						mpl.nextSong();
						break;
					}
				}
			}
		}
	}

	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(w, h, false);

	static chipmachine::ChipMachine app(workDir);

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
	return 0;
}
