
#include "ChipMachine.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>

#include "MusicPlayerList.h"

#include <vector>

using namespace std;
using namespace chipmachine;

int main(int argc, char* argv[]) {

	//logging::setLevel(logging::WARNING);

	bool fullScreen = false;
	vector<SongInfo> songs;
	int w = 720;
	int h = 576;

	if(argc >= 2) {
		for(int i=1; i<argc; i++) {
			if(argv[i][0] == '-') {
				switch(argv[i][1]) {
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
	}

	if(songs.size() > 0) {
		MusicPlayerList mpl;
		for(auto &s : songs) {
			LOGD("Adding %s", s.path);
			mpl.addSong(s);
		}
		mpl.nextSong();
		while(true) {
			if(mpl.getState() == MusicPlayerList::PLAY_STARTED) {
				auto si = mpl.getInfo();
				printf("TITLE:%s LENGTH %d\n", si.title.c_str(), mpl.getLength());
			}
			sleepms(100);
		}
	}


	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(w, h, false);
	static chipmachine::ChipMachine app;

	for(auto &s : songs) {
		app.play(s);
	}

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
	return 0;	
}

