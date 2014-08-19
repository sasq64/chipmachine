#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextField.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"
#include "RemoteLists.h"

#include "state_machine.h"
#include "renderset.h"

#include "LineEdit.h"
#include "Dialog.h"

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
	int currentTune = 0;

	tween::TweenHolder currentTween;
	bool lockDown = false;
	bool isFavorite = false;
	grappix::Texture favTexture;
	grappix::Texture netTexture;
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

	IncrementalQuery iquery;

	grappix::VerticalList songList;

	std::vector<std::string> playlists;

	bool haveSearchChars;

	statemachine::StateMachine smac;

	std::string currentPlaylistName = "Favorites";
	std::string editPlaylistName;

	bool playlistEdit = false;

	std::shared_ptr<Dialog> currentDialog;

	std::string userName;

	uint16_t *screen2 = nullptr;
	int screen2w;
	int screen2h;
};

}


#endif // CHIP_MACHINE_H
