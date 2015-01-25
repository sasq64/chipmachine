#include "ChipMachine.h"
#include "PlaylistDatabase.h"
#include "Icons.h"

#include <cctype>

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

#ifndef RASPBERRYPI
#define PIXEL_EQ
#endif

namespace chipmachine {


void ChipMachine::render_item(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) {

	if(commandMode)
		render_command(rec, y, index, hilight);
	else
		render_song(rec, y, index, hilight);
}

struct Command {
	string name;
	string luaFunction;
	uint32_t ShortCut;
};

void ChipMachine::render_command(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) {
}

void ChipMachine::render_song(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) {

	static const uint32_t colors[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffff00ff, 0xffffff00, 0xff00ffff, 0xff4488ff, 0xff8888ff, 0xff8844ff  };
	Color c;
	string text;

	if(index < playlists.size()) {
		text = format("<%s>", playlists[index]);
		c = Color(0xff6688ff);
	} else {
		auto res = iquery.getResult(index-playlists.size());
		auto parts = split(res, "\t");
		int f = atoi(parts[3].c_str());
		text = format("%s / %s", parts[0], parts[1]);
		//c = colors[f>>8];//resultFieldTemplate->color;
		int g = f&0xff;
		c = 0xff555555;
		if(g == MP3)
			c = 0x088ff88;
		else
		if(g >= AMIGA)
			c = 0xff6666cc;
		else if(g >= PC)
			c = 0xffcccccc;
		else if(g >= ATARI)
			c = 0xffcccc33;
		else if(g >= C64)
			c = 0xffcc8844;
		else if(g >= CONSOLE)
			c = 0xffdd3355;
		c = c * 0.75f;
	}

	if(hilight) {
		static uint32_t markStartcolor = 0;
		if(markStartcolor != c) {
			markStartcolor = c;
			markColor = c;
			markTween = Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
	  		markTween.start();
	  	}
	  	c = markColor;
	}

	grappix::screen.text(listFont, text, rec.x, rec.y, c, resultFieldTemplate->scale);
}

static const std::string eqShaderV = R"(
	attribute vec4 vertex;
	attribute vec2 uv;

	uniform mat4 matrix;
	varying vec2 UV;

	void main() {
		gl_Position = matrix * vertex;
		//vec4 r = matrix * vec4(uv, 0.0, 1.0);
		UV = uv;
		UV.y = uv.y * -10.0 * matrix[1].y;
	}
)";

static const std::string eqShaderF = R"(
	uniform float slots[25];
	uniform float specx;
	uniform float specy;
	uniform float specw;
	uniform float spech;

	const vec4 color0 = vec4(0.0, 0.0, 0.0, 1.0);
	const vec4 color1 = vec4(0.0, 0.4, 0.0, 1.0);
	const vec4 color2 = vec4(0.8, 0.8, 0.0, 1.0);

	varying vec2 UV;

	uniform sampler2D sTexture;

	void main() {

		vec4 c = texture2D(sTexture, UV);

		// int(h) is the eq slot to read
		float h = 24.0 * (gl_FragCoord.x - specx) / specw;
		
		float f = fract(h);

		// Linear interpolation between slots
		// float y = mix(slots[int(h)], slots[int(h)+1], f);
		float y = slots[int(h)]; // 0 -> spech

		// Blend from color0 -> color1 -> color 0 over y+4 -> y-4

		float fy = gl_FragCoord.y - specy;

		//vec4 rgb = mix(color1, color2, (y - fy) / specy);
		//vec4 rgb = mix(color0, color2, smoothstep(y + 2.0, y - 2.0, fy));

		vec4 rgb = mix(color0, c, smoothstep(y + 1.0, y - 1.0, fy));

		//vec4 rgb = mix(color1, color2, smoothstep(fy + 20.0, fy - 20.0, y));
		//rgb = mix(rgb, color0, smoothstep(fy - 2.0, fy - 4.0, y));


		//rgb = rgb * mod(h, 1.0);
		//rgb = rgb * mod(UV.y * 256.0, 2.0);

		// MODIFY UV HERE
		//vec4 color = texture2D(sTexture, UV);
		// MODIFY COLOR HERE
		gl_FragColor = rgb;
	}
)";

ChipMachine::ChipMachine() : currentScreen(0), eq(SpectrumAnalyzer::eq_slots), starEffect(screen), scrollEffect(screen), commandMode(false) {

	RemoteLists::getInstance().onError([=](int rc, const std::string &error) {
		string e = error;
		if(rc == RemoteLists::JSON_INVALID)
			e = "Server unavailable";
		toast(e, 1);
		player.setReportSongs(false);
	});

	PlaylistDatabase::getInstance();

	auto path = current_exe_path() + ":" + File::getAppDir();
	LOGD("####### Finding font");
	File ff = File::findFile(path, "data/Bello.otf");
	scrollEffect.set("font", ff.getName());


#ifdef ENABLE_TELNET
	telnet = make_unique<TelnetInterface>(player);
	telnet->start();
#endif

	for(int i=0; i<3; i++) {
		mainScreen.add(prevInfoField.fields[i]);
		mainScreen.add(currentInfoField.fields[i]);
		mainScreen.add(nextInfoField.fields[i]);
		mainScreen.add(outsideInfoField.fields[i]);
	}

	xinfoField = make_shared<TextField>();
	mainScreen.add(xinfoField);

	nextField = make_shared<TextField>();
	mainScreen.add(nextField);

	timeField = make_shared<TextField>();
	mainScreen.add(timeField);
	lengthField = make_shared<TextField>();
	mainScreen.add(lengthField);
	songField = make_shared<TextField>();
	mainScreen.add(songField);


	auto bm = image::bitmap(8, 6, &heart_icon[0]);
	favTexture = Texture(bm);
	glBindTexture(GL_TEXTURE_2D, favTexture.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	bm = image::bitmap(8, 5, &net_icon[0]);
	netTexture = Texture(bm);
	glBindTexture(GL_TEXTURE_2D, netTexture.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	bm = image::bitmap(19, 19, &volume_icon[0]);
	volumeTexture = Texture(bm);
	glBindTexture(GL_TEXTURE_2D, volumeTexture.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	showVolume = 0;
	volPos = { (float)screen.width() - 19*5 - 5, (float)screen.height() - 19*3 - 5, 19*5, 19*3 };

	// SEARCHSCREEN

	iquery = MusicDatabase::getInstance().createQuery();

	resultFieldTemplate = make_shared<TextField>();
	searchField = make_shared<LineEdit>();
	searchField->setPrompt("#");
	searchScreen.add(searchField);
	searchField->visible(false);

	topStatus = make_shared<TextField>();
	searchScreen.add(topStatus);
	topStatus->visible(false);

	setup_rules();

	initLua();
	MusicDatabase::getInstance().initFromLua();
	layoutScreen();

	oldWidth = screen.width();
	oldHeight = screen.height();
	resizeDelay = 0;

	songList = VerticalList(this, grappix::Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28), numLines);
	playlistField = make_shared<TextField>(listFont, "Favorites", tv1.x - 80, tv1.y - 10, 0.5, 0xff888888);
	//mainScreen.add(playlistField);

	//volumeField = make_shared<TextField>(listFont, "Favorites", tv1.x - 80, tv1.y - 10, 0.5, 0xff888888);

	commandField = make_shared<LineEdit>(font, ">", tv0.x, tv0.y, 1.0, 0xff888888);
	searchScreen.add(commandField);
	commandField->visible(false);

	scrollEffect.set("scrolltext", "Chipmachine Beta 1 -- Begin typing to search -- CRSR UP/DOWN to select -- ENTER to play, SHIFT+ENTER to enque -- CRSR LEFT/RIGHT for subsongs -- F6 for next song -- F5 for pause -- F8 to clear queue -- ESCAPE to clear search text ----- ");
	toastField = make_shared<TextField>(font, "", tv0.x, tv1.y - 134, 2.0, 0x00ffffff);
	renderSet.add(toastField);
	starEffect.fadeIn();

	File f { File::getCacheDir() + "login" };
	if(f.exists())
		userName = f.read();

	image::bitmap eqbar(spectrumWidth*24, spectrumHeight);
	Color col(0xff00aa00);
	Color toc0(0xff00aaaa);
	Color toc1(0xff0000aa);
	int h2 = eqbar.height() / 2;
	Color deltac = (toc0 - col) / (float)h2;
	//auto eqtween = Tween::make().to(c, 0xffff0000).seconds(eqbar.height());
	for(int y=eqbar.height()-1; y>-0; y--) {
		for(int x=0; x<eqbar.width(); x++) {
			eqbar[x+y*eqbar.width()] = (y%5 > 1) && (x%spectrumWidth !=0) ? (uint32_t)col : 0x00000000;
		}
		col = col + deltac;
		if(y == h2) {
			col = toc0;
			deltac = (toc1 - col) / (float)h2;
		}
	}
	//save_png(eqbar, "bar.png");
	eqTexture = Texture(eqbar);
	glBindTexture(GL_TEXTURE_2D, eqTexture.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	eqProgram = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();
#ifdef PIXEL_EQ
	eqProgram.setFragmentSource(eqShaderF);
#endif
	//eqProgram.setVertexSource(eqShaderV);
}

ChipMachine::~ChipMachine() {
	if(telnet)
		telnet->stop();
}

void ChipMachine::set_scrolltext(const std::string &txt) {
	scrollEffect.set("scrolltext", txt);
}

void ChipMachine::initLua() {
	lua.registerFunction<void, string, uint32_t, string>("set_var", [=](string name, uint32_t index, string val) {
		setVariable(name, index, val);
	});
}

void ChipMachine::layoutScreen()  {

	LOGD("LAYOUT SCREEN");

	auto path = File::getUserDir() + ":" + current_exe_path() + ":" + File::getAppDir();
	File f = File::findFile(path, "lua/screen.lua");

	lua.setGlobal("SCREEN_WIDTH", screen.width());
	lua.setGlobal("SCREEN_HEIGHT", screen.height());

	Resources::getInstance().load<string>(f.getName() /*"lua/screen.lua" */, [=](shared_ptr<string> contents) {
		lua.load(*contents, "lua/screen.lua");

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

	starEffect.resize(screen.width(), screen.height());
	scrollEffect.resize(screen.width(), screen.height());
}

void ChipMachine::play(const SongInfo &si) {
	player.addSong(si);
	player.nextSong();
}

void ChipMachine::update() {

	static string msg;
	auto m = player.getMeta("message");
	if(m != msg) {
		msg = m;
		// Turn linefeeds into spaces
		replace(m.begin(), m.end(), '\n', ' ');
		// Turn space sequences into single spaces
		auto last = unique(m.begin(), m.end(), [](const char &a, const char &b) -> bool {
			return (a == ' ' && b == ' ');
		});
		m.resize(last - m.begin());
		scrollEffect.set("scrolltext", m);
	}

	if(currentDialog && currentDialog->getParent() == nullptr)
		currentDialog = nullptr;

	update_keys();

	auto state = player.getState();
	//LOGD("STATE %d vs %d %d %d", state, MusicPlayerList::STOPPED, MusicPlayerList::WAITING, MusicPlayerList::PLAY_STARTED);
	if(state == MusicPlayerList::PLAY_STARTED) {
		LOGD("MUSIC STARTING");
		currentInfo = player.getInfo();
		LOGD("Prev song %s, new song %s", currentInfoField.getInfo().title, currentInfo.title);
		prevInfoField.setInfo(currentInfoField.getInfo());
		currentInfoField.setInfo(currentInfo);
		currentTune = currentInfo.starttune;
		currentTween.finish();

		if(currentInfo.numtunes > 0)
			songField->setText(format("[%02d/%02d]", currentTune+1, currentInfo.numtunes));
		else
			songField->setText("[01/01]");

		auto sub_title = player.getMeta("sub_title");

		int tw = font.get_width(currentInfo.title, currentInfoField[0].scale);

		auto f = [=]() {
			xinfoField->setText(sub_title);
			int d = (tw-(tv1.x-tv0.x-20));
			if(d > 20)
				Tween::make().sine().repeating().to(currentInfoField[0].pos.x, currentInfoField[0].pos.x - d).seconds((d+200)/200.0f);
		};

		auto favorites = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
		auto favsong = find(favorites.begin(), favorites.end(), currentInfo);
		isFavorite = (favsong != favorites.end());

		if(nextInfoField == currentInfoField) {
			currentTween = Tween::make().from(prevInfoField, currentInfoField).
			from(currentInfoField, nextInfoField).
			from(nextInfoField, outsideInfoField).seconds(1.5).onComplete(f);
		} else {
			currentTween = Tween::make().from(prevInfoField, currentInfoField).
			from(currentInfoField, outsideInfoField).seconds(1.5).onComplete(f);
		}
		currentTween.start();

	}

	if(state == MusicPlayerList::PLAYING || state == MusicPlayerList::STOPPED) {
		auto psz = player.listSize();
		if(psz > 0) {
			auto n = player.getInfo(1);
			if(n.path != currentNextPath) {
				if(n.title == "") {
					n.title = path_filename(urldecode(n.path, ""));
				}

				if(psz == 1)
					nextField->setText("Next");
				else
					nextField->setText(format("Next (%d)", psz));
				nextInfoField.setInfo(n);
				currentNextPath = n.path;
			}
		} else if(nextField->getText() != "") {
			nextInfoField.setInfo(SongInfo());
			nextField->setText("");
		}
	}

	int tune = player.getTune();
	if(currentTune != tune) {
		songField->add = 0.0;
		Tween::make().sine().to(songField->add, 1.0).seconds(0.5);
		currentInfo = player.getInfo();
		auto sub_title = player.getMeta("sub_title");
		xinfoField->setText(sub_title);
		currentInfoField.setInfo(currentInfo);
		currentTune = tune;
		songField->setText(format("[%02d/%02d]", currentTune+1, currentInfo.numtunes));
	}

	if(player.playing()) {

		bool party = (player.getPermissions() & MusicPlayerList::PARTYMODE) != 0;
		if(!lockDown && party) {
			lockDown = true;
			Tween::make().to(timeField->color, Color(0xffff0000)).seconds(0.5);
		} else if(lockDown && !party) {
			lockDown = false;
			Tween::make().to(timeField->color, timeColor).seconds(2.0);
		}


		auto p = player.getPosition();
		int length = player.getLength();
		timeField->setText(format("%02d:%02d", p/60, p%60));
		if(length > 0)
			lengthField->setText(format("(%02d:%02d)", length/60, length%60));
		else
			lengthField->setText("");

		auto sub_title = player.getMeta("sub_title");
		if(sub_title != xinfoField->getText())
			xinfoField->setText(sub_title);
	}

	if(!player.getAllowed()) {
		toast("Not allowed", 1);
	} else
	if(player.hasError()) {
		toast(player.getError(), 1);
	}

	for(int i=0; i<(int)eq.size(); i++) {
		if(!player.isPaused()) {
			if(eq[i] >= 4*4)
				eq[i]-=2*4;
			else
				eq[i] = 2*4;
		}
	}

	if(player.playing()) {
		auto spectrum = player.getSpectrum();
		for(int i=0; i<player.spectrumSize(); i++) {
			if(spectrum[i] > 5) {
				unsigned f = static_cast<uint8_t>(logf(spectrum[i]) * 64);
				//LOGD("%d %d\n", spectrumHeight, f);
				if(f > 255) f = 255;
				if(f > eq[i])
					eq[i] = f;
			}
		}
	}

}

void ChipMachine::toast(const std::string &txt, int type) {

	static vector<Color> colors = { 0xffffff, 0xff8888, 0x55aa55 }; // Alpha intentionally left at zero

	toastField->setText(txt);
	int tlen = toastField->getWidth();
	toastField->pos.x = tv0.x + ((tv1.x - tv0.x) - tlen) / 2;
	toastField->color = colors[type];

	Tween::make().to(toastField->color.alpha, 1.0).seconds(0.25).onComplete([=]() {
		Tween::make().to(toastField->color.alpha, 0.0).delay(1.0).seconds(0.25);
	});
}

void ChipMachine::render(uint32_t delta) {

	if(oldWidth != screen.width() || oldHeight != screen.height())
		resizeDelay = 2;
	oldWidth = screen.width();
	oldHeight = screen.height();

	if(resizeDelay) {
		resizeDelay--;
		if(resizeDelay == 0) {
			layoutScreen();
		}
	}


	screen.clear(0xff000000 | bgcolor);

#ifdef PIXEL_EQ
	static std::vector<float> fSlots(25);
	for(int i=0; i<24; i++) {
		fSlots[i] = spectrumHeight * eq[i]  / 256.0;
	}
	fSlots[24] = fSlots[23];

	eqProgram.use();
	eqProgram.setUniform("slots", &fSlots[0], 25);
	eqProgram.setUniform("specx", spectrumPos.x);
	eqProgram.setUniform("specy", screen.height() - spectrumPos.y);
	eqProgram.setUniform("specw", spectrumWidth * 24);
	eqProgram.setUniform("spech", spectrumHeight);
	screen.draw(eqTexture, spectrumPos.x, spectrumPos.y-spectrumHeight, spectrumWidth * 24, spectrumHeight, nullptr, eqProgram);
#else
	screen.draw(eqTexture, spectrumPos.x, spectrumPos.y-spectrumHeight, spectrumWidth * 24, spectrumHeight, nullptr);
	for(int i=0; i<(int)eq.size(); i++) {
		screen.rectangle(spectrumPos.x + (spectrumWidth)*i, spectrumPos.y-spectrumHeight, spectrumWidth, spectrumHeight-(spectrumHeight * eq[i]  / 256), 0xff000000);
	}
#endif

	if(starsOn)
		starEffect.render(delta);
	scrollEffect.render(delta);


	if(currentScreen == MAIN_SCREEN) {
		mainScreen.render(delta);
		if(isFavorite)
			screen.draw(favTexture, favPos.x, favPos.y, favPos.w, favPos.h, nullptr);
	} else {
		searchScreen.render(delta);
		songList.render();
	}

	if(showVolume) {
		static Color color = 0xff000000;
		showVolume--;

		//if(showVolume == 10)
		//	tween::make().to(color, 0x0).seconds(0.5);

		screen.draw(volumeTexture, volPos.x, volPos.y, volPos.w, volPos.h, nullptr);
		int v = player.getVolume() * 10;
		v = v * volPos.w / 10;
		screen.rectangle(volPos.x + v, volPos.y, volPos.w - v, volPos.h, color);
		screen.text(listFont, std::to_string((int)(v*100)), volPos.x, volPos.y, 1.0, 0xff8888ff);
	}

	if(WebRPC::inProgress() > 0 || WebGetter::inProgress() > 0) {
		screen.draw(netTexture, 2, 2, 8*3, 5*3, nullptr);
	}

	renderSet.render(delta);

	font.update_cache();
	listFont.update_cache();

	screen.flip();
}


}