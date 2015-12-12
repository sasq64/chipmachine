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
#include "renderset.h"

#include "LineEdit.h"
#include "Dialog.h"

#include "PiTFT.h"

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

class ChipMachine : public grappix::VerticalList::Renderer {
public:
	virtual void renderItem(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) override;
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
	void setVariable(const std::string &name, int index, const std::string &val);

	void showMain();
	void showSearch();
	SongInfo getSelectedSong();

	void setupRules();
	void updateKeys();

	utils::File workDir;

	MusicPlayerList player;

	RenderSet renderSet;
	TextField toastField;

	const int MAIN_SCREEN = 0;
	const int SEARCH_SCREEN = 1;

	int currentScreen = MAIN_SCREEN;

	std::unique_ptr<TelnetInterface> telnet;

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

	std::string code;

	LuaInterpreter lua;

	demofx::StarField starEffect;
	demofx::Scroller scrollEffect;

	RenderSet mainScreen;
	// grappix::Font font;

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

	string currentNextPath;
	SongInfo currentInfo;
	SongInfo dbInfo;
	int currentTune = 0;

	tween::Tween currentTween;
	bool lockDown = false;
	bool isFavorite = false;
	grappix::Texture favTexture;
	grappix::Texture netTexture;
	grappix::Texture eqTexture;
	grappix::Texture volumeTexture;
	grappix::Texture extTexture;
	grappix::Rectangle favPos = {80, 300, 16 * 8, 16 * 6};

	RenderSet searchScreen;

	LineEdit searchField;
	LineEdit commandField;
	TextField topStatus;

	TextField resultFieldTemplate;

	grappix::Rectangle volPos;

	int numLines = 20;

	tween::Tween markTween;

	grappix::Color timeColor;
	grappix::Color spectrumColor = 0xffffffff;
	grappix::Color spectrumColorMain = 0xff00aaee;
	grappix::Color spectrumColorSearch = 0xff111155;
	grappix::Color markColor = 0xff00ff00;
	grappix::Color hilightColor = 0xffffffff;

	std::shared_ptr<IncrementalQuery> iquery;

	grappix::VerticalList songList;

	std::vector<std::string> playlists;

	bool haveSearchChars;

	statemachine::StateMachine smac;

	std::string currentPlaylistName = "Favorites";
	std::string editPlaylistName;

	bool playlistEdit = false;

	bool commandMode = false;

	std::shared_ptr<Dialog> currentDialog;

	std::string userName;

	int oldWidth;
	int oldHeight;
	int resizeDelay;
	int showVolume;

	bool hasMoved = false;

	bool indexingDatabase = false;

	MusicBars musicBars;
	MusicPlayerList::State playerState;
	std::string scrollText;
};
}

#endif // CHIP_MACHINE_H
