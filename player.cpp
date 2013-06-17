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

		for(auto &plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file.getName());
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		LOGD("Factory checking: %s\n", lname);

		for(auto &plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	template<class T, class... Args>
	void addPlugin(Args&& ... args) {
		plugins.push_back(unique_ptr<ChipPlugin>(new T(args...)));
	}

	//void registerPlugin(ChipPlugin *p) {	
	//	plugins.push_back(p);
	//}

	unique_ptr<ChipPlayer> play(const string &url) {
		return unique_ptr<ChipPlayer>(new URLPlayer {url, this});
	}

private:
	vector<unique_ptr<ChipPlugin>> plugins;
};


int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);

	lcd_init();

	bool daemonize = false;
	queue<string> playQueue;
	int subSong = 0;
	int totalSongs = 0;
	int currentSong = 0;
	string songTitle;
	string songComposer;

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
	if(daemon(0, 0) != 0)
		throw std::exception();
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
	db.generateIndex();

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {

		LOGD("New connection!");
		session.echo(false);
		AnsiScreen screen { session };
		screen.put(0,0, "Chipmachine starting");
		screen.flush();

		AnsiInput input { session };

		auto query = db.find();

		while(true) {

			try {
				//char c = session.getChar();
				int c = input.getKey(500);

				{
					lock_guard<mutex>{playMutex};
					int seconds = frameCount / 44100;
					screen.put(0, 0, format("%s - %s", songComposer, songTitle));
					screen.put(0, 1, format("Song %02d/%02d - [%02d:%02d]", subSong, totalSongs, seconds/60, seconds%60));

					if(c == AnsiInput::KEY_TIMEOUT) {
						screen.flush();
						continue;
					}

					LOGD("char %d\n", c);
					switch(c) {
						case AnsiInput::KEY_BACKSPACE:
						query.removeLast();
						break;
					case AnsiInput::KEY_ESCAPE:
						query.clear();
						break;
					case 0x11:
						session.close();
						doQuit = true;
						return;
					case AnsiInput::KEY_LEFT:
						if(subSong > 0)
							subSong--;
						continue;
					case AnsiInput::KEY_RIGHT:
						if(subSong < totalSongs-1)
							subSong++;
						continue;
					default:
						if(c >= '0' && c <= '9') {
							string r = query.getFull(c - '0');
							LOGD("RESULT: %s", r);
							auto p  = split(r, "\t");
							LOGD("Pushing '%s' to queue", p[2]);
							playQueue.push("http://swimsuitboys.com/droidsound/dl/C64Music/" + p[2]);
						} else if(c >=0x20) {
							query.addLetter(c);
						} 
					}
				}

				screen.clear();
				screen.put(0,3,">                       ");
				screen.put(1,3, query.getString());

				const auto &results = query.getResult();
				int i = 0;
				int h = session.getHeight();
				if(h < 0) h = 40;
				for(const auto &r : results) {
					auto p = split(r, "\t");
					screen.put(1, i+4, format("[%02d] %s - %s", i, p[1], p[0]));
					i++;
					if(i >= h-4)
						break;
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

						songTitle = player->getMeta("title");
						songComposer = player->getMeta("composer");

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
					lcd_print(14, 3, format("%02d:%02d", seconds/60, seconds%60));
					oldSeconds = seconds;
				}

			}
		} else
			sleepms(250);
	}
	return 0;
}