
#include "database.h"

#include "ChipPlayer.h"
#include "MusicPlayer.h"

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <lua/luainterpreter.h>

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
#include <deque>

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace grappix;
using namespace bbs;

class ChipMachine {
public:
	ChipMachine() {

		modland.init();

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
			std::vector<SongInfo> songs;
			while(true) {
				auto l = console->getLine(">");
				auto cmd = split(l, " ", 1);
				LOGD("%s '%s'", cmd[0], cmd[1]);
				if(cmd[0] == "status") {
					LOGD("%s %s", composer, title);
					console->write(format("%s - %s\n", composer, title));
				} else if (cmd[0] == "find") {
					songs = modland.search(cmd[1]);
					for(const auto &s : songs) {
						console->write(s.path + "\n");
					}
				} else if (cmd[0] == "play") {
					play(songs[0]);
				}
			}
		});
		telnet->runThread();

		font = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);

		AudioPlayer::play([=](int16_t *ptr, int size) mutable {
			if(player)
				player->getSamples(ptr, size);
		});
/*
		if(player) {
			title = player->getMeta("title");
			if(title == "")
				title = path_basename(argv[1]);
			composer = player->getMeta("composer");
		}
		LOGD("TITLE:%s", title);
		float zoom1 = 1.0;
		float zoom2 = 1.0; */
	}

	void play(const SongInfo &si) {
		lock_guard<mutex> guard(plMutex);
		playList.push_back(si);
	}

	void render(uint32_t delta) {
		if(!player && playList.size() > 0) {
			lock_guard<mutex> guard(plMutex);
			LOGD("New song");
			auto si = playList.front();
			playList.pop_front();
			title = si.title;
			composer = si.composer;
			player = mp.fromFile(si.path);			
		}

		screen.clear();
		screen.text(font, title, 0, 0, 0xe080c0ff, 2.0);
		screen.text(font, composer, 0, 52, 0xe080c0ff, 2.0);
		screen.flip();
	}

private:

	MusicPlayer mp;

	unique_ptr<TelnetServer> telnet;

	ModlandDatabase modland;
	string title = "NO TITLE";
	string composer = "NO COMPOSER";
	shared_ptr<ChipPlayer> player;
	std::mutex plMutex;
	std::deque<SongInfo> playList;
	LuaInterpreter lua;

	Font font;

};

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	static ChipMachine app;

	if(argc >= 2)
		app.play(SongInfo(argv[1]));

	screen.render_loop([](uint32_t delta) {
		app.render(delta);
	}, 20);
	return 0;	
}