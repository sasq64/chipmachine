#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"

#include "MainScreen.h"
#include "SearchScreen.h"

#include <tween/tween.h>
#include <grappix/grappix.h>
#include <lua/luainterpreter.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace chipmachine {


class ChipMachine {
public:
	ChipMachine();
	void initLua();
	void play(const SongInfo &si);
	void update();
	void render(uint32_t delta);

private:


	MusicPlayerList player;

	MainScreen mainScreen;
	SearchScreen searchScreen;

	int currentScreen = 0;

	std::unique_ptr<TelnetInterface> telnet;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	grappix::Color spectrumColor { 0xffffffff };
	grappix::Color spectrumColorMain { 0xff00aaee };
	grappix::Color spectrumColorSearch { 0xff111155 };
	double spectrumHeight = 20.0;
	int spectrumWidth = 24;
	utils::vec2i spectrumPos;
	std::vector<uint8_t> eq;


	LuaInterpreter lua;

	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;

};

}


#endif // CHIP_MACHINE_H
