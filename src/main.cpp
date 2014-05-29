#include "MusicDatabase.h"

#include "MusicPlayerList.h"
#include "PlayerScreen.h"
#include "TelnetInterface.h"

#include <tween/tween.h>
#include <grappix/grappix.h>
#include <lua/luainterpreter.h>

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

static const string starShaderF = R"(
#ifdef GL_ES
	precision mediump float;
#endif
	uniform sampler2D sTexture;
	uniform float scrollpos; // 0 -> 1

	const vec4 color0 = vec4(0.0, 1.0, 0.0, 1.0);
	const vec4 color1 = vec4(1.0, 0.3, 0.3, 1.0);

	varying vec2 UV;

	void main() {
		float m = mod(gl_FragCoord.y, 3);
		float uvx = mod(UV.x + scrollpos * m, 1.0);
		gl_FragColor = m * texture2D(sTexture, vec2(uvx, UV.y));
	}
)";


class ChipMachine {
public:
	ChipMachine() : currentScreen(&mainScreen), eq(SpectrumAnalyzer::eq_slots)  {

		iquery = modland.createQuery();

		image::bitmap bm(screen.width(), screen.height());
		bm.clear(0x00000000);
		for(int y=0; y<bm.height(); y++) {
			auto x = rand() % bm.width();
			bm[y*bm.width()+x] = bm[y*bm.width()+x + 1] = 0xff888888;
			bm[y*bm.width()+x + 2] = 0xff666666;
		}
		starTexture = Texture(bm);

		starProgram = get_program(TEXTURED_PROGRAM).clone();
		starProgram.setFragmentSource(starShaderF);
		//starProgram.createProgram();

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

		spectrumPos = { tv0.x-10, tv1.y+50 };

		resultFieldTemplate = make_shared<PlayerScreen::TextField>("", tv0.x, tv0.y+30, 0.8, 0xff008000);

		searchField = searchScreen.addField("#", tv0.x, tv0.y, 1.0, 0xff888888);

		static unordered_map<string, PlayerScreen::TextField*> fields = {
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
			{ "search_field", searchField.get() },
			{ "result_field", resultFieldTemplate.get() },
		};

		lua.registerFunction<void, string, uint32_t, string>("set_var", [=](string name, uint32_t index, string val) {
			LOGD("%s(%d) = %s", name, index, val);

			if(fields.count(name) > 0) {
				auto &f = (*fields[name]);
				if(index == 4) {
					f.color = Color(stoll(val));
				} else
					f[index-1] = stod(val);
			} else
			if(name == "spectrum") {
				if(index <= 2)
					spectrumPos[index-1] = stol(val);
				else if(index == 3)
					spectrumWidth = stol(val);
				else if(index == 4)
					spectrumHeight = stod(val);
				else if(index == 5)
					spectrumColorMain = Color(stoll(val));
				else
					spectrumColorSearch = Color(stoll(val));
			} else
			if(name == "top_left") {
				//currentInfoField.fields[0].color = stol(val);
				tv0[index-1] = stol(val);
			} else
			if(name == "down_right") {
				//currentInfoField.fields[0].color = stol(val);
				tv0[index-1] = stol(val);
			} else
			if(name == "font") {
				if(File::exists(val)) {
					auto font = Font(val, 32, 256 | Font::DISTANCE_MAP);
					mainScreen.setFont(font);
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
		});

		Resources::getInstance().load_text("lua/init.lua", [=](const std::string &contents) {
			LOGD("init.lua");
			lua.load(R"(
				Settings = {}
			)");
			lua.load(contents);
			//LOGD(contents);
			lua.load(R"(
				for a,b in pairs(Settings) do 
					if type(b) == 'table' then
						for a1,b1 in ipairs(b) do
							set_var(a, a1, b1)
						end
					else
						set_var(a, 0, b)
					end
				end
			)");
		});



	}

	void play(const SongInfo &si) {
		player.addSong(si);
	}

	void render(uint32_t delta) {

		auto show_main = [=]() {
			currentScreen = &mainScreen;
			make_tween().to(spectrumColor, spectrumColorMain).seconds(1.0);
		};

		auto show_search = [=]() {
			currentScreen = &searchScreen;
			make_tween().to(spectrumColor, spectrumColorSearch).seconds(1.0);
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
			case Window::F5:
				player.pause(!player.isPaused());
				if(player.isPaused()) {
					LOGD("TWEEN IT");
					make_tween().sine().repeating().to(timeField->add, 1.0);
				} else
					make_tween().to(timeField->add, 0.0);
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
				if(currentTune < currentInfo.numtunes-1) {
					player.seek(++currentTune);
					songField->add = 0.0;
					make_tween().sine().to(songField->add, 1.0).seconds(0.5);
				}
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
			currentTween.finish();
			auto sc = currentInfoField[0].scale;

			int tw = mainScreen.getFont().get_width(currentInfo.title, sc);
			LOGD("'%s' is %d pixels", currentInfo.title, tw);
			currentTween = make_tween().from(prevInfoField, currentInfoField).
			from(currentInfoField, nextInfoField).
			from(nextInfoField, outsideInfoField).seconds(1.5).on_complete([=]() {
				LOGD("currentInfo is at x=%d", currentInfoField[0].pos.x);
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
					float f = log(spectrum[i]) * spectrumHeight;
					if(f > eq[i])
						eq[i] = f;
				}
			}
		}

		for(int i=0; i<(int)eq.size(); i++) {
			screen.rectangle( spectrumPos.x + (spectrumWidth)*i, spectrumPos.y-eq[i], spectrumWidth-1, eq[i], spectrumColor);
			if(!player.isPaused()) {
				if(eq[i] >= 4)
					eq[i]-=2;
				else
					eq[i] = 2;
			}
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
		if(marked >= scrollpos + numLines)
			scrollpos = marked-numLines+1;



		if(resultField.size() == 0) {
			const auto &rft = resultFieldTemplate;
			for(int i=0; i<numLines; i++) {
				resultField.push_back(searchScreen.addField("", rft->pos.x, rft->pos.y + i*28*rft->scale, rft->scale, rft->color));
			}
		}

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

		currentScreen->render(delta);

		starProgram.use();
		starProgram.setUniform("scrollpos", starPos += (0.000173 * delta));

		screen.draw(starTexture, starProgram);

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
	shared_ptr<PlayerScreen::TextField>resultFieldTemplate;

	vec2i tv0 = { 80, 54 };
	vec2i tv1 = { 636, 520 };

	Color spectrumColor { 0xffffffff };
	Color spectrumColorMain { 0xff00aaee };
	Color spectrumColorSearch { 0xff111155 };
	double spectrumHeight = 20.0;
	int spectrumWidth = 24;
	vec2i spectrumPos;
	int numLines = 20;

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

	LuaInterpreter lua;

	Texture starTexture;
	Program starProgram;
	float starPos = 0.0;
};

} // namespace chipmachine

int main(int argc, char* argv[]) {

	screen.open(720, 576, false);
	//screen.open(true);
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

