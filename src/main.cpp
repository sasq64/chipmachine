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

class PlayerScreen {
public:

	struct TextField {
		TextField(const std::string &text, float x, float y, float scale, uint32_t color) : text(text), pos(x, y), scale(scale), color(color&0xffffff), alpha(((color>>24)&0xff)/255.0) {
		}
		std::string text;
		vec2f pos;
		float scale;
		uint32_t color;
		float alpha;

		float& operator[](int i) {
			switch(i) {
			case 0:
				return pos.x;
			case 1:
				return pos.y;
			case 2:
				return scale;
			case 3:
				return alpha;
			}
			return alpha;
		}

		int size() { return 4; }


	};

	void render(uint32_t d) {
		for(auto &f : fields) {
			uint32_t c = f->color | ((int)(f->alpha*255)<<24);
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
	ChipMachine() : eq(SpectrumAnalyzer::eq_slots)  {

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
				}
			}
		});
		telnet->runThread();

		font = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);
		memset(&eq[0], 2, eq.size());

		plScreen.setFont(font);

		prevTitleField = plScreen.addField("", -3200, 64, 10.2, 0x00e0e080);
		prevComposerField = plScreen.addField("", -3200, 130, 8.2, 0x00e0e080);

		titleField = plScreen.addField("NO TITLE", 90, 64, 2.2, 0xffe0e080);
		composerField = plScreen.addField("NO COMPOSER", 90, 130, 1.2, 0xffe0e080);

		nextField = plScreen.addField("next", 400, 320, 0.5, 0xe080c0ff);		
		nextTitleField = plScreen.addField("NEXT TITLE", 400, 340, 0.8, 0xffe0e080);
		nextComposerField = plScreen.addField("NEXT COMPOSER", 400, 370, 0.7, 0xffe0e080);


		timeField = plScreen.addField("00:00", 90, 170, 1.0, 0xff888888);
		lengthField = plScreen.addField("(00:00)", 200, 170, 1.0, 0xff888888);

	}

	void play(const SongInfo &si) {
		lock_guard<mutex> guard(plMutex);
		playList.push_back(si);
	}

	void next() {
		lock_guard<mutex> guard(plMutex);
		//player = nullptr;
		mp.stop();
		state = WAITING;
	}

	void render(uint32_t delta) {

		if(screen.get_key() == Window::SPACE)
			next();

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

			currentTween = tween::make_tween().from(*prevTitleField, *titleField)
			.from(*prevComposerField, *composerField)
			.from(*titleField, *nextTitleField)
			.from(*composerField, *nextComposerField)
			.from(nextTitleField->pos.x, 800)
			.from(nextComposerField->pos.x, 800).seconds(1.0);

		}

		//vec2i xy = { 100, 100 };

		screen.clear();
		//screen.text(font, title, 90, 64, 0xe080c0ff, 2.2);
		//screen.text(font, composer, 90, 130, 0xe080c0ff, 1.2);

		auto spectrum = mp.getSpectrum();
		for(int i=0; i<(int)mp.spectrumSize(); i++) {
			if(spectrum[i] > 5) {
				float f = log(spectrum[i]) * 20;
				if(f > eq[i])
					eq[i] = f;
			}
		}

		for(int i=0; i<(int)eq.size(); i++) {
			screen.rectangle(140 + 20*i, 500-eq[i], 18, eq[i], 0xffffffff);
			if(eq[i] >= 4)
				eq[i]-=2;
			else
				eq[i] = 2;
		}

		auto p = mp.getPosition();
		//length = mp.getLength();
		timeField->text = format("%02d:%02d", p/60, p%60);
		lengthField->text = format("(%02d:%02d)", length/60, length%60);

		plScreen.render(delta);

		screen.flip();
	}

private:

	MusicPlayer mp;

	PlayerScreen plScreen;

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

	int length = 0;
	atomic<int> pos;
	// shared_ptr<ChipPlayer> player;
	std::mutex plMutex;
	std::deque<SongInfo> playList;
	LuaInterpreter lua;

	WebGetter webgetter { "_files" };

	Font font;

	tween::Holder currentTween;

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