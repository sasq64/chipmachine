
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

class SearchScreen {

public:

	SearchScreen(MusicPlayerList &mpl) : player(mpl) {
		modland.init();

		iquery = modland.createQuery();

		auto font = Font("data/Neutra.otf", 32, 256 | Font::DISTANCE_MAP);
		searchScreen.setFont(font);

		resultFieldTemplate = make_shared<TextScreen::TextField>("", tv0.x, tv0.y+30, 0.8, 0xff008000);
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
				searchScreen.setFont(font);
			}
		} else
		if(name == "result_lines") {
			numLines = stol(val);
			for(int i=0; i<resultField.size(); i++) {
				searchScreen.removeField(resultField[i]);
			}
			resultField.clear();
		}
	}

	void render(uint32_t delta) {
		searchScreen.render(delta);
		searchScreen.getFont().update_cache();
	}

	void on_key(grappix::Window::key k) {

		if(resultField.size() == 0) {
			const auto &rft = resultFieldTemplate;
			for(int i=0; i<numLines; i++) {
				resultField.push_back(searchScreen.addField("", rft->pos.x, rft->pos.y + i*28*rft->scale, rft->scale, rft->color));
			}
		}

		bool searchUpdated = false;
		int omark = marked;

		if(k >= '0' && k <= '9') {
			iquery.addLetter(tolower(k));
			searchUpdated = true;
		} else if(k >= 'A' && k<='Z') {
			iquery.addLetter(tolower(k));
			searchUpdated = true;
		} else {
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
			case Window::UP:
				marked--;
				break;
			case Window::DOWN:
				marked++;
				break;
			case Window::PAGEUP:
				marked -= numLines;
				break;
			case Window::PAGEDOWN:
				marked += numLines;
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
						//show_main();
					} else {
						player.addSong(si); 
						marked++;
					}
				}
				break;
			}
		}

		auto oldscrollpos = scrollpos;
		int nh = iquery.numHits();

		if(marked < 0) marked = 0;
		if(marked >= nh)
			marked = nh-1;

		if(marked < 0) marked = 0;

		if(marked < scrollpos)
			scrollpos = marked;
		if(marked >= scrollpos + numLines)
			scrollpos = marked-numLines+1;

		if(searchUpdated) {
			searchField->text = "#" + iquery.getString();
			//searchField->setColor(0xffffffff);
			searchField->color = Color(0xffffffff);
		}
		if(iquery.newResult() || scrollpos != oldscrollpos) {

			//if(nh > numLines) nh = numLines;
			auto count = numLines;
			string fmt = "";
			if(nh > 0) {
				if(scrollpos + count >= nh) count = nh - scrollpos;
				const auto &res = iquery.getResult(scrollpos, count);
				for(int i=0; i<numLines; i++) {
					if(i < count) {
						auto parts = split(res[i], "\t");
						resultField[i]->text = format("%s / %s", parts[0], parts[1]);
					} else
						resultField[i]->text = "";
				}
			} else {
				for(int i=0; i<numLines; i++)
					resultField[i]->text = "";
			}
		}

		if(omark != marked && iquery.numHits() > 0) {
			auto p = iquery.getFull(marked);
			auto parts = split(p, "\t");
			auto ext = path_extension(parts[0]);
			searchField->text = format("Format: %s (%s)", parts[3], ext);
			//searchField->setColor(0xffcccc66);
			searchField->color = Color(0xffcccc66);
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
	}


private:
	MusicPlayerList &player;

	TextScreen searchScreen;

	ModlandDatabase modland;

	std::vector<std::shared_ptr<TextScreen::TextField>> resultField;
	std::shared_ptr<TextScreen::TextField> searchField;
	std::shared_ptr<TextScreen::TextField>resultFieldTemplate;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	int numLines = 20;

	int marked = 0;
	int old_marked = -1;
	int scrollpos = 0;
	tween::TweenHolder markTween;

	IncrementalQuery iquery;

};

}