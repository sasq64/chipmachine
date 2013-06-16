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

#include "SongDb.h"

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

	template<class T, class... Args>
	void addPlugin(Args&& ... args) {
		plugins.push_back(new T(args...));
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

	unique_ptr<ChipPlayer> play(const string &url) {
		return unique_ptr<ChipPlayer>(new URLPlayer {url, this});
	}

private:
	vector<ChipPlugin*> plugins;
};


int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);

	lcd_init();

	bool daemonize = false;
	queue<string> playQueue;
	int subSong = 0;
	int totalSongs = 0;
	int currentSong = 0;

	volatile bool doQuit = false;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if((strcmp(argv[i], "--start-daemon") == 0) || (strcmp(argv[i], "-d") == 0)) {
				daemonize = true;
			}
		} else {
			playQueue.push(argv[1]);
		}
	}
	if(daemonize)
#ifdef WIN32
		sleepms(1);	
#else
		int rc = daemon(0, 0);
#endif

	logging::setOutputFile("chipmachine.log");
	if(playQueue.size() > 0) {
		logging::setLevel(logging::WARNING);
	}

	mutex playMutex;

	File startSongs { "/opt/chipmachine/startsongsX" };
	if(startSongs.exists()) {
		for(string s : startSongs.getLines()) {
			playQueue.push(s);
		}
		startSongs.close();
	}

	PlayerSystem psys;
	psys.addPlugin<ModPlugin>();
	psys.addPlugin<VicePlugin>();
	psys.addPlugin<SexyPSFPlugin>();
	psys.addPlugin<GMEPlugin>();

	unique_ptr<ChipPlayer> player = nullptr; //psys.play(name);

	int frameCount = 0;
	string songName;

	SongDatabase db { "hvsc.db" };

	//db.generateIndex();
	//db.search("ghost");
	//return 0;

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

		LOGD("New connection!");
		session.echo(false);
		AnsiScreen screen { session };
		//screen.setFg(2);
		//screen.put(5,5, "Chipmachine");
		//screen.setFg(4);
		//screen.put(3,3, "Chipmachine");
		screen.put(0,0, "Chipmachine Login");
		screen.flush();

		AnsiInput input { session };

		auto query = db.find();

		while(true) {

			try {
				//char c = session.getChar();
				int c = input.getKey();
				LOGD("char %d\n", c);
				if(c == AnsiInput::KEY_BACKSPACE)
					query.removeLast();
				else if(c >= '0' && c <= '9') {
					string r = query.getResult()[c - '0'];
					auto p  = split(r, "\t");
					lock_guard<mutex>{playMutex};
					LOGD("Pushing '%s' to queue", p[0]);
					playQueue.push("http://swimsuitboys.com/droidsound/dl/C64Music/" + p[0]);
				} else if(c == 0x11) {
					lock_guard<mutex>{playMutex};
					session.close();
					doQuit = true;
					return;
				}  else if(c == AnsiInput::KEY_LEFT) {
					lock_guard<mutex>{playMutex};
					if(subSong > 0)
						subSong--;
				}  else if(c == AnsiInput::KEY_RIGHT) {
					lock_guard<mutex>{playMutex};
					if(subSong < totalSongs-1)
						subSong++;
				} else if(c >=0x21)
					query.addLetter(c);
				//session.write({ '\x1b', '[', '2', 'J' }, 4);
				//session.write("\x1b[2J\x1b[%d;%dH", 1, 1);
				screen.put(0,0,"                       ");
				screen.put(0,0, query.getString());
				//session.write("[%s]\r\n\r\n", query.getString());
				if(query.numHits() > 0) {
					screen.clear();
					screen.put(0,0, query.getString());
					const auto &results = query.getResult();
					int i = 0;					
					for(const auto &r : results) {
						auto p = split(r, "\t");
						//session.write("[%d] %s - %s\r\n", i++, p[2], p[1]);
						screen.put(1, i+2, format("[%02d] %s - %s", i, p[2], p[1]));
						i++;
						if(i > 38)
							break;
					}
				}
				screen.flush();
			} catch (TelnetServer::disconnect_excpetion &e) {
				LOGD(e.what());
				return;
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

		if(doQuit) {
			telnet.stop();
			return 0;
		}

		{
			lock_guard<mutex> guard(playMutex);
			if(!playQueue.empty()) {
				//if(player)
				//	delete player;
				songName = playQueue.front();
				LOGD("Found '%s' in queue", songName);
				player = psys.play(songName);

				player->onMeta([&](const string &meta, ChipPlayer *player) {
					if(meta == "metaend") {
						LOGD("Now playing: %s - %s", player->getMeta("composer"), player->getMeta("title"));
						totalSongs = player->getMetaInt("songs");
						int startsong = player->getMetaInt("startsong");
						subSong = currentSong = startsong;

						lcd_print(0,0, player->getMeta("title"));
						lcd_print(0,1, player->getMeta("composer"));
						lcd_print(0,2, player->getMeta("copyright"));
						lcd_print(0,3, format("Song %02d/%02d - [00:00]", subSong, totalSongs));
					}
				});
				oldSeconds = 0;
				playQueue.pop();
				frameCount = 0;
			}
			if(subSong != currentSong) {
				player->seekTo(subSong);
				currentSong = subSong;
			}

		}

		if(player) {
			int rc = player->getSamples(&buffer[0], bufSize);
			if(rc > 0) {
				ap.writeAudio(&buffer[0], rc);
				frameCount += rc/2;

				int seconds = frameCount / 44100;
				if(seconds != oldSeconds) {
					lcd_print(14, 3, format("%02d:%02d", seconds/60, seconds%60));
					oldSeconds = seconds;
				}

			}
		} else
			sleepms(250);
	}
	return 0;
}