
#include "ChipMachine.h"

#include <grappix/grappix.h>
#include <musicplayer/PSFFile.h>

int main(int argc, char* argv[]) {

	grappix::screen.open(720, 576, false);
	//grappix::screen.open(true);
	static chipmachine::ChipMachine app;

	if(argc >= 2) {
		for(int i=1; i<argc; i++) {

			PSFFile f { argv[i] };
			f.getTagData();

			app.play(SongInfo(argv[i]));
		}
	}

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
	return 0;	
}

