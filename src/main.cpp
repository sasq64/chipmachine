
#include "ChipMachine.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>

#include <vector>

using namespace std;

int main(int argc, char* argv[]) {

	bool fullScreen = false;
	vector<SongInfo> songs;

	if(argc >= 2) {
		for(int i=1; i<argc; i++) {
			if(argv[i][0] == '-') {
				switch(argv[i][1]) {
				case 'f':
					fullScreen = true;
				}
			} else {
				songs.push_back(SongInfo(argv[i]));
			}
		}
	}

	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(720, 576, false);
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

