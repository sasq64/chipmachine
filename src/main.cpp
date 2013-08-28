#include <coreutils/log.h>
#include <coreutils/utils.h>
//#include <utils/var.h>

//#include "SharedState.h"

#include <bbsutils/TelnetServer.h>
#include <bbsutils/Console.h>
//#include "SongState.h"

#include "TelnetInterface.h"

//#include "inject.h"

#include "Player.h"

#include "SongDb.h"

#include <stdio.h>
#include <stdint.h>
//#include <string.h>
//#include <sys/stat.h>
#include <vector>
#include <string>
//#include <memory>
//#include <mutex>
//#include <queue>
#include <cstdlib>
#include <unistd.h>

#ifdef RASPBERRYPI
void lcd_init();
void lcd_print(int x, int y, const std::string &text);
#else
void lcd_init() {}
void lcd_print(int x, int y, const std::string &text) {
	//puts(text.c_str());
	//putchar('\r');
}
#endif

typedef unsigned int uint;
using namespace std;
using namespace utils;
using namespace bbs;

//class SharedState playerState;

int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);
	lcd_init();

	bool daemonize = false;

	//volatile bool doQuit = false;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if((strcmp(argv[i], "--start-daemon") == 0) || (strcmp(argv[i], "-d") == 0)) {
				daemonize = true;
			}
		} else {
			//playQueue.push(argv[1]);
		}
	}
	if(daemonize)
#ifdef WIN32
		sleepms(1);	
#else
	if(daemon(0, 0) != 0)
		throw std::exception();
#endif

	logging::setOutputFile("chipmachine.log");
	//if(playQueue.size() > 0) {
	//	logging::setLevel(logging::WARNING);
	//}


	File startSongs { "/opt/chipmachine/startsongsX" };
	if(startSongs.exists()) {
		for(string s : startSongs.getLines()) {
			//playQueue.push(s);
		}
		startSongs.close();
	}

	Player player;

	LOGI("Opening database");
	SongDatabase db { "modland.db" };
	db.generateIndex();
	LOGI("Index generated");

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {

		session.echo(false);
		
		unique_ptr<Console> console;
		string termType = session.getTermType();
		LOGD("New connection, TERMTYPE '%s'", termType);
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}

		try {
			TelnetInterface ti(&player);
			ti.launchConsole(*console.get(), db);
			exit(0);
		} catch (TelnetServer::disconnect_excpetion &e) {
			LOGD(e.what());
			return;
		}
	});

	telnet.runThread();

	SharedState &g = SharedState::getGlobal("playerState");
	g.callOnChange("songTitle", [&](const string &what) {
		string title = g["songTitle"];
		printf("#### SONG IS NOW %s\n", title.c_str());
	});

	player.run();

	return 0;
}