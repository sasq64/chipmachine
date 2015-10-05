// #ifdef LINUX
// #define BACKWARD_HAS_BFD 1
// #include <backward-cpp/backward.hpp>
// namespace backward {
// 	backward::SignalHandling sh;
// }
// #endif

#include "ChipMachine.h"
#include "MusicPlayer.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>
#include <coreutils/format.h>
#include <coreutils/var.h>
#include <bbsutils/console.h>

#include <vector>

#define ENABLE_CONSOLE

using namespace std;
using namespace chipmachine;
using namespace bbs;
using namespace utils;

int main(int argc, char *argv[]) {

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

	for(int i = 1; i < argc; i++) {
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
			case 't':
				w = 720;
				h = 576;
				break;
			case 'h':
				w = 1280;
				h = 720;
				break;
			}
		} else {
			songs.emplace_back(argv[i]);
		}
	}

	string path = File::makePath(
	    {File::getExeDir(), (File::getExeDir() / ".." / "Resources:"), File::getAppDir()});
	LOGD("PATH:%s", path);
	string workDir = File::findFile(path, "data").getDirectory();

	if(workDir == "") {
		fprintf(stderr, "** Error: Could not find data files\n");
		exit(-1);
	}

	LOGD("WorkDir:%s", workDir);

	if(server) {
		MusicPlayerList player(workDir);
		MusicDatabase::getInstance().initFromLua(workDir);
#ifdef ENABLE_TELNET
		TelnetInterface telnet(player);
		telnet.start();
#endif
		while(true)
			sleepms(500);
	}

	if(songs.size() > 0) {
		int pos = 0;
#ifdef ENABLE_CONSOLE
		Console *c = Console::createLocalConsole();
#endif
		MusicPlayer pl(workDir);
		while(true) {
			if(pos >= songs.size())
				return 0;
			pl.playFile(songs[pos++].path);
			SongInfo info = pl.getPlayingInfo();
			print_fmt("Playing: %s\n",
			          info.title != "" ? info.title : utils::path_filename(songs[pos - 1].path));
			while(pl.playing()) {
				pl.update();
				if(c) {
					auto k = c->getKey(100);
					if(k != Console::KEY_TIMEOUT) {
						switch(k) {
						case Console::KEY_ENTER:
							pl.stop();
							break;
						}
					}
				}
			}
		}
#if 0
		for(auto &s : songs) {
			LOGD("Adding '%s'", s.path);
			mpl.addSong(s);
		}
		mpl.nextSong();
		printf("\n--------------------------\nCHIPMACHINE CONSOLE PLAYER\n--------------------------\n");
		int currentTune = 0;
		SongInfo currentInfo;
		while(true) {
			if(mpl.getState() == MusicPlayerList::PLAY_STARTED) {
				currentInfo = mpl.getInfo();
				currentTune = mpl.getTune();
				auto l = mpl.getLength();
				auto fileName = path_filename(currentInfo.path);
				auto title = currentInfo.title;
				if(title != "") {
					if(currentInfo.composer != "")
						title = currentInfo.composer + " - " + title;
					title += " (" + fileName + ")";
				} else title = fileName;
				print_fmt("%s [%s] [%02d:%02d]\n", title, currentInfo.format, l/60, l%60);
			}
#ifdef ENABLE_CONSOLE

			int tune = mpl.getTune();
			if(currentTune != tune) {
				currentTune = tune;
				print_fmt("Subtune: %d\n", currentTune);
			}


			if(c) {
				auto k = c->getKey(100);
				if(k != Console::KEY_TIMEOUT) {
					switch(k) {
					case ' ':
						break;
					case Console::KEY_RIGHT:
						if(currentTune < currentInfo.numtunes-1)
							mpl.seek(currentTune+1);
						break;
					case Console::KEY_LEFT:
						if(currentTune > 0)
							mpl.seek(currentTune-1);
						break;
					case Console::KEY_ENTER:
						mpl.nextSong();
						break;
					}
				}
			}
#else
			sleepms(500);
#endif
		}
#endif
	}

	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(w, h, false);

	static chipmachine::ChipMachine app(workDir);

	grappix::screen.render_loop(
	    [](uint32_t delta) {
		    app.update();
		    app.render(delta);
		},
	    20);
	return 0;
}
