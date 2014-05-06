
#include "ChipPlayer.h"
#include "MusicPlayer.h"

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <ModPlugin/ModPlugin.h>
#include <VicePlugin/VicePlugin.h>
#include <SexyPSFPlugin/SexyPSFPlugin.h>
#include <GMEPlugin/GMEPlugin.h>
#include <SC68Plugin/SC68Plugin.h>
#include <UADEPlugin/UADEPlugin.h>

#include <grappix/grappix.h>

#include <coreutils/utils.h>
#include <audioplayer/audioplayer.h>
#include <cstdio>
#include <vector>
#include <string>

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace grappix;
using namespace bbs;

unique_ptr<TelnetServer> telnet;

int main(int argc, char* argv[]) {

	string title;
	string composer;

	telnet = make_unique<TelnetServer>(12345);
	telnet->setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		string termType = session.getTermType();		
		LOGD("New connection, TERMTYPE '%s'", termType);

		unique_ptr<Console> console;
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();
		while(true) {
			auto l = console->getLine(">");
			if(l == "status") {
				console->write(format("%s - %s\n", composer, title));
			}
		}
	});
	telnet->runThread();


	screen.open(720, 576, false);

	Font font = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);

	MusicPlayer mp;
	shared_ptr<ChipPlayer> player;

	if(argc >= 2)
		player = mp.fromFile(argv[1]);

	AudioPlayer ap ([=](int16_t *ptr, int size) mutable {
		if(player)
			player->getSamples(ptr, size);
	});

	if(player) {
		title = player->getMeta("title");
		if(title == "")
			title = path_basename(argv[1]);
		composer = player->getMeta("composer");
	}
	LOGD("TITLE:%s", title);
	float zoom1 = 1.0;
	float zoom2 = 1.0;
	screen.render_loop([=](uint32_t delta) mutable {
		screen.clear();
		screen.text(font, title, 0, 0, 0xe080c0ff, zoom1 *= 1.01);
		screen.text(font, composer, 0, 52, 0xe080c0ff, zoom2 *= 1.03);
		screen.flip();
	});

	return 0;	
}