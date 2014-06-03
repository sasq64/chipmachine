
#include "ChipMachine.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>

int main(int argc, char* argv[]) {

	bool fullScreen = false;

	if(argc >= 2) {
		for(int i=1; i<argc; i++) {
			if(argv[i][0] == '-') {
				switch(argv[i][1]) {
				case 'f':
					fullScreen = true;
				}
			} else {
				PSFFile f { argv[i] };
				f.getTagData();
				//app.play(SongInfo(argv[i]));
			}
		}
	}

	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(720, 576, false);
	static chipmachine::ChipMachine app;

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
	return 0;	
}

