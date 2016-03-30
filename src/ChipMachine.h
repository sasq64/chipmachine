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
	    : texture(tx), x(x), y(y), w(w), h(h) {}

	Icon(const image::bitmap &bm, float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {
		texture = new grappix::Texture(bm);
		glBindTexture(GL_TEXTURE_2D, texture->id());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		target->draw(*texture, x, y, w, h, nullptr);
	}

private:
	grappix::Texture *texture;
	float x, y, w, h;
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
	void setVariable(const std::string &name, int index, const std::string &val);

	void showScreen(int screen);
	SongInfo getSelectedSong();

	void setupRules();
	void updateKeys();

	utils::File workDir;

	MusicPlayerList player;

	const int MAIN_SCREEN = 0;
	const int SEARCH_SCREEN = 1;
	const int COMMAND_SCREEN = 2;

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
