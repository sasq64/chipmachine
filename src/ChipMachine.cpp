#include "ChipMachine.h"


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

	varying vec2 UV;

	void main() {
		float m = mod(gl_FragCoord.y, 3.0);
		float uvx = mod(UV.x + scrollpos * m, 1.0);
		gl_FragColor = m * texture2D(sTexture, vec2(uvx, UV.y));
	}
)";


ChipMachine::ChipMachine() : mainScreen(player), searchScreen(player, modland), currentScreen(0), eq(SpectrumAnalyzer::eq_slots)  {

	image::bitmap bm(screen.width(), screen.height());
	bm.clear(0x00000000);
	for(int y=0; y<bm.height(); y++) {
		auto x = rand() % bm.width();
		bm[y*bm.width()+x] = bm[y*bm.width()+x + 1] = 0xff666666;
		bm[y*bm.width()+x + 2] = 0xff444444;
	}
	starTexture = Texture(bm);

	starProgram = get_program(TEXTURED_PROGRAM).clone();
	starProgram.setFragmentSource(starShaderF);

	modland.init();

	telnet = make_unique<TelnetInterface>(modland, player);
	telnet->start();

	memset(&eq[0], 2, eq.size());

	spectrumPos = { tv0.x-10, tv1.y+50 };

	initLua();
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
		LOGD("%s(%d) = %s", name, index, val);

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
		}
	});

	Resources::getInstance().load_text("lua/init.lua", [=](const std::string &contents) {
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
		make_tween().to(spectrumColor, spectrumColorMain).seconds(1.0);
	};

	auto show_search = [=]() {
		currentScreen = 1;
		make_tween().to(spectrumColor, spectrumColorSearch).seconds(1.0);
	};

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

	starProgram.use();
	starProgram.setUniform("scrollpos", starPos += (0.3 / screen.width()));
	screen.draw(starTexture, starProgram);

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