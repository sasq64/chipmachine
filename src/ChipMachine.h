#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextField.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"
#include "PlayTracker.h"

#include "state_machine.h"
#include "renderset.h"

#include "../demofx/StarField.h"
#include "../demofx/Scroller.h"

#include <coreutils/utils.h>
#include <tween/tween.h>
#include <grappix/grappix.h>
#include <grappix/gui/list.h>
#include <lua/luainterpreter.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace chipmachine {

class LineEdit : public TextField {
public:
	LineEdit() : TextField() {
		cursorColor = grappix::Color::WHITE;
	}

	LineEdit(const grappix::Font &font, const std::string &text, float x, float y, float sc, uint32_t col) : TextField(font, text, x, y, sc, col) {
		cursorColor = grappix::Color::WHITE;//grappix::Color(col)/2.0F;

		//tween::make_tween().sine().repeating().to(cursorColor, grappix::Color::WHITE).seconds(1.7);
	}

	void on_key(uint32_t key) {
		if(key < 0x100)
			text = text + (char)key;
		else {
			switch(key) {
			case grappix::Window::BACKSPACE:
				text = text.substr(0, text.length()-1);				
				break;
			}
		}
		tsize = font.get_size(text, scale);
	}

	virtual void setText(const std::string &t) override {
		text = prompt + t;
		tsize.x = -1;
	}

	virtual std::string getText() const override {
		return text.substr(prompt.length());
	}

	void setPrompt(const std::string &p) {
		prompt = p;
	}

	virtual void render(grappix::RenderTarget &target, uint32_t delta) override {
		TextField::render(target, delta);
		getWidth();
		//LOGD("REC %s %d %d", text, tsize.x, tsize.y);
		target.rectangle(pos.x + tsize.x + 2, pos.y + 2, 10, tsize.y - 4, cursorColor);
	}
	grappix::Color cursorColor;
	std::string prompt = ">";
};

class Dialog : public Renderable {
	virtual void render(grappix::RenderTarget &target, uint32_t delta) override {

	}

};

class ChipMachine : public grappix::VerticalList::Renderer {
public:

	virtual void render_item(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) override;

	ChipMachine();

	void initLua();
	void play(const SongInfo &si);
	void update();
	void render(uint32_t delta);
	void toast(const std::string &txt, int type);

	void set_scrolltext(const std::string &txt);

	MusicPlayerList &music_player() { return player; }

private:

	void setVariable(const std::string &name, int index, const std::string &val);

	void show_main();
	void show_search();
	SongInfo get_selected_song();

	void setup_rules();
	void update_keys();

	MusicPlayerList player;

	RenderSet renderSet;
	std::shared_ptr<TextField> toastField;

	const int MAIN_SCREEN = 0;
	const int SEARCH_SCREEN = 1;

	int currentScreen = MAIN_SCREEN;

	std::unique_ptr<TelnetInterface> telnet;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	grappix::Font font;
	grappix::Font listFont;

	double spectrumHeight = 20.0;
	int spectrumWidth = 24;
	utils::vec2i spectrumPos;
	std::vector<uint8_t> eq;

	uint32_t bgcolor = 0;
	bool starsOn = true;

	std::string code;

	LuaInterpreter lua;

	demofx::StarField starEffect;
	demofx::Scroller scrollEffect;

	RenderSet mainScreen;
	//grappix::Font font;

	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;

	std::shared_ptr<TextField> timeField;
	std::shared_ptr<TextField> lengthField;
	std::shared_ptr<TextField> songField;
	std::shared_ptr<TextField> nextField;
	std::shared_ptr<TextField> xinfoField;
	std::shared_ptr<TextField> playlistField;

	string currentNextPath;
	SongInfo currentInfo;
	unsigned currentTune = 0;

	tween::TweenHolder currentTween;
	bool lockDown = false;
	bool isFavorite = false;
	grappix::Texture favTexture;
	grappix::Rectangle favPos = { 80, 300, 16*8, 16*6 };

	RenderSet searchScreen;

	std::shared_ptr<LineEdit> searchField;
	std::shared_ptr<LineEdit> commandField;
	std::shared_ptr<TextField> topStatus;

	std::shared_ptr<TextField>resultFieldTemplate;


	int numLines = 20;

	tween::TweenHolder markTween;

	grappix::Color timeColor;

	grappix::Color spectrumColor = 0xffffffff;
	grappix::Color spectrumColorMain = 0xff00aaee;
	grappix::Color spectrumColorSearch = 0xff111155;

	grappix::Color markColor = 0xff00ff00;
	grappix::Color hilightColor = 0xffffffff;

	//grappix::Color searchColor = 0xffffffff;
	//grappix::Color formatColor = 0xffcccc66;

	IncrementalQuery iquery;

	grappix::VerticalList songList;

	bool switchedToMain = false;

	std::vector<std::string> playlists;

	bool onSearchScreen;
	bool onMainScreen;
	bool haveSearchChars;

	statemachine::StateMachine smac;

	std::string currentPlaylistName = "Favorites";
	std::string editPlaylistName;

	bool playlistEdit = false;
};

}


#endif // CHIP_MACHINE_H
