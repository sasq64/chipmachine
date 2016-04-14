#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextField.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"
#ifdef USE_REMOTELISTS
#include "RemoteLists.h"
#endif

#include "state_machine.h"
#include <grappix/gui/renderset.h>

#include "LineEdit.h"
#include "Dialog.h"

#include "MusicBars.h"

#include "../demofx/StarField.h"
#include "../demofx/Scroller.h"

#include <coreutils/utils.h>
#include <tween/tween.h>
#include <grappix/grappix.h>
#include <grappix/gui/list.h>
#include <luainterpreter/luainterpreter.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace chipmachine {

class Icon : public Renderable {
public:
	Icon() {}

	Icon(grappix::Texture *tx, float x, float y, float w, float h)
	    : texture(tx), rec(x, y, w, h) {}

	Icon(const image::bitmap &bm, float x, float y, float w, float h) : rec(x, y, w, h) {
		texture = new grappix::Texture(bm);
		glBindTexture(GL_TEXTURE_2D, texture->id());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		if((color >> 24) == 0)
			return;
		target->draw(*texture, rec.x, rec.y, rec.w, rec.h, nullptr, color);
	}
	
	void set(const grappix::Rectangle &r) {
		rec = r;
	}

	grappix::Color color{0xffffffff};
	grappix::Rectangle rec;

private:
	grappix::Texture *texture;
};

class ChipMachine {
public:
	using Color = grappix::Color;

	void renderSong(grappix::Rectangle &rec, int y, uint32_t index, bool hilight);
	void renderCommand(grappix::Rectangle &rec, int y, uint32_t index, bool hilight);

	ChipMachine(const std::string &workDir);
	~ChipMachine();

	void initLua();
	void layoutScreen();
	void play(const SongInfo &si);
	void update();
	void render(uint32_t delta);
	void toast(const std::string &txt, int type);
	void removeToast();

	void setScrolltext(const std::string &txt);
	void shuffleSongs(bool format, bool composer, bool collection, int limit);

	MusicPlayerList &musicPlayer() { return player; }

private:
	enum Screen {
		NO_SCREEN = -1,
		MAIN_SCREEN = 0,
		SEARCH_SCREEN = 1,
		COMMAND_SCREEN = 2,
	};
	
	static const uint32_t SHIFT = 0x10000;
	static const uint32_t CTRL = 0x20000;
	static const uint32_t ALT = 0x40000;
	
	void setVariable(const std::string &name, int index, const std::string &val);

	void showScreen(Screen screen);
	SongInfo getSelectedSong();

	void setupRules();
	void setupCommands();
	void updateKeys();
	void updateFavorite();
	void updateLists() {
		
		int y = resultFieldTemplate.pos.y + 5;
		
		songList.setArea(grappix::Rectangle(topLeft.x, y, grappix::screen.width() - topLeft.x,
		                                    downRight.y - topLeft.y - y));
		commandList.setArea(grappix::Rectangle(topLeft.x, y, grappix::screen.width() - topLeft.x,
		                                    downRight.y - topLeft.y - y));
	}

	const std::vector<std::string> key_names = {
		"UP",
		"DOWN",
		"LEFT",
		"RIGHT",
		"ENTER",
		"ESCAPE",
		"BACKSPACE",
		"TAB",
		"PAGEUP",
		"PAGEDOWN",
		"DELETE",
		"HOME",
		"END",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12"
	};
	
	void addKey(uint32_t key, statemachine::Condition cond, const std::string &cmd) {
		
		auto screen = currentScreen;
		bool onMain = false;
		bool onSearch = false;
		        
		currentScreen = NO_SCREEN;
		if(!cond.check()) {
			currentScreen = MAIN_SCREEN;
			onMain = cond.check();
			currentScreen = SEARCH_SCREEN;
			onSearch = cond.check();
		}
		currentScreen = screen;
		
		
		auto it = std::find(commands.begin(), commands.end(), cmd);
		if(it != commands.end()) {
			smac.add(key, cond, static_cast<uint32_t>(std::distance(commands.begin(), it)));
			if(key == grappix::Window::BACKSPACE)
				return;
			if(it->shortcut == "") {
				std::string name;
				if(key & SHIFT)
					name += "shift+";
				if(key & ALT)
					name += "alt+";
				if(key & CTRL)
					name += "ctrl+";
				key &= 0xffff;
				if(key < 0x100)
					name.append(1, tolower(key));
				else if(key <= grappix::Window::F12)
					name += utils::toLower(key_names[key - 0x100]);
				if(onSearch)
					name += " [search]";
				if(onMain)
					name += " [main]";
				it->shortcut = name;
			}
		}
	}

	void addKey(std::vector<uint32_t> events, statemachine::Condition cond,
	            const std::string &cmd) {
		for(auto &e : events)
			addKey(e, cond, cmd);
	}

	void addKey(std::vector<uint32_t> events, const std::string &cmd) {
		addKey(events, statemachine::ALWAYS_TRUE, cmd);
	}

	void addKey(uint32_t key, const std::string &cmd) {
		addKey(key, statemachine::ALWAYS_TRUE, cmd);
	}

	void clearCommand() {
		matchingCommands.resize(commands.size());
		int i = 0;
		for(auto &c : commands)
			matchingCommands[i++] = &c;
	}

	utils::File workDir;

	MusicPlayerList player;

	Screen lastScreen = MAIN_SCREEN;
	Screen currentScreen = MAIN_SCREEN;

	std::unique_ptr<TelnetInterface> telnet;

	// Aera of screen used for UI (defaults are for TV with overscan)
	utils::vec2i topLeft = {80, 54};
	utils::vec2i downRight = {636, 520};

	grappix::Font font;
	grappix::Font listFont;

	int spectrumHeight = 20;
	int spectrumWidth = 24;
	utils::vec2i spectrumPos;
	std::vector<uint8_t> eq;

	uint32_t bgcolor = 0;
	bool starsOn = true;

	LuaInterpreter lua;

	demofx::StarField starEffect;
	demofx::Scroller scrollEffect;

	// OVERLAY AND ITS RENDERABLES

	RenderSet overlay;
	TextField toastField;

	// MAINSCREEN AND ITS RENDERABLES
	RenderSet mainScreen;

	SongInfoField currentInfoField;
	SongInfoField nextInfoField;
	SongInfoField prevInfoField;
	SongInfoField outsideInfoField;

	TextField timeField;
	TextField lengthField;
	TextField songField;
	TextField nextField;
	TextField xinfoField;
	TextField playlistField;

	// SEARCHSCREEN AND ITS RENDERABLES
	RenderSet searchScreen;

	LineEdit searchField;
	TextField filterField;
	TextField topStatus;
	grappix::VerticalList songList;

	// COMMANDSCREEN AND ITS RENDERABLES

	RenderSet commandScreen;
	LineEdit commandField;
	grappix::VerticalList commandList;

	//

	TextField resultFieldTemplate;

	string currentNextPath;
	SongInfo currentInfo;
	SongInfo dbInfo;
	int currentTune = 0;

	tween::Tween currentTween;
	bool lockDown = false;
	bool isFavorite = false;

	Icon favIcon;
	Icon netIcon;
	Icon volumeIcon;

	grappix::Rectangle favPos = {80, 300, 16 * 8, 16 * 6};
	grappix::Rectangle volPos;

	int numLines = 20;

	tween::Tween markTween;

	Color timeColor;
	Color spectrumColor = 0xffffffff;
	Color spectrumColorMain = 0xff00aaee;
	Color spectrumColorSearch = 0xff111155;
	Color markColor = 0xff00ff00;
	Color hilightColor = 0xffffffff;

	std::shared_ptr<IncrementalQuery> iquery;

	bool haveSearchChars = false;

	statemachine::StateMachine smac;

	std::string currentPlaylistName = "Favorites";
	std::string editPlaylistName;

	bool playlistEdit = false;

	bool commandMode = false;

	std::shared_ptr<Dialog> currentDialog;

	std::string userName;

	std::pair<float, float> screenSize;
	int resizeDelay = 0;
	int showVolume = 0;

	bool hasMoved = false;

	bool indexingDatabase = false;

	MusicBars musicBars;
	MusicPlayerList::State playerState;
	std::string scrollText;

	struct Command {
		Command(const std::string &name, const std::function<void()> fn) : name(name), fn(fn) {}
		std::string name;
		std::function<void()> fn;
		std::string shortcut;
		bool operator==(const std::string &n) { return n == name; }
		bool operator==(const Command &c) { return c.name == name; }
	};

	std::vector<Command> commands;
	std::vector<Command*> matchingCommands;

	int lastKey = 0;
	bool searchUpdated = false;
	std::string filter;
	uint32_t favColor = 0x884444;
};
}

#endif // CHIP_MACHINE_H
