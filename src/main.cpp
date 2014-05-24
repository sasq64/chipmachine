#include "database.h"

#include <musicplayer/chipplayer.h>

#include "MusicPlayer.h"
#include "PlayerScreen.h"

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <lua/luainterpreter.h>
#include <webutils/webgetter.h>

#include <musicplayer/plugins/ModPlugin/ModPlugin.h>
#include <musicplayer/plugins/VicePlugin/VicePlugin.h>
#include <musicplayer/plugins/SexyPSFPlugin/SexyPSFPlugin.h>
#include <musicplayer/plugins/GMEPlugin/GMEPlugin.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <musicplayer/plugins/UADEPlugin/UADEPlugin.h>

#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <deque>
#include <atomic>
#include <memory>

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace grappix;
using namespace bbs;
using namespace tween;

class MusicPlayerList {
public:
	enum State {
		STOPPED,
		WAITING,
		STARTED,
		PLAY_STARTED,
		PLAYING
	};

	void addSong(const SongInfo &si) {
		lock_guard<mutex> guard(plMutex);
		playList.push_back(si);
	}

	void clearSongs() {
		lock_guard<mutex> guard(plMutex);
		playList.clear();
	}

	void nextSong() {
		lock_guard<mutex> guard(plMutex);
		mp.stop();
		state = WAITING;
	}

	void playFile(const std::string &fileName) {
		lock_guard<mutex> guard(plMutex);
		mp.playFile(fileName);
		state = PLAY_STARTED;
		length = mp.getLength();
		auto si = mp.getPlayingInfo();
		if(si.title != "")
			currentInfo.title = si.title;
		if(si.composer != "")
			currentInfo.composer = si.composer;
		if(si.format != "")
			currentInfo.format = si.format;
	}

	State update() {
		if(state == PLAYING && !mp.playing()) {
			LOGD("#### Music ended");
			if(playList.size() == 0)
				state = STOPPED;
			else
				state = WAITING;
		}

		if(state == PLAY_STARTED) {
			state = PLAYING;
		}

		if(state == WAITING && playList.size() > 0) {
			{
				lock_guard<mutex> guard(plMutex);
				state = STARTED;
				currentInfo = playList.front();
				playList.pop_front();
				//pos = 0;
			}
			LOGD("##### New song: %s", currentInfo.path);

			auto proto = split(currentInfo.path, ":");
			if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
				webgetter.getURL(currentInfo.path, [=](const WebGetter::Job &job) {
					playFile(job.getFile());
				});
			} else {
				playFile(currentInfo.path);
			}
		}

		return state;
	}

	uint16_t *getSpectrum() {
		return mp.getSpectrum();
	}

	int spectrumSize() {
		return mp.spectrumSize();
	}

	SongInfo getInfo(int index = 0) {
		if(index == 0)
			return currentInfo;
		else
			return playList[index-1];
	}

	int getLength() {
		return length;
	}

	int getPosition() {
		return mp.getPosition();
	}

	int listSize() {
		return playList.size();
	}

private:
	MusicPlayer mp;
	std::mutex plMutex;
	std::deque<SongInfo> playList;

	WebGetter webgetter { "_files" };

	State state = STOPPED;
	int length;
	SongInfo currentInfo;

};


class ChipMachine {
public:
	ChipMachine() : currentScreen(&mainScreen), eq(SpectrumAnalyzer::eq_slots)  {

		iquery = modland.createQuery();

		lua.registerFunction<void, int>("play", [=](int a) {
		});

		lua.registerFunction<int>("play_seconds", [=]() -> int {
			return 123;
		});

		lua.loadFile("lua/init.lua");

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

			LuaInterpreter lip;

			lip.setOuputFunction([&](const std::string &s) {
				console->write(s);
			});

			lip.loadFile("lua/init.lua");

			lip.registerFunction<int, string, int, int, float, int>("Infoscreen.add", [&](const string &q, int x, int y, float scale, int color) -> int {
				return 0;
			});

			lip.registerFunction<void, string>("find", [&](const string &q) {
				songs = modland.find(q);
				int i = 0;
				for(const auto &s : songs) {
					console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
				}				
			});

			lip.registerFunction<void, int>("play", [&](int which) {
				player.clearSongs();
				player.addSong(songs[which]);
				player.nextSong();
			});

			while(true) {
				auto l = console->getLine(">");
				auto parts = split(l, " ");
				if(isalpha(parts[0]) && (parts.size() == 1 || parts[1][0] != '=')) {
					l = parts[0] + "(";
					for(int i=1; i<(int)parts.size(); i++) {
						if(i!=1) l += ",";
						l += parts[i];
					}
					l += ")";
					LOGD("Changed to %s", l);
				}
				if(!lip.load(l))
					console->write("** SYNTAX ERROR\n");
			/*
				auto cmd = split(l, " ", 1);
				//LOGD("%s '%s'", cmd[0], cmd[1]);
				if(cmd[0] == "status") {
					//LOGD("%s %s", composer, title);
					//console->write(format("%s - %s\n", composer, title));
				} else if (cmd[0] == "find") {
					songs = modland.search(cmd[1]);
					int i = 0;
					for(const auto &s : songs) {
						console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
					}
				} else if (cmd[0] == "play") {
					int which = atoi(cmd[1].c_str());
					play(songs[which]);
					next();
				} else if (cmd[0] == "q") {
					int which = atoi(cmd[1].c_str());
					play(songs[which]);
				} else if (cmd[0] == "next") {
					next();
				}*/
			}
		});
		telnet->runThread();

		memset(&eq[0], 2, eq.size());

		auto mfont = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);
		auto sfont = Font("data/topaz.ttf", 16, 128 | Font::DISTANCE_MAP);

		mainScreen.setFont(mfont);
		searchScreen.setFont(mfont);

		prevInfoField = SongInfoField(mainScreen, -3200, 64, 10.0, 0x00e0e080);
		currentInfoField = SongInfoField(mainScreen, tv0.x, tv0.y, 2.0, 0xffe0e080);
		nextInfoField = SongInfoField(mainScreen, 440, 340, 0.8, 0xffe0e080);
		outsideInfoField = SongInfoField(800, 340, 0.8, 0xffe0e080);

		nextField = mainScreen.addField("next", 440, 320, 0.5, 0xe080c0ff);		


		timeField = mainScreen.addField("00:00", tv0.x, 188, 1.0, 0xff888888);
		lengthField = mainScreen.addField("(00:00)", 200, 188, 1.0, 0xff888888);


		searchField = searchScreen.addField("#", tv0.x, tv0.y, 1.0, 0xff888888);
		for(int i=0; i<20; i++) {
			resultField.push_back(searchScreen.addField("", tv0.x, tv0.y+30+i*22, 0.8, 0xff008000));
		}


	}

	void play(const SongInfo &si) {
		player.addSong(si);
	}
/*
	void clear() {
		lock_guard<mutex> guard(plMutex);
		playList.clear();
	}

	void next() {
		lock_guard<mutex> guard(plMutex);
		//player = nullptr;
		mp.stop();
		state = WAITING;
	}
*/
	void render(uint32_t delta) {

		auto k = screen.get_key();

		if(k >= '1' && k <= '9') {
			// TODO : If more than 9 songs, require 2 presses
			// and also display pressed digits in corner
			currentScreen = &searchScreen;
			iquery.addLetter(tolower(k));
			//mp.seek(k - '1');
			//length = mp.getLength();
		} else if(k >= 'A' && k<='Z') {
			currentScreen = &searchScreen;
			iquery.addLetter(tolower(k));
		} else {
			switch(k) {
			case Window::F1:
				currentScreen = &mainScreen;
				break;
			case Window::F2:
				currentScreen = &searchScreen;
				break;
			case Window::SPACE:
				currentScreen = &searchScreen;
				iquery.addLetter(' ');
				//next();
				break;
			case Window::BACKSPACE:
				currentScreen = &searchScreen;
				iquery.removeLast();
				break;
			case Window::F9:
				currentScreen = &mainScreen;
				player.nextSong();
				break;
			case Window::F12:
				player.clearSongs();
				break;		
			case Window::UP:
				marked--;
				break;
			case Window::DOWN:
				marked++;
				break;
			case Window::PAGEUP:
				marked -= 20;
				break;
			case Window::PAGEDOWN:
				marked += 20;
				break;
			case Window::ENTER:
				{
					auto r = iquery.getFull(marked);
					auto parts = split(r, "\t");
					LOGD("######### %s", parts[0]);
					SongInfo si(string("ftp://ftp.modland.com/pub/modules/") + parts[0], parts[1], parts[2], parts[3]);
					if(!(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_LEFT))) {
						player.clearSongs();
						player.addSong(si);
						player.nextSong();
						//playList.clear();
						//playList.push_back(si);
						//next();
						currentScreen = &mainScreen;
					} else
						player.addSong(si); 
						//playList.push_back(si);
				}
				break;
			}
		}
/*
		if(state == PLAYING && !mp.playing()) {
			LOGD("#### Music ended");
			if(playList.size() == 0)
				state = STOPPED;
			else
				state = WAITING;
		}

		if(state == WAITING && playList.size() > 0) {
			lock_guard<mutex> guard(plMutex);
			state = STARTED;
			currentInfo = playList.front();
			playList.pop_front();
			pos = 0;
			LOGD("##### New song: %s", currentInfo.path);
			//title = si.title;
			//composer = si.composer;

			auto proto = split(currentInfo.path, ":");
			if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
				webgetter.getURL(currentInfo.path, [=](const WebGetter::Job &job) {
					mp.playFile(job.getFile());
					state = PLAY_STARTED;
					length = mp.getLength();
					auto si = mp.getPlayingInfo();
					if(si.title != "")
						currentInfo.title = si.title;
					if(si.composer != "")
						currentInfo.composer = si.composer;
					if(si.format != "")
						currentInfo.format = si.format;
			});
			} else {
				mp.playFile(currentInfo.path);
				state = PLAY_STARTED;
				length = mp.getLength();
				auto si = mp.getPlayingInfo();
				if(si.title != "")
					currentInfo.title = si.title;
				if(si.composer != "")
					currentInfo.composer = si.composer;
				if(si.format != "")
					currentInfo.format = si.format;
			}
		}
*/

		//if(state == PLAY_STARTED) {
		if(player.update() == MusicPlayerList::PLAY_STARTED) {
			//state = PLAYING;
			prevInfoField.setInfo(currentInfoField.getInfo());
			currentInfoField.setInfo(player.getInfo());

			currentTween.finish();
			currentTween = make_tween().from(prevInfoField, currentInfoField).
			from(currentInfoField, nextInfoField).
			from(nextInfoField, outsideInfoField).seconds(1.5);
		}


		auto psz = player.listSize();
		if(psz > 0) {
			auto n = player.getInfo(1);
			if(n.path != currentNextPath) {
				if(n.title == "") {
					n.title = utils::path_filename(urldecode(n.path, ""));
				}

				if(psz == 1)
					nextField->text = "Next";
				else
					nextField->text = format("Next (%d)", psz);
				nextInfoField.setInfo(n);
				currentNextPath = n.path;
			}
		} else
			nextField->text = "";

/*
		int psz = playList.size();

		if(psz > 0 && nextPath != playList.front().path) {
			SongInfo next("");

			auto &n = playList.front();
			if(n.title == "") {
				n.title = utils::path_filename(urldecode(n.path, ""));
			}
			next = n;
			nextPath = n.path;

			if(psz == 0)
				nextField->text = "";
			else if(psz == 1)
				nextField->text = "Next";
			else
				nextField->text = format("Next (%d)", psz);

			//nextTitleField->text = next.title;
			//nextComposerField->text = next.composer;
			nextInfoField.setInfo(next);
		}
*/
		//vec2i xy = { 100, 100 };

		screen.clear();
		//screen.text(font, title, 90, 64, 0xe080c0ff, 2.2);
		//screen.text(font, composer, 90, 130, 0xe080c0ff, 1.2);

		//screen.rectangle(tv0, tv1-tv0, 0xff444488);

		auto spectrum = player.getSpectrum();
		for(int i=0; i<(int)player.spectrumSize(); i++) {
			if(spectrum[i] > 5) {
				float f = log(spectrum[i]) * 20;
				if(f > eq[i])
					eq[i] = f;
			}
		}

		for(int i=0; i<(int)eq.size(); i++) {
			screen.rectangle(tv0.x-10 + 24*i, tv1.y+20-eq[i], 23, eq[i], 0xffffffff);
			if(eq[i] >= 4)
				eq[i]-=2;
			else
				eq[i] = 2;
		}

		auto p = player.getPosition();
		int length = player.getLength();
		timeField->text = format("%02d:%02d", p/60, p%60);
		lengthField->text = format("(%02d:%02d)", length/60, length%60);

		auto oldscrollpos = scrollpos;
		int nh = iquery.numHits();

		if(marked < 0) marked = 0;
		if(marked >= nh)
			marked = nh-1;

		if(marked < 0) marked = 0;

		if(marked < scrollpos)
			scrollpos = marked;
		if(marked >= scrollpos + 20)
			scrollpos = marked-20+1;


		searchField->text = "#" + iquery.getString();
		if(iquery.newResult() || scrollpos != oldscrollpos) {

			//if(nh > 20) nh = 20;
			auto count = 20;
			if(nh > 0) {
				if(scrollpos + count >= nh) count = nh - scrollpos;
				const auto &res = iquery.getResult(scrollpos, count);
				LOGD("HITS %d COUNT %d", nh, count);
				for(int i=0; i<20; i++) {
					if(i < count) {
						auto parts = split(res[i], "\t");
						resultField[i]->text = format("%s / %s", parts[0], parts[1]);
					} else resultField[i]->text = "";
				}
			} else {
				for(int i=0; i<20; i++)
					resultField[i]->text = "";
			}
		}

		//resultField[marked]->scale = sin(counter*2.0*M_PI/100.0)+1)*0.75;
		//counter = (counter+1)%100;

		marked_field = marked-scrollpos;

		if(marked_field != old_marked) {

			LOGD("MARKED %d SCROLLPOS %d", marked, scrollpos);
			
			if(markTween.valid()) {
				markTween.cancel();
				make_tween().to(resultField[old_marked]->g, 0.5f).seconds(1.0);
			}
			if(iquery.numHits() > 0) {
				const auto &res = iquery.getFull(scrollpos+marked);
				LOGD("%s", res);
			}
/*
			for(int i=0; i<20; i++) {
				float y = i/20.0;
				y = pow(y, 0.9);
				float s = 0.8 / y;
				y = tv0.y+30+(y*20.0*22.0);
				LOGD("%f", y);
				make_tween().to(resultField[i]->scale, s).to(resultField[i]->pos.y, y);
			}
*/
			resultField[marked_field]->g = 0.5;
			markTween = make_tween().sine().repeating().from(resultField[marked_field]->g, 1.0f).seconds(1.0);
			old_marked = marked_field;
		}

		currentScreen->render(delta);

		screen.flip();
	}

private:

	IncrementalQuery query;

	//MusicPlayer mp;
	MusicPlayerList player;

	PlayerScreen mainScreen;
	PlayerScreen searchScreen;

	PlayerScreen *currentScreen;

	unique_ptr<TelnetServer> telnet;

	ModlandDatabase modland;
	//string title = "NO TITLE";
	//string composer = "NO COMPOSER";
	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;
/*
	shared_ptr<PlayerScreen::TextField> titleField;
	shared_ptr<PlayerScreen::TextField> composerField;
	shared_ptr<PlayerScreen::TextField> nextTitleField;
	shared_ptr<PlayerScreen::TextField> nextComposerField;
	shared_ptr<PlayerScreen::TextField> prevTitleField;
	shared_ptr<PlayerScreen::TextField> prevComposerField;
*/
	shared_ptr<PlayerScreen::TextField> timeField;
	shared_ptr<PlayerScreen::TextField> lengthField;
	shared_ptr<PlayerScreen::TextField> nextField;

	std::vector<shared_ptr<PlayerScreen::TextField>> resultField;
	shared_ptr<PlayerScreen::TextField> searchField;

	vec2i tv0 = { 80, 54 };
	vec2i tv1 = { 636, 520 };

	int marked = 0;
	int old_marked = -1;
	int scrollpos = 0;
	int marked_field = 0;
	TweenHolder markTween;
	string currentNextPath;

	//atomic<int> pos;
	// shared_ptr<ChipPlayer> player;
	LuaInterpreter lua;


	Font font;

	TweenHolder currentTween;
/*
	std::mutex plMutex;
	int length = 0;
	std::deque<SongInfo> playList;
	WebGetter webgetter { "_files" };

	enum State {
		STOPPED,
		WAITING,
		STARTED,
		PLAY_STARTED,
		PLAYING
	};

	State state = STOPPED;

	SongInfo currentInfo;
*/
	std::vector<uint8_t> eq;

	IncrementalQuery iquery;

};

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	static ChipMachine app;

	if(argc >= 2) {
		for(int i=1; i<argc; i++)
			app.play(SongInfo(argv[i]));
	}

	screen.render_loop([](uint32_t delta) {
		app.render(delta);
	}, 20);
	return 0;	
}