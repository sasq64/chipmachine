#include <bbsutils/console.h>
#include "SongDb.h"
#include <coreutils/utils.h>
#include "inject.h"
#include "SharedState.h"
#include "SongState.h"
#include "TelnetInterface.h"
#include "PlayerInterface.h"

#include <coreutils/log.h>
#include <mutex>

using namespace std;
using namespace utils;
using namespace bbs;

//extern SharedState playerState;
//extern mutex playMutex;

//void addToQueue(const string &url);

TelnetInterface::TelnetInterface(PlayerInterface *pi) : player(pi) {
}

void TelnetInterface::launchConsole(Console &console, SongDatabase &db) {

	console.flush();
	console.setFg(Console::GREEN);
	console.setBg(Console::BLACK);

	console.setFg(Console::WHITE);
	console.put(0,2,">");
	console.setFg(Console::YELLOW);
	console.moveCursor(1, 2);

	auto query = db.find();
	int marker = 0;
	int start = 0;
	while(true) {

		//char c = session.getChar();
		int c = console.getKey(500);
		int h = console.getHeight();
		console.setFg(Console::WHITE);
		//console.setBg(Console::PINK);
		console.fill(Console::PINK, 0,0, 0, 2);

		{
			lock_guard<mutex>{playMutex};

			//int seconds = playerState["playSeconds"];
			const PlayState &ps = player->getPlayState();
			int seconds = ps.seconds;
			//int seconds = frameCount / 44100;
			int subSong = 0;
			int totalSongs = 0;


			//const SongState &ss = playerState["songState"];
			const SongState &ss = player->getSongState();
			
			string title;// = ss.title;//playerState["title"];
			if(ss.title.length())
				title = format("%s - %s", ss.composer, ss.title);
			else 
				title = "<Nothing playing>";

			injection_point("player.titlebar", title);
			console.put(0, 0, title);


			console.put(0, 1, format("Song %02d/%02d - [%02d:%02d] %s", subSong+1, totalSongs, seconds/60, seconds%60, ss.format));
			console.setBg(Console::BLACK);
			console.setFg(Console::YELLOW);
			console.flush();
			if(c == Console::KEY_TIMEOUT) {
				console.flush();
				continue;
			}

			LOGD("Key pressed: %d", c);

			switch(c) {
				case Console::KEY_BACKSPACE:
				query.removeLast();
				break;
			case Console::KEY_ESCAPE:
				query.clear();
				break;
			case Console::KEY_ENTER:
			//case 13:
			case 10: {
				if(query.numHits() > 0) {
					string r = query.getFull(marker);
					LOGD("RESULT: %s", r);
					auto p  = utils::split(r, "\t");
					for(size_t i = 0; i<p[2].length(); i++) {
						if(p[2][i] == '\\')
							p[2][i] = '/';
					}
					LOGD("Pushing '%s' to queue", p[2]);
					//addToQueue("ftp://modland.ziphoid.com/pub/modules/" + p[2]);
					auto &plist = player->getPlayList();
					plist.push("ftp://modland.ziphoid.com/pub/modules/" + p[2]);
					player->releasePlayList();
				}
				break;
			}
			case 0x11:
				//session.close();
				//doQuit = true;
				return;
			case Console::KEY_DOWN:
				marker++;
				break;
			case Console::KEY_PAGEUP:
			case Console::KEY_F1:
				marker -= (h-5);
				break;
			case Console::KEY_PAGEDOWN:
			case Console::KEY_F7:
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
		
		console.put(1,2,"                       ");

		console.setFg(Console::YELLOW);
		console.put(1,2, query.getString());
		console.moveCursor(1 + query.getString().length(), 2);

		console.setFg(Console::GREEN);

		console.fill(Console::BLACK, 0, 3, console.getWidth(), console.getHeight()-3);

		console.setFg(Console::LIGHT_BLUE);
		console.put(0, marker-start+3, "!");
		console.setFg(Console::GREEN);
		int i = 0;
		
		if(h < 0) h = 40;
		const auto &results = query.getResult(start, h);
		for(const auto &r : results) {
			auto p = split(r, "\t");
			if(p.size() < 3) {
				LOGD("Illegal result line '%s' -> [%s]", r, p);
			} else {
				int index = atoi(p[2].c_str());
				int fmt = db.getFormat(index);
				if(fmt >= 0x10 && fmt <= 0x2f) {
					if(fmt == 0x11)
						console.setFg(Console::LIGHT_GREY);
					else
						console.setFg(Console::GREY);
				} else if(fmt >= 0x30 && fmt <= 0x4f) {
					console.setFg(Console::ORANGE);
				} else if(fmt == 1)
					console.setFg(Console::LIGHT_BLUE);

				console.put(1, i+3, format("%s - %s", p[1], p[0]));
				console.setFg(Console::GREEN);
			}
			i++;
			if(i >= h-3)
				break;
		}

		console.flush();
	}

}
