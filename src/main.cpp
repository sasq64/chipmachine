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
	bool fullScreen = false;
	logging::setLevel(logging::DEBUG);
#else
	bool fullScreen = true;
	logging::setLevel(logging::WARNING);
#endif

	vector<SongInfo> songs;
	int w = 720;
	int h = 576;

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
			case 'h':
				w = 1200;
				h = 800;
				break;
			}
		} else {
			songs.push_back(SongInfo(argv[i]));
		}
	}

	if(songs.size() > 0) {

		Console *c = Console::createLocalConsole();

		MusicPlayerList mpl;
		for(auto &s : songs) {
			LOGD("Adding %s", s.path);
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

	static chipmachine::ChipMachine app;

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
	return 0;
}

