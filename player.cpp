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

	for(int i=1; i<argc; i++) 
{		if(argv[i][0] == '-') {
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
		console->flush();
		//console->put(0,0, "Chipmachine starting");
		//console->flush();
		//console->clear();
		console->setFg(Console::GREEN);
		console->setBg(Console::BLACK);

		console->setFg(Console::WHITE);
		console->put(0,2,">");
		console->setFg(Console::YELLOW);
		console->moveCursor(1, 2);

		auto query = db.find();
		int marker = 0;
		int start = 0;
		while(true) {

			try {
				//char c = session.getChar();
				int c = console->getKey(500);
				int h = console->getHeight();
				{
					lock_guard<mutex>{playMutex};


					int seconds = frameCount / 44100;

					console->setFg(Console::WHITE);
					console->setBg(Console::PINK);

					console->fill(0,0, console->getWidth(), 2);

					if(songTitle.length())
						console->put(0, 0, format("%s - %s", songComposer, songTitle));
					else
						console->put(0, 0, "<Nothing playing>");
					console->put(0, 1, format("Song %02d/%02d - [%02d:%02d]", subSong+1, totalSongs, seconds/60, seconds%60));
					console->setBg(Console::BLACK);
					console->setFg(Console::YELLOW);
					console->flush();
					if(c == Console::KEY_TIMEOUT) {
						console->flush();
						continue;
					}

					LOGD("char %d\n", (int)c);

					switch(c) {
						case Console::KEY_BACKSPACE:
						query.removeLast();
						break;
					case Console::KEY_ESCAPE:
						query.clear();
						break;
					case Console::KEY_ENTER:
					case 13:
					case 10: {
						string r = query.getFull(marker+start);
						LOGD("RESULT: %s", r);
						auto p  = split(r, "\t");
						for(size_t i = 0; i<p[2].length(); i++) {
							if(p[2][i] == '\\')
								p[2][i] = '/';
						}
						LOGD("Pushing '%s' to queue", p[2]);
						playQueue.push("ftp://modland.ziphoid.com/pub/modules/" + p[2]);
						break;
					}
					case 0x11:
						session.close();
						doQuit = true;					
						return;
					case Console::KEY_DOWN:
						marker++;
						break;
					case Console::KEY_PAGEUP:
						marker -= (h-5);
						break;
					case Console::KEY_PAGEDOWN:
						marker += (h-5);
						break;
					case Console::KEY_UP:
						marker--;
						break;
					case Console::KEY_LEFT:
						if(subSong > 0)
							subSong--;
						continue;
					case Console::KEY_RIGHT:
						if(subSong < totalSongs-1)
							subSong++;
						continue;
					default:
						if(isalnum(c) || c == ' ') {
							query.addLetter(c);
						} 
					}
				}

				if(marker >= query.numHits())
					marker = query.numHits()-1;
				if(marker < 0) marker = 0;

				if(marker < start)
					start = marker;
				if(marker > start+h-4)
					start = marker;
				
				console->put(1,2,"                       ");

				console->setFg(Console::YELLOW);
				console->put(1,2, query.getString());
				console->moveCursor(1 + query.getString().length(), 2);

				console->setFg(Console::GREEN);

				console->fill(0, 3, console->getWidth(), console->getHeight()-3);

				console->setFg(Console::LIGHT_BLUE);
				console->put(0, marker-start+3, "!");
				console->setFg(Console::GREEN);
				int i = 0;
				
				if(h < 0) h = 40;
				const auto &results = query.getResult(start, h);
				for(const auto &r : results) {
					auto p = split(r, "\t");
					console->put(1, i+3, format("%03d. %s - %s", start+i, p[1], p[0]));
					i++;
					if(i >= h-3)
						break;
				}

				//console->setFg(Console::YELLOW);
				//console->moveCursor(query.getString().length()+1,3);

				console->flush();
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

				songTitle = path_basename(songName);
				songComposer = "";

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
						lcd_print(0,3, format("Song %02d/%02d - [00:00]", subSong+1, totalSongs));
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