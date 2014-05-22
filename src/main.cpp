#include "database.h"

#include <musicplayer/chipplayer.h>

#include "MusicPlayer.h"

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


class PlayerScreen {
public:

	struct TextField {
		TextField(const std::string &text, float x, float y, float sc, uint32_t color) : text(text), pos(x, y), scale(sc), f {&pos.x, &pos.y, &scale, &r, &g, &b, &alpha} {
			r = ((color>>16)&0xff)/255.0;
			g = ((color>>8)&0xff)/255.0;
			b = (color&0xff)/255.0;
			alpha = ((color>>24)&0xff)/255.0;
		}
		std::string text;
		vec2f pos;

		float scale;
		float r;
		float g;
		float b;
		float alpha;

		float& operator[](int i) { return *f[i]; }

		int size() { return 7; }
	private:
		float* f[8];
	};

	void render(uint32_t d) {
		for(auto &f : fields) {
			uint32_t c = ((int)(f->alpha*255)<<24) | ((int)(f->r*255)<<16) | ((int)(f->g*255)<<8) | (int)(f->b*255);
			screen.text(font, f->text, f->pos.x, f->pos.y, c, f->scale);
		}
	}

	void setFont(const Font &font) {
		this->font = font;
	}

	std::shared_ptr<TextField> addField(const std::string &text, float x = 0, float y = 0, float scale = 1.0, uint32_t color = 0xffffffff) {
		fields.push_back(make_shared<TextField>(text, x, y, scale, color));
		return fields.back();
	}
private:

	grappix::Font font;

	std::vector<std::shared_ptr<TextField>> fields;

	//std::unordered_map<std::string, std::shared_ptr<TextField>> fields;

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
				play(songs[which]);
				next();
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

		prevTitleField = mainScreen.addField("", -3200, 64, 10.2, 0x00e0e080);
		prevComposerField = mainScreen.addField("", -3200, 130, 8.2, 0x00e0e080);

		titleField = mainScreen.addField("NO TITLE", tv0.x, tv0.y, 2.0, 0xffe0e080);
		composerField = mainScreen.addField("NO COMPOSER", tv0.x, tv0.y+60, 1.0, 0xffe0e080);

		nextField = mainScreen.addField("next", 440, 320, 0.5, 0xe080c0ff);		
		nextTitleField = mainScreen.addField("NEXT TITLE", 440, 340, 0.8, 0xffe0e080);
		nextComposerField = mainScreen.addField("NEXT COMPOSER", 440, 370, 0.7, 0xffe0e080);


		timeField = mainScreen.addField("00:00", tv0.x, 148, 1.0, 0xff888888);
		lengthField = mainScreen.addField("(00:00)", 200, 148, 1.0, 0xff888888);


		searchField = searchScreen.addField(">", tv0.x, tv0.y, 1.0, 0xff888888);
		for(int i=0; i<20; i++) {
			resultField.push_back(searchScreen.addField("", tv0.x, tv0.y+30+i*22, 0.8, 0xff008000));
		}


	}

	void play(const SongInfo &si) {
		lock_guard<mutex> guard(plMutex);
		playList.push_back(si);
	}

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

	void render(uint32_t delta) {

		auto k = screen.get_key();

		/*if(k >= '1' && k <= '9') {
			// TODO : If more than 9 songs, require 2 presses
			// and also display pressed digits in corner
			mp.seek(k - '1');
			length = mp.getLength();
		} else */ if(k >= 'A' && k<='Z') {
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
				iquery.addLetter(' ');
				//next();
				break;
			case Window::BACKSPACE:
				iquery.removeLast();
				break;
			case Window::F9:
				next();
				break;
			case Window::F12:
				clear();
				break;		
			case Window::UP:
				marked--;
				break;
			case Window::DOWN:
				marked++;
				break;
			case Window::ENTER:
				{
					auto r = iquery.getFull(marked);
					auto parts = split(r, "\t");
					LOGD("######### %s", parts[2]);
					SongInfo si(string("ftp://ftp.modland.com/pub/modules/") + parts[2], parts[0], parts[1]);
					playList.push_back(si);
					//next();
				}
				break;
			}
		}

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
			}
		}

		if(state == PLAY_STARTED) {
			state = PLAYING;
			prevTitleField->text = titleField->text;
			prevComposerField->text = composerField->text;
			titleField->text = currentInfo.title;
			composerField->text = currentInfo.composer;

			SongInfo next("");

			int psz = playList.size();

			if(psz > 0) {
				auto &n = playList.front();
				if(n.title == "") {
					n.title = utils::path_filename(urldecode(n.path, ""));
				}
				next = n;
			}

			if(psz == 0)
				nextField->text = "";
			else if(psz == 1)
				nextField->text = "Next";
			else
				nextField->text = format("Next (%d)", psz);

			nextTitleField->text = next.title;
			nextComposerField->text = next.composer;

			currentTween.finish();

			currentTween = make_tween().from(*prevTitleField, *titleField)
			.from(*prevComposerField, *composerField)
			.from(*titleField, *nextTitleField)
			.from(*composerField, *nextComposerField)
			.from(nextTitleField->pos.x, 800)
			.from(nextComposerField->pos.x, 800).seconds(1.5);

		}

		//vec2i xy = { 100, 100 };

		screen.clear();
		//screen.text(font, title, 90, 64, 0xe080c0ff, 2.2);
		//screen.text(font, composer, 90, 130, 0xe080c0ff, 1.2);

		//screen.rectangle(tv0, tv1-tv0, 0xff444488);

		auto spectrum = mp.getSpectrum();
		for(int i=0; i<(int)mp.spectrumSize(); i++) {
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

		auto p = mp.getPosition();
		//length = mp.getLength();
		timeField->text = format("%02d:%02d", p/60, p%60);
		lengthField->text = format("(%02d:%02d)", length/60, length%60);

		searchField->text = "#" + iquery.getString();
		if(iquery.newResult()) {
			int nh = iquery.numHits();
			if(nh > 20) nh = 20;
			const auto &res = iquery.getResult(0, nh);
			for(int i=0; i<20; i++) {
				if(i < nh) {
					auto parts = split(res[i], "\t");
					resultField[i]->text = format("%s / %s", parts[0], parts[1]);
				} else resultField[i]->text = "";
			}
		}

		//resultField[marked]->scale = sin(counter*2.0*M_PI/100.0)+1)*0.75;
		//counter = (counter+1)%100;

		if(marked != old_marked) {
			
			if(markTween.valid()) {
				markTween.cancel();
				make_tween().to(resultField[old_marked]->scale, 0.8F).to(resultField[old_marked]->g, 0.5f).seconds(1.0);
			}
			markTween = make_tween().sine().repeating().from(resultField[marked]->scale, 1.0F).from(resultField[marked]->g, 1.0f).seconds(1.0);
			old_marked = marked;
		}


		currentScreen->render(delta);

		screen.flip();
	}

private:

	IncrementalQuery query;

	MusicPlayer mp;

	PlayerScreen mainScreen;
	PlayerScreen searchScreen;

	PlayerScreen *currentScreen;

	unique_ptr<TelnetServer> telnet;

	ModlandDatabase modland;
	//string title = "NO TITLE";
	//string composer = "NO COMPOSER";
	shared_ptr<PlayerScreen::TextField> titleField;
	shared_ptr<PlayerScreen::TextField> composerField;
	shared_ptr<PlayerScreen::TextField> nextTitleField;
	shared_ptr<PlayerScreen::TextField> nextComposerField;
	shared_ptr<PlayerScreen::TextField> prevTitleField;
	shared_ptr<PlayerScreen::TextField> prevComposerField;
	shared_ptr<PlayerScreen::TextField> timeField;
	shared_ptr<PlayerScreen::TextField> lengthField;
	shared_ptr<PlayerScreen::TextField> nextField;

	std::vector<shared_ptr<PlayerScreen::TextField>> resultField;
	shared_ptr<PlayerScreen::TextField> searchField;

	vec2i tv0 = { 80, 54 };
	vec2i tv1 = { 636, 520 };

	int marked = 0;
	int old_marked = -1;
	TweenHolder markTween;

	int length = 0;
	atomic<int> pos;
	// shared_ptr<ChipPlayer> player;
	std::mutex plMutex;
	std::deque<SongInfo> playList;
	LuaInterpreter lua;

	WebGetter webgetter { "_files" };

	Font font;

	TweenHolder currentTween;

	enum State {
		STOPPED,
		WAITING,
		STARTED,
		PLAY_STARTED,
		PLAYING
	};

	State state = STOPPED;

	SongInfo currentInfo;

	std::vector<uint8_t> eq;

	IncrementalQuery iquery;

};

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	static ChipMachine app;

	if(argc >= 2) {
		for(int i=1; i<argc; i++)
			app.play(SongInfo(argv[i]));
		app.next();
	}

	screen.render_loop([](uint32_t delta) {
		app.render(delta);
	}, 20);
	return 0;	
}