
#include "database.h"

#include <musicplayer/chipplayer.h>

#include "MusicPlayer.h"

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <lua/luainterpreter.h>
#include <webutils/webgetter.h>

#include <fft/spectrum.h>

#include <musicplayer/plugins/ModPlugin/ModPlugin.h>
#include <musicplayer/plugins/VicePlugin/VicePlugin.h>
#include <musicplayer/plugins/SexyPSFPlugin/SexyPSFPlugin.h>
#include <musicplayer/plugins/GMEPlugin/GMEPlugin.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <musicplayer/plugins/UADEPlugin/UADEPlugin.h>

#include <grappix/grappix.h>

#include <coreutils/utils.h>
//#include <audioplayer/audioplayer.h>
#include <cstdio>
#include <vector>
#include <string>
#include <deque>
#include <atomic>

#ifdef RASPBERRYPI
#define AUDIO_DELAY 12
#else
#define AUDIO_DELAY 20
#endif

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace grappix;
using namespace bbs;

class ChipMachine {
public:
	ChipMachine() : mp(fft), eq(SpectrumAnalyzer::eq_slots)  {

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
					LOGD("%s %s", composer, title);
					console->write(format("%s - %s\n", composer, title));
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
				} else if (cmd[0] == "next") {
					next();
				}
			}
		});
		telnet->runThread();

		font = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);

		memset(&eq[0], 2, eq.size());

		/* AudioPlayer::play([=](int16_t *ptr, int size) mutable {
			lock_guard<mutex> guard(plMutex);
			if(player) {				
				int rc = player->getSamples(ptr, size);
				if(rc < 0)
					player = nullptr;
				else pos += size/2;
			}
		}); */
/*
		if(player) {
			title = player->getMeta("title");
			if(title == "")
				title = path_basename(argv[1]);
			composer = player->getMeta("composer");
		}
		LOGD("TITLE:%s", title);
		float zoom1 = 1.0;
		float zoom2 = 1.0; */
	}

	void play(const SongInfo &si) {
		lock_guard<mutex> guard(plMutex);
		playList.push_back(si);
	}

	void next() {
		lock_guard<mutex> guard(plMutex);
		//player = nullptr;
		mp.stop();
	}

	void render(uint32_t delta) {

		if(!mp.playing()/*!player*/ && playList.size() > 0) {
			lock_guard<mutex> guard(plMutex);
			LOGD("New song");
			auto si = playList.front();
			playList.pop_front();
			pos = 0;
			title = si.title;
			composer = si.composer;
			//mp.playFile(si.path);
			auto proto = split(si.path, ":");
			//if(proto.size() > 0)
			//	LOGD("PROTO %s", proto[0]);
			if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
				webgetter.getURL(si.path, [=](const WebGetter::Job &job) {
					mp.playFile(job.getFile());
				});
			} else {
				mp.playFile(si.path);
			}
			/*player = mp.fromFile(si.path);
			if(player) {
				title = player->getMeta("title");
				composer = player->getMeta("composer");
				length = player->getMetaInt("length");
			}*/
		}

		vec2i xy = { 100, 100 };

		screen.clear();
		screen.text(font, title, 120, 50, 0xe080c0ff, 2.2);
		screen.text(font, composer, 120, 122, 0xe080c0ff, 1.2);
		//auto p = pos/44100;
		auto p = mp.getPosition();
		length = mp.getLength();

		screen.text(font, format("%02d:%02d / %02d:%02d", p/60, p%60, length/60, length%60), 120, 280, 0x888888ff, 1.0);

		if(fft.size() > AUDIO_DELAY) {
			auto levels = fft.getLevels();
			for(int i=0; i<(int)levels.size(); i++) {
				if(levels[i] > 5) {
					float f = log(levels[i]) * 20;
					if(f > eq[i])
						eq[i] = f;
				}
			}

			fft.popLevels();
		}

		for(int i=0; i<(int)eq.size(); i++) {
			screen.rectangle(140 + 20*i, 500-eq[i], 18, eq[i], 0xffffffff);
			if(eq[i] >= 4)
				eq[i]-=2;
			else
				eq[i] = 2;
		}


		screen.flip();
	}

private:

	SpectrumAnalyzer fft;

	MusicPlayer mp;

	unique_ptr<TelnetServer> telnet;

	ModlandDatabase modland;
	string title = "NO TITLE";
	string composer = "NO COMPOSER";
	int length = 0;
	atomic<int> pos;
	// shared_ptr<ChipPlayer> player;
	std::mutex plMutex;
	std::deque<SongInfo> playList;
	LuaInterpreter lua;

	WebGetter webgetter { "_files" };

	Font font;

	std::vector<uint8_t> eq;

};

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	static ChipMachine app;

	if(argc >= 2)
		app.play(SongInfo(argv[1]));

	screen.render_loop([](uint32_t delta) {
		app.render(delta);
	}, 20);
	return 0;	
}