
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"

#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>


using namespace tween;
using namespace grappix;
using namespace utils;

namespace chipmachine {

class MainScreen {

public:

	MainScreen(MusicPlayerList &mpl) : player(mpl) {
		auto font = Font("data/Neutra.otf", 32, 256 | Font::DISTANCE_MAP);
		mainScreen.setFont(font);

		prevInfoField = SongInfoField(mainScreen, -3200, 64, 10.0, 0x00e0e080);
		currentInfoField = SongInfoField(mainScreen, tv0.x, tv0.y, 2.0, 0xffe0e080);
		nextInfoField = SongInfoField(mainScreen, 440, 340, 1.0, 0xffe0e080);
		outsideInfoField = SongInfoField(800, 340, 1.0, 0xffe0e080);

		nextField = mainScreen.addField("next", 440, 320, 0.6, 0xe080c0ff);		

		timeField = mainScreen.addField("", tv0.x, 188, 1.0, 0xff888888);
		lengthField = mainScreen.addField("(00:00)", tv0.x + 100, 188, 1.0, 0xff888888);
		songField = mainScreen.addField("[00/00]", tv0.x + 220, 188, 1.0, 0xff888888);
	}

	void update() {

		auto state = player.update();
		if(state == MusicPlayerList::PLAY_STARTED) {
			LOGD("MUSIC STARTING");
			//state = PLAYING;
			currentInfo = player.getInfo();
			prevInfoField.setInfo(currentInfoField.getInfo());
			currentInfoField.setInfo(currentInfo);
			currentTune = currentInfo.starttune;
			currentTween.finish();
			auto sc = currentInfoField[0].scale;

			int tw = mainScreen.getFont().get_width(currentInfo.title, sc);
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
						n.title = path_filename(urldecode(n.path, ""));
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


	}

	void set_variable(const std::string &name, int index, const std::string &val) {
		static unordered_map<string, TextScreen::TextField*> fields = {
			{ "main_title", &currentInfoField[0] },
			{ "main_composer", &currentInfoField[1] },
			{ "main_format", &currentInfoField[2] },
			{ "next_title", &nextInfoField[0] },
			{ "next_composer", &nextInfoField[1] },
			{ "next_format", &nextInfoField[2] },
			{ "prev_title", &prevInfoField[0] },
			{ "prev_composer", &prevInfoField[1] },
			{ "prev_format", &prevInfoField[2] },
			{ "length_field", lengthField.get() },
			{ "time_field", timeField.get() },
			{ "song_field", songField.get() },
			{ "next_field", nextField.get() },
		};

		if(fields.count(name) > 0) {
			auto &f = (*fields[name]);
			if(index == 4) {
				f.color = Color(stoll(val));
			} else
				f[index-1] = stod(val);
		} else
		if(name == "top_left") {
			//currentInfoField.fields[0].color = stol(val);
			tv0[index-1] = stol(val);
		} else
		if(name == "down_right") {
			//currentInfoField.fields[0].color = stol(val);
			tv1[index-1] = stol(val);
		} else
		if(name == "font") {
			if(File::exists(val)) {
				auto font = Font(val, 32, 256 | Font::DISTANCE_MAP);
				mainScreen.setFont(font);
			}
		}
	}

	void render(uint32_t delta) {
		mainScreen.render(delta);
		mainScreen.getFont().update_cache();
	}

	void on_key(grappix::Window::key k) {
		switch(k) {
		case Window::F5:
			player.pause(!player.isPaused());
			if(player.isPaused()) {
				make_tween().sine().repeating().to(timeField->add, 1.0);
			} else
				make_tween().to(timeField->add, 0.0);
			break;
		case Window::F9:
			player.nextSong();
			break;
		case Window::F12:
			player.clearSongs();
			break;		
		case Window::LEFT:
			if(currentTune > 0) {
				player.seek(--currentTune);
				songField->add = 0.0;
				make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				currentInfo = player.getInfo();
				currentInfoField.setInfo(currentInfo);
			}
			break;
		case Window::RIGHT:
			if(currentTune < currentInfo.numtunes-1) {
				player.seek(++currentTune);
				songField->add = 0.0;
				make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				currentInfo = player.getInfo();
				currentInfoField.setInfo(currentInfo);
			}
			break;
		}

	}
private:

	MusicPlayerList &player;

	TextScreen mainScreen;

	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;

	std::shared_ptr<TextScreen::TextField> timeField;
	std::shared_ptr<TextScreen::TextField> lengthField;
	std::shared_ptr<TextScreen::TextField> songField;
	std::shared_ptr<TextScreen::TextField> nextField;

	string currentNextPath;
	SongInfo currentInfo;
	int currentTune = 0;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	tween::TweenHolder currentTween;
};

}