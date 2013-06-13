#include "log.h"

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "utils.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "SexyPSFPlugin.h"
#include "GMEPlugin.h"

#include "TelnetServer.h"
#include "TextScreen.h"

#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include "Archive.h"

#include "SongDb.cpp"

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
#include <unistd.h>

#ifdef PI
void lcd_init();
void lcd_print(int x, int y, const std::string &text);
#else
void lcd_init() {}
void lcd_print(int x, int y, const std::string &text) {
	puts(text.c_str());
	putchar('\n');
}
#endif


typedef unsigned int uint;
using namespace std;
using namespace utils;
using namespace logging;

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

	vector<string> strings { "kalle", "bertil" };
	vector<short> x { 0,1,2,3,999,45678};
	LOGD("Chipmachine starting\n");
	LOGD("Chipmachine %% starting '%s' & '%04x' and %03d%% number %04x in hex\n", strings, x, 23, 40);

	lcd_init();

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

	SongDatabase db { "hvsc.db" };

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


	telnet.setOnConnect([&](TelnetServer::Session &session) {

		AnsiScreen screen { session };
		screen.setFg(2);
		screen.put(5,5, "Chipmachine");
		screen.setFg(4);
		screen.put(3,3, "Chipmachine");

		auto query = db.find();

		while(true) {

			char c = session.getChar();
			LOGD("char %d\n", c);
			if(c == 127)
				query.removeLast();
			else if(c >= '0' && c <= '9') {
				string r = query.getResult()[c - '0'];
				auto p  = split(r, "\t");
				lock_guard<mutex>{playMutex};
				LOGD("Pushing '%s' to queue", p[0]);
				playQueue.push("http://swimsuitboys.com/droidsound/dl/C64Music/" + p[0]);
			} else
				query.addLetter(c);
			//session.write({ '\x1b', '[', '2', 'J' }, 4);
			session.write("\x1b[2J\x1b[%d;%dH", 1, 1);
			session.write("[%s]\r\n\r\n", query.getString());
			if(query.numHits() > 0) {
				const auto &results = query.getResult();
				int i = 0;
				for(const auto &r : results) {
					auto p = split(r, "\t");
					session.write("[%d] %s - %s\r\n", i++, p[2], p[1]);
				}
			}

			/*session.write("\r\n>> ");
			auto line = session.getLine();
			auto args = split(line);
			if(args[0] == "play") {
				lock_guard<mutex>{playMutex};
				//LOGD("Pushing '%s' to queue", st.getString(1));
				playQueue.push(args[1]);
			} */
		}
	});

	telnet.runThread();

	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	int oldSeconds = -1;
	while(true) {
		{
			lock_guard<mutex> guard(playMutex);
			if(!playQueue.empty()) {
				if(player)
					delete player;
				songName = playQueue.front();
				LOGD("Found '%s' in queue", songName);
				player = psys.play(songName);

				LOGD("Now playing: %s - %s", player->getMeta("composer"), player->getMeta("title"));

				int songs = player->getMetaInt("songs");
				int startsong = player->getMetaInt("startsong");

				lcd_print(0,0, player->getMeta("title"));
				lcd_print(0,1, player->getMeta("composer"));
				lcd_print(0,2, player->getMeta("copyright"));
				lcd_print(0,3, format("Song %02d/%02d -- [00:00]", startsong, songs));
				oldSeconds = 0;
				playQueue.pop();
				frameCount = 0;
			}
		}

		if(player) {
			int rc = player->getSamples(&buffer[0], bufSize);
			if(rc > 0) {
				ap.writeAudio(&buffer[0], rc);
				frameCount += rc/2;

				int seconds = frameCount / 44100;
				if(seconds != oldSeconds) {
					lcd_print(0, 15, format("%02d:%02d", seconds/60, seconds%60));
					oldSeconds = seconds;
				}

			}
		} else
			sleepms(250);
	}
	return 0;
}