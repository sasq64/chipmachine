#include "MusicDatabase.h"

#include "MusicPlayerList.h"
#include "PlayerScreen.h"
#include "TelnetInterface.h"

#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

class ChipMachine {
public:
	ChipMachine() : currentScreen(&mainScreen), eq(SpectrumAnalyzer::eq_slots)  {

		iquery = modland.createQuery();

		modland.init();
		telnet = make_unique<TelnetInterface>(modland, player);
		telnet->start();
		memset(&eq[0], 2, eq.size());

		auto mfont = Font("data/ObelixPro.ttf", 32, 256 | Font::DISTANCE_MAP);

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
			case Window::F10:
				currentScreen = &searchScreen;
				iquery.clear();
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
			case Window::LEFT:
				if(currentTune > 0) {
					player.seek(--currentTune);
				}
				break;
			case Window::RIGHT:
				if(currentTune < currentInfo.numtunes) {
					player.seek(++currentTune);
				}
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
		if(player.update() == MusicPlayerList::PLAY_STARTED) {
			//state = PLAYING;
			currentInfo = player.getInfo();
			prevInfoField.setInfo(currentInfoField.getInfo());
			currentInfoField.setInfo(currentInfo);

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
		} else if(nextField->text != "") {
			nextInfoField.setInfo(SongInfo());
			nextField->text = "";
		}

		screen.clear();

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
		lengthField->text = format("(%02d:%02d) [%02d/%02d]", length/60, length%60, currentTune+1, currentInfo.numtunes);

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
				LOGD("HITS %d COUNT %d, RESSIZE %d", nh, count, res.size());
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

		auto marked_field = marked-scrollpos;

		if(marked_field != old_marked) {

			LOGD("MARKED %d SCROLLPOS %d", marked, scrollpos);
			
			if(markTween.valid()) {
				markTween.cancel();
				make_tween().to(resultField[old_marked]->g, 0.5f).seconds(1.0);
			}
			//if(iquery.numHits() > 0) {
				//const auto &res = iquery.getFull(scrollpos+marked);
				//LOGD("%s", res);
			//}
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

	MusicPlayerList player;

	PlayerScreen mainScreen;
	PlayerScreen searchScreen;

	PlayerScreen *currentScreen;

	unique_ptr<TelnetInterface> telnet;

	ModlandDatabase modland;

	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;

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
	//int marked_field = 0;
	TweenHolder markTween;
	string currentNextPath;
	SongInfo currentInfo;
	int currentTune = 0;

	Font font;

	TweenHolder currentTween;
	std::vector<uint8_t> eq;

	IncrementalQuery iquery;

};

} // namespace chipmachine

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	static chipmachine::ChipMachine app;

	if(argc >= 2) {
		for(int i=1; i<argc; i++)
			app.play(SongInfo(argv[i]));
	}

	screen.render_loop([](uint32_t delta) {
		app.render(delta);
	}, 20);
	return 0;	
}

