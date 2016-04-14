#include "ChipMachine.h"
#include "Icons.h"
#include "version.h"

#include <coreutils/format.h>

#include <cctype>
#include <map>

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

std::string compressWhitespace(std::string &&m) {
	// Turn linefeeds into spaces
	replace(m.begin(), m.end(), '\n', ' ');
	// Turn whitespace sequences into single spaces
	auto last = unique(m.begin(), m.end(),
	                   [](char a, char b) -> bool { return (isspace(a) && isspace(b)); });
	m.resize(distance(m.begin(), last));
	return m;
}

std::string compressWhitespace(const std::string &text) {
	return compressWhitespace(std::string(text));
}

namespace chipmachine {

void ChipMachine::renderSong(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) {

	static const map<uint32_t, uint32_t> colors = {
	    {NOT_SET, 0xffff00ff}, {PLAYLIST, 0xffffff88}, {CONSOLE, 0xffdd3355}, {C64, 0xffcc8844},
	    {ATARI, 0xffcccc33},   {MP3, 0xff88ff88},      {M3U, 0xffaaddaa},     {YOUTUBE, 0xffff0000},
	    {PC, 0xffcccccc},      {AMIGA, 0xff6666cc},    {255, 0xff00ffff}};

	Color c;
	string text;

	auto res = iquery->getResult(index);
	auto parts = split(res, "\t");
	int f = atoi(parts[3].c_str()) & 0xff;

	if(f == PLAYLIST) {
		if(parts[1] == "")
			text = format("<%s>", parts[0]);
		else
			text = format("<%s / %s>", parts[0], parts[1]);
	} else {
		if(parts[1] == "")
			text = parts[0];
		else
			text = format("%s / %s", parts[0], parts[1]);
	}
	auto it = --colors.upper_bound(f);
	c = it->second;
	c = c * 0.75f;

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

	grappix::screen.text(listFont, text, rec.x, rec.y, c, resultFieldTemplate.scale);
}

ChipMachine::ChipMachine(const std::string &wd)
    : workDir(wd), player(wd), currentScreen(MAIN_SCREEN), eq(SpectrumAnalyzer::eq_slots),
      starEffect(screen), scrollEffect(screen) {

#ifdef USE_REMOTELISTS
	RemoteLists::getInstance().onError([=](int rc, const std::string &error) {
		string e = error;
		if(rc == RemoteLists::JSON_INVALID)
			e = "Server unavailable";
		screen.run_safely([=] { toast(e, 1); });
	});
#endif

	File ff = File::findFile(workDir, "data/Bello.otf");
	scrollEffect.set("font", ff.getName());

#ifdef ENABLE_TELNET
	telnet = make_unique<TelnetInterface>(player);
	telnet->start();
#endif

	// SongInfo fields
	mainScreen.add(&prevInfoField);
	mainScreen.add(&currentInfoField);
	mainScreen.add(&nextInfoField);
	mainScreen.add(&outsideInfoField);

	// Other text fields
	mainScreen.add(&xinfoField);
	mainScreen.add(&nextField);
	mainScreen.add(&timeField);
	mainScreen.add(&lengthField);
	mainScreen.add(&songField);

	// SEARCHSCREEN

	iquery = MusicDatabase::getInstance().createQuery();

	searchField.setPrompt("#");
	searchScreen.add(&searchField);
	searchField.visible(false);

	searchScreen.add(&topStatus);
	topStatus.visible(false);

	//toastField = TextField(font, "", topLeft.x, downRight.y - 134, 2.0, 0x00ffffff);
	overlay.add(&toastField);
	
	Resources::getInstance().load<image::bitmap>(File::getCacheDir() / "favicon.png",
		[=](shared_ptr<image::bitmap> bitmap) {
			favIcon = Icon(heart_icon, favPos.x, favPos.y, favPos.w, favPos.h);
		}, heart_icon);
	//favIcon = Icon(heart_icon, favPos.x, favPos.y, favPos.w, favPos.h);
	
	setupCommands();
	setupRules();

	initLua();
	layoutScreen();

	filterField = searchField;
	searchScreen.add(&filterField);
	filterField.visible(false);
	filterField.color = 0xff55ff55;

	mainScreen.add(&favIcon);
	//favIcon.visible(false);
	favIcon.color = Color(favColor);

	netIcon = Icon(net_icon, 2, 2, 8 * 3, 5 * 3);
	mainScreen.add(&netIcon);
	netIcon.visible(false);

	Tween::make().sine().repeating().from(netIcon.color, hilightColor).seconds(1.0);

	float ww = volume_icon.width() * 15;
	float hh = volume_icon.height() * 10;
	volPos = {((float)screen.width() - ww) / 2.0f, ((float)screen.height() - hh) / 2.0f, ww, hh};
	volumeIcon = Icon(volume_icon, volPos.x, volPos.y, volPos.w, volPos.h);
	showVolume = 0;

	musicBars.setup(spectrumWidth, spectrumHeight, 24);


	LOGD("WORKDIR %s", workDir.getName());
	MusicDatabase::getInstance().initFromLuaAsync(this->workDir);

	if(MusicDatabase::getInstance().busy()) {
		indexingDatabase = true;
	}

	screenSize = screen.size();
	resizeDelay = 0;

	auto listrec = grappix::Rectangle(topLeft.x, topLeft.y + 30 * searchField.scale,
	                                  screen.width() - topLeft.x,
	                                  downRight.y - topLeft.y - searchField.scale * 30);
	songList =
	    VerticalList(listrec, numLines, [=](grappix::Rectangle &rec, int y, uint32_t index,
	                                        bool hilight) { renderSong(rec, y, index, hilight); });
	
	searchScreen.add(&songList);

	commandList = VerticalList(listrec, numLines, [=](grappix::Rectangle &rec, int y,
	                                                  uint32_t index, bool hilight) {
		if(index < matchingCommands.size()) {
			auto cmd = matchingCommands[index];
			uint32_t c = 0xaa00cc00;
			if(hilight) {
				static uint32_t markStartcolor = 0;
				if(markStartcolor != c) {
					markStartcolor = c;
					markColor = c;
					markTween =
					    Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
					markTween.start();
				}
				c = markColor;
			}
			static int cmdPos = -1;
			if(cmdPos == -1)
				cmdPos = listFont.get_width("012345678901234567890123456789", resultFieldTemplate.scale);
			grappix::screen.text(listFont, cmd->name, rec.x, rec.y, c, resultFieldTemplate.scale);
			grappix::screen.text(listFont, cmd->shortcut, rec.x + cmdPos, rec.y, 0xffffffff, resultFieldTemplate.scale * 0.8);
			
		}
	});

	commandList.setTotal(commands.size());
	clearCommand();
	
	updateLists();

	// playlistField = TextField(listFont, "Favorites", downRight.x - 80, downRight.y - 10, 0.5,
	// 0xff888888);
	// mainScreen.add(playlistField);

	//commandField = LineEdit(font, "", topLeft.x, topLeft.y, 1.0, 0xff888888);
	commandScreen.add(&commandField);
	commandScreen.add(&commandList);

	scrollText = "INITIAL_TEXT";
	scrollEffect.set("scrolltext",
	                 "Chipmachine " VERSION_STR " -- Just type to search -- UP/DOWN to select "
	                 "-- ENTER to play -- Press TAB to show all commands ---");
	starEffect.fadeIn();

	File f{File::getCacheDir() / "login"};
	if(f.exists())
		userName = f.read();
}

ChipMachine::~ChipMachine() {
#ifdef ENABLE_TELNET
	if(telnet)
		telnet->stop();
#endif
}

void ChipMachine::setScrolltext(const std::string &txt) {
	scrollEffect.set("scrolltext", txt);
}

void ChipMachine::initLua() {
	lua.registerFunction(
	    "set_var", [=](string name, uint32_t index, string val) { setVariable(name, index, val); });
}

void ChipMachine::layoutScreen() {

	LOGD("LAYOUT SCREEN");
	currentTween.finish();
	currentTween = Tween();

	File f(workDir / "lua" / "screen.lua");

	lua.setGlobal("SCREEN_WIDTH", screen.width());
	lua.setGlobal("SCREEN_HEIGHT", screen.height());
	lua.setGlobal("SCREEN_PPI", screen.getPPI() < 0 ? 100 : screen.getPPI());

	Resources::getInstance().load<string>(f.getName(), [=](shared_ptr<string> contents) {
		lua.load(*contents, f);

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
	scrollEffect.resize(screen.width(), 45 * scrollEffect.scrollsize);
	musicBars.setup(spectrumWidth, spectrumHeight, 24);
	
	searchField.setFont(font);
	commandField.pos = searchField.pos;
	commandField.scale = searchField.scale;
	commandField.cursorH = searchField.cursorH;
	commandField.cursorW = searchField.cursorW;
	
	favIcon.set(favPos);
}

void ChipMachine::play(const SongInfo &si) {
	player.addSong(si);
	player.nextSong();
}

void ChipMachine::updateFavorite() {
	auto favorites = MusicDatabase::getInstance().getPlaylist(currentPlaylistName);
	auto favsong = find_if(favorites.begin(), favorites.end(), [&](const SongInfo &song) -> bool {
		return (song.path == currentInfo.path && (currentTune == song.starttune || (currentTune == currentInfo.starttune && song.starttune == -1)));
	});
	bool last = isFavorite;
	isFavorite = (favsong != favorites.end());
	uint32_t alpha = isFavorite ? 0xff : 0x00;
	favIcon.color = Color(favColor | (alpha << 24));
	//favIcon.visible(isFavorite);
}

void ChipMachine::update() {

	if(indexingDatabase) {

		static int delay = 30;
		if(delay-- == 0)
			toast("Indexing database", 3);

		if(!MusicDatabase::getInstance().busy()) {
			indexingDatabase = false;
			removeToast();
		} else
			return;
	}

	auto click = screen.get_click();
	if(click != Window::NO_CLICK) {
		LOGD("Clicked at %d %d\n", click.x, click.y);
	}

	if(currentDialog && currentDialog->getParent() == nullptr)
		currentDialog = nullptr;

	updateKeys();

	// DEAL WITH MUSICPLAYER STATE

	playerState = player.getState();

	if(playerState == MusicPlayerList::PLAY_STARTED) {
		LOGD("MUSIC STARTING %s", currentInfo.title);
		timeField.add = 0;
		currentInfo = player.getInfo();
		dbInfo = player.getDBInfo();
		string m;
		if(currentInfo.metadata != "") {
			m = compressWhitespace(currentInfo.metadata);
		} else {
			m = compressWhitespace(player.getMeta("message"));
		}
		if(scrollText != m) {
			scrollEffect.set("scrolltext", m);
			scrollText = m;
		}

		// Make sure any previous tween is complete
		currentTween.finish();
		currentInfoField[0].pos.x = currentInfoField[1].pos.x;
		prevInfoField = currentInfoField;

		// Update current info, rely on tween to make sure it will fade in
		currentInfoField.setInfo(currentInfo);
		//currentTune = currentInfo.starttune;
		currentTune = player.getTune();

		if(currentInfo.numtunes > 0)
			songField.setText(format("[%02d/%02d]", currentTune + 1, currentInfo.numtunes));
		else
			songField.setText("[01/01]");

		auto sub_title = player.getMeta("sub_title");

		int tw = currentInfoField.getWidth(0);

		auto f = [=]() {
			xinfoField.setText(sub_title);
			int d = (tw - (downRight.x - topLeft.x - 20));
			if(d > 20)
				Tween::make()
				    .sine()
				    .repeating()
				    .to(currentInfoField[0].pos.x, currentInfoField[0].pos.x - d)
				    .seconds((d + 200) / 200.0f);
		};

		updateFavorite();
		// Start tweening

		if(player.wasFromQueue()) {
			currentTween = Tween::make()
			                   .from(prevInfoField, currentInfoField)
			                   .from(currentInfoField, nextInfoField)
			                   .from(nextInfoField, outsideInfoField)
			                   .seconds(1.5)
			                   .onComplete(f);
		} else {
			currentTween = Tween::make()
			                   .from(prevInfoField, currentInfoField)
			                   .from(currentInfoField, outsideInfoField)
			                   .seconds(1.5)
			                   .onComplete(f);
		}
		currentTween.start();
	}

	if(playerState == MusicPlayerList::ERROR) {
		player.stop();
		currentTween.finish();
		currentInfoField[0].pos.x = currentInfoField[1].pos.x;

		SongInfo song = player.getInfo();
		prevInfoField.setInfo(song);
		LOGD("SONG %s could not be played", song.path);
		currentTween = Tween::make()
		                   .from(prevInfoField, nextInfoField)
		                   .seconds(3.0)
		                   .onComplete([=]() {
			                   if(playerState == MusicPlayerList::STOPPED)
				                   player.nextSong();
			               });
		currentTween.start();
	}

	if(playerState == MusicPlayerList::PLAYING || playerState == MusicPlayerList::STOPPED) {
		auto psz = player.listSize();
		if(psz > 0) {
			auto info = player.getInfo(1);
			if(info.path != "")
				RemoteLoader::getInstance().preCache(info.path);
			if(info.path != currentNextPath) {

				if(psz == 1)
					nextField.setText("Next");
				else
					nextField.setText(format("Next (%d)", psz));
				nextInfoField.setInfo(info);
				currentNextPath = info.path;
			}
		} else if(nextField.getText() != "") {
			nextInfoField.setInfo(SongInfo());
			nextField.setText("");
		}
	}

	int tune = player.getTune();
	if(currentTune != tune) {
		songField.add = 0.0;
		Tween::make().sine().to(songField.add, 1.0).seconds(0.5);
		currentInfo = player.getInfo();
		auto sub_title = player.getMeta("sub_title");
		xinfoField.setText(sub_title);
		currentInfoField.setInfo(currentInfo);
		currentTune = tune;
		songField.setText(format("[%02d/%02d]", currentTune + 1, currentInfo.numtunes));
		auto m = compressWhitespace(player.getMeta("message"));
		if(m != "" && scrollText != m) {
			scrollEffect.set("scrolltext", m);
			scrollText = m;
		}
		updateFavorite();
	}

	if(player.playing()) {

		bool party = (player.getPermissions() & MusicPlayerList::PARTYMODE) != 0;
		if(!lockDown && party) {
			lockDown = true;
			Tween::make().to(timeField.color, Color(0xffff0000)).seconds(0.5);
		} else if(lockDown && !party) {
			lockDown = false;
			Tween::make().to(timeField.color, timeColor).seconds(2.0);
		}

		auto p = player.getPosition();
		int length = player.getLength();
		timeField.setText(format("%02d:%02d", p / 60, p % 60));
		if(length > 0)
			lengthField.setText(format("(%02d:%02d)", length / 60, length % 60));
		else
			lengthField.setText("");

		auto sub_title = player.getMeta("sub_title");
		if(sub_title != xinfoField.getText())
			xinfoField.setText(sub_title);

		if(scrollText == "") {
			auto m = player.getMeta("message");
			if(m != "") {
				m = compressWhitespace(m);
				if(scrollText != m) {
					scrollEffect.set("scrolltext", m);
					scrollText = m;
				}
			}
		}
	}

	if(!player.getAllowed()) {
		toast("Not allowed", 1);
	} else if(player.hasError()) {
		toast(player.getError(), 1);
	}

	if(!player.isPaused()) {
		for(auto &e : eq) {
			if(e >= 4 * 4)
				e -= 2 * 4;
			else
				e = 2 * 4;
		}
	}

	if(player.playing()) {
		auto spectrum = player.getSpectrum();
		for(auto i : count_to(player.spectrumSize())) {
			if(spectrum[i] > 5) {
				unsigned f = static_cast<uint8_t>(logf(spectrum[i]) * 64);
				if(f > 255)
					f = 255;
				if(f > eq[i])
					eq[i] = f;
			}
		}
	}
	bool busy = (
#ifdef ENABLE_TELNET
	    WebRPC::inProgress() > 0 ||
#endif
	    playerState == MusicPlayerList::LOADING || webutils::Web::inProgress() > 0);

	netIcon.visible(busy);
}

void fadeOut(float &alpha, float t = 0.25) {
	Tween::make().to(alpha, 0.0).seconds(t);
}


void ChipMachine::toast(const std::string &txt, int type) {

	static vector<Color> colors = {0xffffff, 0xff8888,
	                               0x55aa55}; // Alpha intentionally left at zero

	toastField.setText(txt);
	int tlen = toastField.getWidth();
	toastField.pos.x = topLeft.x + ((downRight.x - topLeft.x) - tlen) / 2;
	toastField.color = colors[type % 3];

	Tween::make()
	    .to(toastField.color.alpha, 1.0)
	    .seconds(0.25)
	    .onComplete([=]() {
		    if(type < 3)
			    Tween::make().to(toastField.color.alpha, 0.0).delay(1.0).seconds(0.25);
		});
}

void ChipMachine::removeToast() {
	toastField.setText("");
	toastField.color = 0;
}

void ChipMachine::render(uint32_t delta) {

	if(screen.size() != screenSize) {
		resizeDelay = 2;
		screenSize = screen.size();
	}

	if(resizeDelay) {
		resizeDelay--;
		if(resizeDelay == 0) {
			layoutScreen();
		}
	}

	screen.clear(0xff000000 | bgcolor);

	if(showVolume) {
		static Color color = 0xff000000;
		showVolume--;

		volumeIcon.render(screenptr, 0);
		auto v = (int)(player.getVolume() * 10);
		v = (int)(v * volPos.w) / 10;
		screen.rectangle(volPos.x + v, volPos.y, volPos.w - v, volPos.h, color);
		// screen.text(listFont, std::to_string((int)(v * 100)), volPos.x, volPos.y, 10.0,
		// 0xff8888ff);
	}

	musicBars.render(spectrumPos, spectrumColor, eq);

	if(starsOn)
		starEffect.render(delta);
	scrollEffect.render(delta);

	if(currentScreen == MAIN_SCREEN) {
		mainScreen.render(screenptr, delta);
	} else if(currentScreen == SEARCH_SCREEN) {
		searchScreen.render(screenptr, delta);
	} else {
		commandScreen.render(screenptr, delta);
	}

	overlay.render(screenptr, delta);

	font.update_cache();
	listFont.update_cache();

	screen.flip();
}
}
