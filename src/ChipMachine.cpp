#include "ChipMachine.h"


using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

ChipMachine::ChipMachine() : mainScreen(player), searchScreen(player, mdb), currentScreen(0), eq(SpectrumAnalyzer::eq_slots), starEffect(screen), scrollEffect(screen) {

	mdb.init();

	telnet = make_unique<TelnetInterface>(mdb, player);
	telnet->start();

	memset(&eq[0], 2, eq.size());

	spectrumPos = { tv0.x-10, tv1.y+50 };

	initLua();

	scrollEffect.set("scrolltext", "Chipmachine Beta 1 -- Type letters/digits to search -- CRSR UP/DOWN to select -- ENTER to play, SHIFT+ENTER to enque -- CRSR LEFT/RIGHT for subsongs -- F9 for next song -- F5 for pause -- F12 to clear queue -- F10 to clear search text ----- ");
}

void ChipMachine::initLua() {

	lua.registerFunction<int, string>("get_var", [=](string name) -> int {
		if(name == "screen_width")
			return screen.width();
		else if(name == "screen_height")
			return screen.height();
		return -1;
	});

	lua.registerFunction<void, string, uint32_t, string>("set_var", [=](string name, uint32_t index, string val) {
		//LOGD("%s(%d) = %s", name, index, val);

		mainScreen.set_variable(name, index, val);
		searchScreen.set_variable(name, index, val);

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
			tv0[index-1] = stol(val);
		} else
		if(name == "down_right") {
			tv1[index-1] = stol(val);
		} else
		if(name == "scroll") {
			switch(index) {
			case 1:
				scrollEffect.scrolly = stol(val);
				break;
			case 2:
				scrollEffect.scrollsize = stod(val);
				break;
			case 3:
				scrollEffect.scrollspeed = stol(val);
				break;
			}
			LOGD("%d %f %d", scrollEffect.scrolly, scrollEffect.scrollsize, scrollEffect.scrollspeed);
		}
	});

	Resources::getInstance().load<string>("lua/init.lua", [=](const std::string &contents) {
		LOGD("init.lua");
		lua.load(R"(
			Config = {}
			Config.screen_width = get_var('screen_width')
			Config.screen_height = get_var('screen_height')
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

void ChipMachine::play(const SongInfo &si) {
	player.addSong(si);
	player.nextSong();
}

void ChipMachine::update() {

	auto show_main = [=]() {
		currentScreen = 0;
		make_tween().to(spectrumColor, spectrumColorMain).seconds(0.5);
		make_tween().to(scrollEffect.alpha, 1.0).seconds(0.5);
	};

	auto show_search = [=]() {
		currentScreen = 1;
		make_tween().to(spectrumColor, spectrumColorSearch).seconds(0.5);
		make_tween().to(scrollEffect.alpha, 0.0).seconds(0.5);
	};

	static string msg;
	auto m = player.getMeta("message");
	if(m != msg) {
		msg = m;
		replace(m.begin(), m.end(), '\n', ' ');
		auto last = unique(m.begin(), m.end(), [](const char &a, const char &b) -> bool {
			return (a == ' ' && b == ' ');
		});
		m.resize(last - m.begin());
		//LOGD("MSG:%s", m);
		scrollEffect.set("scrolltext", m);
	}

	auto k = screen.get_key();
	if(k != Window::NO_KEY) {

		if((k >= '0' && k <= '9') || (k >= 'A' && k<='Z')) {
			show_search();
		} else {
			switch(k) {
			case Window::F1:
				show_main();
				break;
			case Window::F2:
			case Window::SPACE:
			case Window::BACKSPACE:
			case Window::F10:
			case Window::ESCAPE:
				show_search();
				break;
			case Window::ENTER:
				if(!(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_LEFT))) {
					show_main();
				}
				break;
			}
		}

		mainScreen.on_key(k);
		searchScreen.on_key(k);
	}
	mainScreen.update();
	searchScreen.update();

	for(int i=0; i<(int)eq.size(); i++) {
		if(!player.isPaused()) {
			if(eq[i] >= 4)
				eq[i]-=2;
			else
				eq[i] = 2;
		}
	}

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

}

void ChipMachine::render(uint32_t delta) {

	screen.clear();

	starEffect.render(delta);
	scrollEffect.render(delta);

	for(int i=0; i<(int)eq.size(); i++) {
		screen.rectangle(spectrumPos.x + (spectrumWidth)*i, spectrumPos.y-eq[i], spectrumWidth-1, eq[i], spectrumColor);
	}

	if(currentScreen == 0)
		mainScreen.render(delta);
	else
		searchScreen.render(delta);

	screen.flip();
}


}