#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <cstdlib>

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "utils.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "SexyPSFPlugin.h"
#include "GMEPlugin.h"

#include "TelnetServer.h"


#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include "Archive.h"

#include <unistd.h>


typedef unsigned int uint;
using namespace std;
using namespace utils;
using namespace logging;
/*
namespace aec {
	std::string gotoxy(int x, int y) {
		char temp[16];
		sprintf(temp, "\x1b[%d;%dH", x, y);
		return string(temp);
	}
}

class AnsiScreen : public Screen {
public:
	void update() {
		for(int y = 0; y<height; y++) {
			for(int x = 0; x<width; x++) {
				Tile &t0 = oldGrid[x+y*width];
				Tile &t1 = grid[x+y*width];
				if(t0 != t1) {					
					if(curY != y or curX != x)
						smartGoto(x, y);
					if(t0.fg != t1.fg || t0.bg != t1.bg)
						setColor(t1.fg, t1.bg);
					putChar(t1.c);
				}
			}
		}
		//for(auto &f : fragments) {
		//	if(f.x != x || f.y != y)
		//		ansiGotoxy(f.x, f.y);
		//	write(f.text);
		//}
	}

	void setColor(int fg, int bg) {
	}

	void putChar(char c) {
		write({c});
		curX++;
		if(curX > width) {
			curX -= width;
			curY++;
		}
	}



	void smartGoto(int x, int y) {
		// Not so smart for now
		char temp[16];
		sprintf(temp, "\x1b[%d;%dH", x, y);
		write(temp);
		curX = x;
		curY = y;
	}

	void ansiGotoxy(int x, int y) {
	}
	void write(const string &text) {
	}
};

//class LineEditor : Editor {
//public:
//};

*/
class PlayerSystem : public PlayerFactory {
public:
	virtual ChipPlayer *fromFile(File &file) override {

		string name = file.getName();
		makeLower(name);
		LOGD("Handling %s\n", name);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file.getName());
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		LOGD("Factory checking: %s\n", lname);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

	ChipPlayer *play(const string &url) {
		return new URLPlayer {url, this};
	}

private:
	vector<ChipPlugin*> plugins;
};


int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("Chipmachine starting\n");

	std::string t = "gurka";

	//puts(format("Hello %% from '%s' and %s %d%%\n", argv[0], t, 19).c_str());

	bool daemonize = false;
	queue<string> playQueue;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if((strcmp(argv[i], "--start-daemon") == 0) || (strcmp(argv[i], "-d") == 0)) {
				daemonize = true;
			}
		} else 
			playQueue.push(argv[1]);
	}
	if(daemonize)
#ifdef WIN32
		sleepms(1);	
#else
		daemon(0, 0);
#endif


	mutex playMutex;

	File startSongs { "/opt/chipmachine/startsongs" };
	if(startSongs.exists()) {
		for(string s : startSongs.getLines()) {
			playQueue.push(s);
		}
		startSongs.close();
	}

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {});
	psys.registerPlugin(new SexyPSFPlugin {});
	psys.registerPlugin(new GMEPlugin {});

	ChipPlayer *player = nullptr; //psys.play(name);

	int frameCount = 0;
	string songName;

	TelnetServer telnet { 12345 };

/*

	telnet.addCommand("play", [&](TelnetServer::Session &session, const vector<string> &args) {
		//printf("Play '%s'\n", args[1].c_str());
		{ lock_guard<playMutex>;
			playQueue.push(args[1]);
		}
	});

	telnet.addCommand("go", [&](TelnetServer::Session &session, const vector<string> &args) {
		int song = atoi(args[1].c_str());
		if(player && song >= 0) {
			player->seekTo(song);
			session.write("Setting song %d\r\n", song);
		}
	});

	telnet.addCommand("status", [&](TelnetServer::Session &session, const vector<string> &args) {
		if(player) {
			session.write("Playing '%s' for %d seconds\r\n", songName, frameCount / 44100);
		} else {
			session.write("Nothing playing\r\n");
		}
	}); */

	//telnet.onData([&](TelnetServer::Session &session) {
	//});


	telnet.setConnectCallback([&](shared_ptr<TelnetServer::Session> session) {

		//auto userSession = make_shared<UserSession>();
		//session.addUserSession(userSession);
		//AnsiScreen screen;
		//screen.put(5,5, "Chipmachine");
		//screen.update(session);
		session->write("\r\nName:");
		string name = session->getLine();
		LOGD("Line\n");
		session->write("Name is '%s'\r\n", name);
		string pass = session->getLine();
		session->write("Pass: %s\r\n", pass);


	});

	telnet.runThread();

	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	while(true) {
		{
			lock_guard<mutex> guard(playMutex);
			if(!playQueue.empty()) {
				if(player)
					delete player;
				songName = playQueue.front();
				player = psys.play(songName);
				playQueue.pop();
				frameCount = 0;
			}
		}

		if(player) {
			int rc = player->getSamples(&buffer[0], bufSize);
			if(rc > 0) {
				ap.writeAudio(&buffer[0], rc);
				frameCount += rc/2;
			}
		} else
			sleepms(250);
	}
	return 0;
}