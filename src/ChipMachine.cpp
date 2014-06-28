#include "ChipMachine.h"

#include <ctype.h>

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

ChipMachine::ChipMachine() : mainScreen(player), searchScreen(player, mdb), currentScreen(0), eq(SpectrumAnalyzer::eq_slots), starEffect(screen), scrollEffect(screen) {

	mdb.init();

#ifdef ENABLE_TELNET
	telnet = make_unique<TelnetInterface>(mdb, player);
	telnet->start();
#endif

	memset(&eq[0], 2, eq.size());

	spectrumPos = { tv0.x-10, tv1.y+50 };

	initLua();

	scrollEffect.set("scrolltext", "Chipmachine Beta 1 -- Begin typing to to search -- CRSR UP/DOWN to select -- ENTER to play, SHIFT+ENTER to enque -- CRSR LEFT/RIGHT for subsongs -- F6 for next song -- F5 for pause -- F8 to clear queue -- ESCAPE to clear search text ----- ");
	toastField = textScreen.addField("", tv0.x, tv1.y - 134, 2.0, 0x00ffffff);
}

void ChipMachine::initLua() {

	lua.registerFunction<int, string>("get_var", [=](string name) -> int {
		if(name == "screen_width")
			return screen.width();
		else if(name == "screen_height")
			return screen.height();
		return -1;
	});

	unordered_map<string, string> dbmap;
	lua.registerFunction<void, string, string>("set_db_var", [&](string name, string val) {
		LOGD("%s %s", name, val); 
		if(val == "start") {
		} else if(val == "end") {
			mdb.initDatabase(name, dbmap);
			dbmap.clear();
		} else {
			dbmap[name] = val;
		}
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
		if(name == "font") {
			if(File::exists(val)) {
				auto font = Font(val, 48, 512 | Font::DISTANCE_MAP);
				textScreen.setFont(font);
			}
		} else
		if(name == "background") {
			bgcolor = stol(val);
		} else
		if(name == "stars") {
			starsOn = stol(val) != 0;
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
			case 4:
				scrollEffect.set("font", val);
				break;
			}
			LOGD("%d %f %d", scrollEffect.scrolly, scrollEffect.scrollsize, scrollEffect.scrollspeed);
		}
	});

	File f { "lua/db.lua" };
	if(!f.exists()) {
		f.copyFrom("lua/db.lua.orig");
		f.close();
	}

	lua.load(R"(
		DB = {}
	)");
	lua.loadFile("lua/db.lua");
	lua.load(R"(
		for a,b in pairs(DB) do 
			if type(b) == 'table' then
				set_db_var(a, 'start')
				for a1,b1 in pairs(b) do
					set_db_var(a1, b1)
				end
				set_db_var(a, 'end')
			end
		end
	)");
	mdb.generateIndex();

	File f2 { "lua/init.lua" };
	if(!f2.exists()) {
		f2.copyFrom("lua/init.lua.orig");
		f2.close();
	}

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

	if(screen.key_pressed(Window::CTRL_LEFT) || screen.key_pressed(Window::CTRL_RIGHT)) {

		if(k >= '0' && k <= 'z') {
			code.push_back(tolower(k));
			LOGD("'%s'", code);
			if(code == "party") {
				toast("Party Mode ON", 2);
				player.setPartyMode(true);
				code = "";	
			} else if(code == "chill") {
				player.setPartyMode(false);
				toast("Party Mode OFF", 2);
				code = "";	
			}
		}
	} else {
		code = "";
		
		if(k != Window::NO_KEY) {

			if((k >= '0' && k <= '9') || k == '/' || (k >= 'A' && k<='Z')) {
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
				case Window::UP:
				case Window::DOWN:
				case Window::PAGEUP:
				case Window::PAGEDOWN:
					show_search();
					break;
				}
			}

		if(currentScreen == 0)
			mainScreen.on_key(k);
		else
			searchScreen.on_key(k);
		}

		switch(k) {
		case Window::ENTER:
			if(!(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_RIGHT))) {
				show_main();
			}
			break;
		case Window::RIGHT_CLICK:
		case Window::F6:
			player.nextSong();
			show_main();
			break;
		case Window::ESCAPE:
			show_main();
			break;
		case Window::F8:
			player.clearSongs();
			toast("Playlist cleared", 2);
			break;
		//case Window::F10:
		//	player.setPermissions(0);
		//	break;
		//case Window::F9:
		//	player.setPermissions(0xffff);
		//	break;
		}
	}


	mainScreen.update();
	searchScreen.update();

	if(!player.getAllowed()) {
		toast("Not allowed", 1);
	} else
	if(player.hasError()) {
		toast(player.getError(), 1);
	}

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

void ChipMachine::toast(const std::string &txt, int type) {

	static vector<Color> colors = { 0xffffff, 0xff8888, 0x55aa55 }; // Alpha intentionally left at zero

	toastField->setText(txt);
	int tlen = textScreen.getWidth(toastField);
	toastField->pos.x = tv0.x + ((tv1.x - tv0.x) - tlen) / 2;
	toastField->color = colors[type];

	make_tween().to(toastField->color.alpha, 1.0).seconds(0.25).on_complete([=]() {
		make_tween().delay(1.0).to(toastField->color.alpha, 0.0).seconds(0.25);
	});
}

void ChipMachine::render(uint32_t delta) {

	screen.clear(0xff000000 | bgcolor);

	if(starsOn)
		starEffect.render(delta);
	scrollEffect.render(delta);

	for(int i=0; i<(int)eq.size(); i++) {
		screen.rectangle(spectrumPos.x + (spectrumWidth)*i, spectrumPos.y-eq[i], spectrumWidth-1, eq[i], spectrumColor);
	}

	if(currentScreen == 0)
		mainScreen.render(delta);
	else
		searchScreen.render(delta);

	textScreen.render(delta);

	screen.flip();
}


}