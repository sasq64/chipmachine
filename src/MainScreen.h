
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"
#include "PlaylistDatabase.h"

#include <tween/tween.h>
#include <grappix/grappix.h>
#include <coreutils/utils.h>
#include <image/image.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

using namespace tween;
using namespace grappix;
using namespace utils;

namespace chipmachine {

#define Z 0xff444488
const vector<uint32_t> heart = { 0,Z,Z,0,Z,Z,0,0,
                                 Z,Z,Z,Z,Z,Z,Z,0,
                                 Z,Z,Z,Z,Z,Z,Z,0,
                                 0,Z,Z,Z,Z,Z,0,0,
                                 0,0,Z,Z,Z,0,0,0,
                                 0,0,0,Z,0,0,0,0 };
#undef Z

class MainScreen {

public:

	MainScreen(MusicPlayerList &mpl) : player(mpl) {
		font = Font("data/Neutra.otf", 32, 256 | Font::DISTANCE_MAP);
		//mainScreen.setFont(font);

		prevInfoField = SongInfoField(font, -3200, 64, 10.0, 0x00e0e080);
		currentInfoField = SongInfoField(font, tv0.x, tv0.y, 2.0, 0xffe0e080);
		nextInfoField = SongInfoField(font, 440, 340, 1.0, 0xffe0e080);
		outsideInfoField = SongInfoField(font, screen.width()+10, 340, 1.0, 0xffe0e080);
		for(int i=0; i<3; i++)
			mainScreen.add(prevInfoField.fields[i]);
		for(int i=0; i<3; i++)
			mainScreen.add(currentInfoField.fields[i]);
		for(int i=0; i<3; i++)
			mainScreen.add(nextInfoField.fields[i]);
		for(int i=0; i<3; i++)
			mainScreen.add(outsideInfoField.fields[i]);

		xinfoField = make_shared<TextField>(font, "", tv1.x-100, tv0.y, 0.8, 0x50a0c0ff);
		mainScreen.add(xinfoField);

		nextField = make_shared<TextField>(font, "next", 440, 320, 0.6, 0xe080c0ff);
		mainScreen.add(nextField);

		timeField = make_shared<TextField>(font, "", tv0.x, 188, 1.0, 0xff888888);
		mainScreen.add(timeField);
		lengthField = make_shared<TextField>(font, "", tv0.x + 100, 188, 1.0, 0xff888888);
		mainScreen.add(lengthField);
		songField = make_shared<TextField>(font, "", tv0.x + 220, 188, 1.0, 0xff888888);
		mainScreen.add(songField);

		auto bm = image::bitmap(8, 6, &heart[0]);
		favTexture = Texture(bm);
		glBindTexture(GL_TEXTURE_2D, favTexture.id());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	}

	void update() {

		//player.update();
		auto state = player.getState();
		//LOGD("STATE %d vs %d %d %d", state, MusicPlayerList::STOPPED, MusicPlayerList::WAITING, MusicPlayerList::PLAY_STARTED);
		if(state == MusicPlayerList::PLAY_STARTED) {
			LOGD("MUSIC STARTING");
			//state = PLAYING;
			currentInfo = player.getInfo();
			LOGD("Prev song %s, new song %s", currentInfoField.getInfo().title, currentInfo.title);
			prevInfoField.setInfo(currentInfoField.getInfo());
			currentInfoField.setInfo(currentInfo);
			currentTune = currentInfo.starttune;
			currentTween.finish();
			auto sc = currentInfoField[0].scale;

			if(currentInfo.numtunes > 0)
				songField->setText(format("[%02d/%02d]", currentTune+1, currentInfo.numtunes));
			else
				songField->setText("");

			auto sub_title = player.getMeta("sub_title");

			int tw = font.get_width(currentInfo.title, sc);

			LOGD("%s vs %s", nextInfoField.path, currentInfoField.path);

			auto f = [=]() {
				xinfoField->setText(sub_title);
				auto d = (tw-(tv1.x-tv0.x-20));
				if(d > 20)
					make_tween().sine().repeating().to(currentInfoField[0].pos.x, currentInfoField[0].pos.x - d).seconds((d+200.0)/200.0);
			};

			auto favorites = PlaylistDatabase::getInstance().getPlaylist("Favorites");
			auto favsong = find_if(favorites.begin(), favorites.end(), [&](const SongInfo &song) -> bool { return song.path == currentInfo.path; });
			isFavorite = (favsong != favorites.end());

			if(nextInfoField == currentInfoField) {
				currentTween = make_tween().from(prevInfoField, currentInfoField).
				from(currentInfoField, nextInfoField).
				from(nextInfoField, outsideInfoField).seconds(1.5).on_complete(f);
			} else {
				currentTween = make_tween().from(prevInfoField, currentInfoField).
				from(currentInfoField, outsideInfoField).seconds(1.5).on_complete(f);
			}

		}

		if(state == MusicPlayerList::PLAYING || state == MusicPlayerList::STOPPED) {
			auto psz = player.listSize();
			if(psz > 0) {
				auto n = player.getInfo(1);
				if(n.path != currentNextPath) {
					if(n.title == "") {
						n.title = path_filename(urldecode(n.path, ""));
					}

					if(psz == 1)
						nextField->setText("Next");
					else
						nextField->setText(format("Next (%d)", psz));
					nextInfoField.setInfo(n);
					currentNextPath = n.path;
				}
			} else if(nextField->getText() != "") {
				nextInfoField.setInfo(SongInfo());
				nextField->setText("");
			}
		}

		auto tune = player.getTune();
		if(currentTune != tune) {
			songField->add = 0.0;
			make_tween().sine().to(songField->add, 1.0).seconds(0.5);
			currentInfo = player.getInfo();
			auto sub_title = player.getMeta("sub_title");
			xinfoField->setText(sub_title);
			currentInfoField.setInfo(currentInfo);
			currentTune = tune;
			songField->setText(format("[%02d/%02d]", currentTune+1, currentInfo.numtunes));
		}

		if(player.playing()) {

			bool party = (player.getPermissions() & MusicPlayerList::PARTYMODE) != 0;
			if(!lockDown && party) {
				lockDown = true;
				make_tween().to(timeField->color, Color(0xffff0000)).seconds(0.5);
			} else if(lockDown && !party) {
				lockDown = false;
				make_tween().to(timeField->color, timeColor).seconds(2.0);
			}


			auto p = player.getPosition();
			int length = player.getLength();
			timeField->setText(format("%02d:%02d", p/60, p%60));
			if(length > 0)
				lengthField->setText(format("(%02d:%02d)", length/60, length%60));
			else
				lengthField->setText("");

			auto sub_title = player.getMeta("sub_title");
			if(sub_title != xinfoField->getText())
				xinfoField->setText(sub_title);
		}
	}

	void set_variable(const std::string &name, int index, const std::string &val) {
		static unordered_map<string, TextField*> fields = {
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
			{ "xinfo_field", xinfoField.get() },
		};

		if(fields.count(name) > 0) {
			auto &f = (*fields[name]);
			if(index == 4) {
				f.color = Color(stoll(val));
				if(name == "main_title" || name == "next_title")
					outsideInfoField[0].color = f.color;
				else if(name == "main_composer" || name == "next_composer")
					outsideInfoField[1].color = f.color;
				else if(name == "main_format" || name == "next_format")
					outsideInfoField[2].color = f.color;
				else if(name == "time_field")
					timeColor = f.color;
			} else {
				f[index-1] = stod(val);
			}
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
				font = Font(val, 48, 512 | Font::DISTANCE_MAP);
				//mainScreen.setFont(font);
			}
		}
	}

	void render(uint32_t delta) {
		mainScreen.render(screen, delta);
		font.update_cache();
		if(isFavorite)
			screen.draw(favTexture, tv0.x, 300, 16*8, 16*6, nullptr);
	}

	void on_key(grappix::Window::key k) {
		switch(k) {
		case Window::F5:
			player.pause(!player.isPaused());
			if(player.isPaused()) {
				make_tween().sine().repeating().to(timeField->add, 1.0).seconds(0.5);
			} else
				make_tween().to(timeField->add, 0.0).seconds(0.5);
			break;
		case Window::F7:
			if(isFavorite) {
				PlaylistDatabase::getInstance().removeFromPlaylist("Favorites", currentInfo);

			} else {
				PlaylistDatabase::getInstance().addToPlaylist("Favorites", currentInfo);
			}
			isFavorite = !isFavorite;
			break;
		//case Window::F9:
		case Window::ENTER:
			player.nextSong();
			break;
		//case Window::F12:
		//	player.clearSongs();
		//	break;
		case Window::LEFT:
			if(currentTune > 0) {
				player.seek(currentTune - 1);
				songField->add = 0.0;
				make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				currentInfo = player.getInfo();
				auto sub_title = player.getMeta("sub_title");
				xinfoField->setText(sub_title);
				currentInfoField.setInfo(currentInfo);
			}
			break;
		case Window::RIGHT:
			if(currentTune < currentInfo.numtunes-1) {
				player.seek(currentTune + 1);
				songField->add = 0.0;
				make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				currentInfo = player.getInfo();
				auto sub_title = player.getMeta("sub_title");
				xinfoField->setText(sub_title);
				currentInfoField.setInfo(currentInfo);
			}
			break;
		}

	}
private:

	MusicPlayerList &player;

	RenderSet mainScreen;
	grappix::Font font;

	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;

	std::shared_ptr<TextField> timeField;
	std::shared_ptr<TextField> lengthField;
	std::shared_ptr<TextField> songField;
	std::shared_ptr<TextField> nextField;
	std::shared_ptr<TextField> xinfoField;

	string currentNextPath;
	SongInfo currentInfo;
	unsigned currentTune = 0;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	tween::TweenHolder currentTween;
	Color timeColor;
	bool lockDown = false;
	bool isFavorite = false;
	Texture favTexture;
};

}