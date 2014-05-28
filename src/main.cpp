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

//static std::unordered_set<std::string> tracker_formats = { "Protracker", "Fasttracker 2", "Screamtracker 3", "Screamtracker 2", "Impulsetracker" };
//static std::unordered_set<std::string> game_formats = { "Super Nintendo Sound Format", "Gameboy Sound System", "Playstation Sound Format" };
//static std::unordered_set<std::string> sid_formats = { "PlaySID", "RealSID"};
//static std::unordered_set<std::string> amiga_formats = { "Delitracker Custom", "TFMX", "AHX", "Future Composer 1.3", "Future Composer 1.4", "Future Composer BSI" };
//static std::unordered_set<std::string> atari_formats = { "SC68", "SNDH" };

namespace chipmachine {

class ChipMachine {
public:
	ChipMachine() : currentScreen(&mainScreen), eq(SpectrumAnalyzer::eq_slots)  {

		iquery = modland.createQuery();

		modland.init();
		telnet = make_unique<TelnetInterface>(modland, player);
		telnet->start();
		memset(&eq[0], 2, eq.size());

		auto font = Font("data/Neutra.otf", 32, 256 | Font::DISTANCE_MAP);

		mainScreen.setFont(font);
		searchScreen.setFont(font);

		prevInfoField = SongInfoField(mainScreen, -3200, 64, 10.0, 0x00e0e080);
		currentInfoField = SongInfoField(mainScreen, tv0.x, tv0.y, 2.0, 0xffe0e080);
		nextInfoField = SongInfoField(mainScreen, 440, 340, 1.0, 0xffe0e080);
		outsideInfoField = SongInfoField(800, 340, 1.0, 0xffe0e080);

		nextField = mainScreen.addField("next", 440, 320, 0.6, 0xe080c0ff);		

		timeField = mainScreen.addField("", tv0.x, 188, 1.0, 0xff888888);
		lengthField = mainScreen.addField("(00:00)", tv0.x + 100, 188, 1.0, 0xff888888);
		songField = mainScreen.addField("[00/00]", tv0.x + 220, 188, 1.0, 0xff888888);

		searchField = searchScreen.addField("#", tv0.x, tv0.y, 1.0, 0xff888888);
		for(int i=0; i<20; i++) {
			resultField.push_back(searchScreen.addField("", tv0.x, tv0.y+30+i*22, 0.8, 0xff008000));
		}


	}

	void play(const SongInfo &si) {
		player.addSong(si);
	}

	void render(uint32_t delta) {

		auto show_main = [=]() {
			currentScreen = &mainScreen;
			make_tween().to(spectrumColor, Color<float>(0xff00aaee)).seconds(1.0);
		};

		auto show_search = [=]() {
			currentScreen = &searchScreen;
			make_tween().to(spectrumColor, Color<float>(0xff111155)).seconds(1.0);
		};

		auto k = screen.get_key();

		bool searchUpdated = false;
		int omark = marked;

		if(k >= '1' && k <= '9') {
			// TODO : If more than 9 songs, require 2 presses
			// and also display pressed digits in corner
			show_search();
			iquery.addLetter(tolower(k));
			searchUpdated = true;
			//mp.seek(k - '1');
			//length = mp.getLength();
		} else if(k >= 'A' && k<='Z') {
			show_search();
			iquery.addLetter(tolower(k));
			searchUpdated = true;
		} else {
			switch(k) {
			case Window::F1:
				show_main();
				break;
			case Window::F2:
				show_search();
				break;
			case Window::SPACE:
				show_search();
				iquery.addLetter(' ');
				searchUpdated = true;
				//next();
				break;
			case Window::BACKSPACE:
				show_search();
				iquery.removeLast();
				searchUpdated = true;
				break;
			case Window::F10:
			case Window::ESCAPE:
				show_search();
				iquery.clear();
				searchUpdated = true;
				break;
			case Window::F9:
				show_main();
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
					songField->add = 0.0;
					make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				}
				break;
			case Window::RIGHT:
				if(currentTune < currentInfo.numtunes) {
					player.seek(++currentTune);
					songField->add = 0.0;
					make_tween().sine().to(songField->add, 1.0).seconds(0.5);
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
						show_main();
					} else {
						player.addSong(si); 
						marked++;
					}
				}
				break;
			}
		}
		auto state = player.update();
		if(state == MusicPlayerList::PLAY_STARTED) {
			//state = PLAYING;
			currentInfo = player.getInfo();
			prevInfoField.setInfo(currentInfoField.getInfo());
			currentInfoField.setInfo(currentInfo);
			currentTune = currentInfo.starttune;

			int tw = mainScreen.getFont().get_width(currentInfo.title, 2.0);

			currentTween.finish();
			currentTween = make_tween().from(prevInfoField, currentInfoField).
			from(currentInfoField, nextInfoField).
			from(nextInfoField, outsideInfoField).seconds(1.5).on_complete([=]() {
				auto d = (tw-(tv1.x-tv0.x-20));
				if(d > 20)
					make_tween().sine().repeating().to(currentInfoField[0].pos.x, currentInfoField[0].pos.x - d).seconds((d+200.0)/200.0);
			});
		}


		auto psz = player.listSize();
		if(psz > 0) {
			if((state != MusicPlayerList::LOADING) && (state != MusicPlayerList::STARTED)) {
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
			}
		} else if(nextField->text != "") {
			nextInfoField.setInfo(SongInfo());
			nextField->text = "";
		}

		screen.clear();

		//screen.rectangle(tv0, tv1-tv0, 0xff444488);
		if(player.playing()) {
			auto spectrum = player.getSpectrum();
			for(int i=0; i<(int)player.spectrumSize(); i++) {
				if(spectrum[i] > 5) {
					float f = log(spectrum[i]) * 20;
					if(f > eq[i])
						eq[i] = f;
				}
			}
		}

		for(int i=0; i<(int)eq.size(); i++) {
			screen.rectangle(tv0.x-10 + 24*i, tv1.y+50-eq[i], 23, eq[i], spectrumColor);
			if(eq[i] >= 4)
				eq[i]-=2;
			else
				eq[i] = 2;
		}

		auto p = player.getPosition();
		int length = player.getLength();
		timeField->text = format("%02d:%02d", p/60, p%60);
		if(length > 0)
			lengthField->text = format("(%02d:%02d)", length/60, length%60);
		else
			lengthField->text = "";

		if(currentInfo.numtunes > 0)
			songField->text = format("[%02d/%02d]", currentTune+1, currentInfo.numtunes);
		else
			songField->text = "";

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

		if(searchUpdated) {
			searchField->text = "#" + iquery.getString();
			searchField->setColor(0xffffffff);
		}
		if(iquery.newResult() || scrollpos != oldscrollpos) {

			//if(nh > 20) nh = 20;
			auto count = 20;
			string fmt = "";
			if(nh > 0) {
				if(scrollpos + count >= nh) count = nh - scrollpos;
				const auto &res = iquery.getResult(scrollpos, count);
				for(int i=0; i<20; i++) {
					if(i < count) {
						auto parts = split(res[i], "\t");
						resultField[i]->text = format("%s / %s", parts[0], parts[1]);
					} else
						resultField[i]->text = "";
				}
			} else {
				for(int i=0; i<20; i++)
					resultField[i]->text = "";
			}
		}

		if(omark != marked && iquery.numHits() > 0) {
			auto p = iquery.getFull(marked);
			auto parts = split(p, "\t");
			auto ext = path_extension(parts[0]);
			searchField->text = format("Format: %s (%s)", parts[3], ext);
			searchField->setColor(0xffcccc66);
		}


		auto marked_field = marked-scrollpos;

		if(marked_field != old_marked) {

			LOGD("MARKED %d SCROLLPOS %d", marked, scrollpos);
			
			if(markTween.valid()) {
				markTween.cancel();
				make_tween().to(resultField[old_marked]->add, 0.0f).seconds(1.0);
			}

			resultField[marked_field]->add = 0.0;
			markTween = make_tween().sine().repeating().from(resultField[marked_field]->add, 1.0f).seconds(1.0);
			old_marked = marked_field;
		}

		currentScreen->render(delta);

		screen.flip();

		mainScreen.getFont().update_cache();
		searchScreen.getFont().update_cache();

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
	shared_ptr<PlayerScreen::TextField> songField;
	shared_ptr<PlayerScreen::TextField> nextField;

	std::vector<shared_ptr<PlayerScreen::TextField>> resultField;
	shared_ptr<PlayerScreen::TextField> searchField;

	vec2i tv0 = { 80, 54 };
	vec2i tv1 = { 636, 520 };

	Color<float> spectrumColor { 0xffffffff };

	int marked = 0;
	int old_marked = -1;
	int scrollpos = 0;
	//int marked_field = 0;
	TweenHolder markTween;
	string currentNextPath;
	SongInfo currentInfo;
	int currentTune = 0;

	//Font font;

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

