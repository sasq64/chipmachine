
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"
#include "SongList.h"

#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

#include <grappix/gui/list.h>

using namespace tween;
using namespace grappix;
using namespace utils;

namespace chipmachine {

class SearchScreen : public SongList::Renderer {

public:

	virtual void render_item(Rectangle &rec, int y, uint32_t index, bool hilight) override {
		auto res = iquery.getResult(index);
		auto parts = split(res, "\t");
		auto text = format("%s / %s", parts[0], parts[1]);
		auto c = hilight ? markColor : resultFieldTemplate->color;
		grappix::screen.text(font, text, rec.x, rec.y, c, resultFieldTemplate->scale);
	};

	SearchScreen(MusicPlayerList &mpl, MusicDatabase &mdb) : player(mpl), /* mdb(mdb), */ songList(this, Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28), 20) {

		iquery = mdb.createQuery();

		font = Font("data/Neutra.otf", 32, 256 | Font::DISTANCE_MAP);
		searchScreen.setFont(font);

		resultFieldTemplate = make_shared<TextScreen::TextField>("", tv0.x, tv0.y+30, 0.8, 0xff20c020);
		markColor = resultFieldTemplate->color;
		hilightColor = Color(0xffffffff);
		markTween = make_tween().sine().repeating().from(markColor, hilightColor).seconds(1.0);

		searchField = searchScreen.addField("#", tv0.x, tv0.y, 1.0, 0xff888888);

	}

	void update() {
	}

	void set_variable(const std::string &name, int index, const std::string &val) {
		static unordered_map<string, TextScreen::TextField*> fields = {
			{ "search_field", searchField.get() },
			{ "result_field", resultFieldTemplate.get() },
		};

		if(fields.count(name) > 0) {
			auto &f = (*fields[name]);
			if(index >= 4) {
				auto c = Color(stoll(val));
				if(name == "search_field") {
					if(index == 5) {
						formatColor = c;
						return;
					} else
						searchColor = c;
				}
				f.color = c;
				if(name == "result_field") {
					markColor = c;
					markTween = make_tween().sine().repeating().from(markColor, hilightColor).seconds(1.0);
				}
			} else
				f[index-1] = stod(val);

		}  else
		if(name == "hilight_color") {
			hilightColor = Color(stoll(val));
			markTween = make_tween().sine().repeating().from(markColor, hilightColor).seconds(1.0);
		} else
		if(name == "top_left") {
			//currentInfoField.fields[0].color = stol(val);
			tv0[index-1] = stol(val);
			songList.set_area(Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28));
		} else
		if(name == "down_right") {
			//currentInfoField.fields[0].color = stol(val);
			tv1[index-1] = stol(val);
			songList.set_area(Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28));
		} else
		if(name == "font") {
			if(File::exists(val)) {
				font = Font(val, 32, 256);// | Font::DISTANCE_MAP);
				searchScreen.setFont(font);
			}
		} else
		if(name == "result_lines") {
			numLines = stol(val);
			songList.set_visible(numLines);
		}
	}

	void render(uint32_t delta) {
		songList.render();
		searchScreen.render(delta);
		searchScreen.getFont().update_cache();
	}

	void on_key(grappix::Window::key k) {

		bool searchUpdated = false;
		auto last_selection = songList.selected();

		songList.on_key(k);

		if((k >= '0' && k <= '9') || k == '/') {
			if(switchedToMain)
				iquery.clear();
			switchedToMain = false;
			iquery.addLetter(tolower(k));
			searchUpdated = true;
		} else if(k >= 'A' && k<='Z') {
			if(switchedToMain)
				iquery.clear();
			switchedToMain = false;
			iquery.addLetter(tolower(k));
			searchUpdated = true;
		} else {
			switchedToMain = false;
			switch(k) {
			case Window::SPACE:
				iquery.addLetter(' ');
				searchUpdated = true;
				break;
			case Window::BACKSPACE:
				iquery.removeLast();
				searchUpdated = true;
				break;
			case Window::F10:
			case Window::ESCAPE:
				iquery.clear();
				searchUpdated = true;
				break;
/*			case Window::UP:
				songList.select(songList.selected()-1);
				break;
			case Window::DOWN:
				songList.select(songList.selected()+1);
				break;
			case Window::PAGEUP:
				songList.pageup();
				break;
			case Window::PAGEDOWN:
				songList.pagedown();
				break; */
			case Window::DELETE:
				{
					auto r = iquery.getFull(songList.selected());
					auto parts = split(r, "\t");
					LOGD("######### %s", parts[0]);
					SongInfo si(parts[0], parts[1], parts[2], parts[3]);
					if(player.addSong(si)) 
						songList.select(songList.selected()+1);
				}
				break;
			case Window::ENTER:
				{
					auto r = iquery.getFull(songList.selected());
					auto parts = split(r, "\t");
					LOGD("######### %s", parts[0]);
					SongInfo si(parts[0], parts[1], parts[2], parts[3]);
					if(!(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_RIGHT))) {
						//player.clearSongs();
						//player.addSong(si, 0);
						//player.nextSong();
						player.playSong(si);
						switchedToMain = true;
						//show_main();
						//iquery.clear();
					} else {
					if(player.addSong(si)) 
						songList.select(songList.selected()+1);
						//marked++;
					}
				}
				break;
			}
		}

		if(searchUpdated) {
			searchField->setText("#" + iquery.getString());
			//searchField->setColor(0xffffffff);
			searchField->color = searchColor;
		}
		if(iquery.newResult())
			songList.set_total(iquery.numHits());

		if(songList.selected() != last_selection && iquery.numHits() > 0) {
			auto p = iquery.getFull(songList.selected());
			auto parts = split(p, "\t");
			auto ext = path_extension(parts[0]);
			searchField->setText(format("Format: %s (%s)", parts[3], ext));
			//searchField->setColor(0xffcccc66);
			searchField->color = Color(formatColor);
		}
	}
private:
	MusicPlayerList &player;

	TextScreen searchScreen;

	//MusicDatabase &mdb;

	//std::vector<std::shared_ptr<TextScreen::TextField>> resultField;
	std::shared_ptr<TextScreen::TextField> searchField;
	std::shared_ptr<TextScreen::TextField>resultFieldTemplate;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	int numLines = 20;

	tween::TweenHolder markTween;

	Color markColor;
	Color hilightColor;

	Color searchColor = Color(0xffffffff);
	Color formatColor = Color(0xffcccc66);

	IncrementalQuery iquery;

	SongList songList;
	Font font;

	bool switchedToMain = false;

};

}