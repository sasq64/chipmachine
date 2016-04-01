#include "ChipMachine.h"

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

static const uint32_t SHIFT = 0x10000;
static const uint32_t CTRL = 0x20000;
static const uint32_t ALT = 0x40000;

void ChipMachine::setupRules() {

	using namespace statemachine;

	addKey(Window::F1, "show_main");
	addKey({Window::UP, Window::DOWN, Window::PAGEUP, Window::PAGEDOWN},
	       if_equals(currentScreen, MAIN_SCREEN), "show_search");
	addKey(Window::F2, "show_search");
	addKey(Window::F5, "play_pause");
	addKey(Window::ENTER, if_equals(currentScreen, COMMAND_SCREEN), "execute_selected_command");
	addKey(Window::F3, "show_command");
	
	addKey(Window::ESCAPE, if_false(haveSearchChars), "show_main");
	addKey(Window::ESCAPE, if_true(haveSearchChars), "clear_search");
	addKey(Window::F6, "next_song");
	addKey(Window::ENTER, if_equals(currentScreen, MAIN_SCREEN), "next_song");
	addKey(Window::ENTER, if_equals(currentScreen, SEARCH_SCREEN), "play_list_song");
	addKey(Window::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), "add_list_song");
	addKey(Window::F9, if_equals(currentScreen, SEARCH_SCREEN), "add_list_song");
	addKey(Window::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), "next_composer");
	addKey(Window::F7, if_equals(currentScreen, SEARCH_SCREEN), "add_list_favorite");
	addKey(Window::F7, if_equals(currentScreen, MAIN_SCREEN), "add_current_favorite");
	addKey(Window::F8, "clear_songs");
	addKey(Window::LEFT, "prev_subtune");
	addKey(Window::RIGHT, "next_subtune");
	addKey(Window::F4, "layout_screen");
	addKey(Window::F4 | ALT, "quit");
	addKey(Window::ESCAPE | SHIFT, "quit");
	addKey('r' | CTRL, "random_shuffle");
	addKey('f' | CTRL, "format_shuffle");
	addKey('c' | CTRL, "composer_shuffle");
	addKey('s' | CTRL, "result_shuffle");
	addKey('o' | CTRL, "collection_shuffle");
	addKey('1' | CTRL, "random_shuffle");
	addKey('2' | CTRL, "collection_shuffle");
	addKey('3' | CTRL, "format_shuffle");
	addKey('4' | CTRL, "composer_shuffle");
	addKey('5' | CTRL, "result_shuffle");
	addKey('-', "volume_down");
	addKey("=+", "volume_up");
	addKey(Window::TAB, "toggle_command");
}

void ChipMachine::showScreen(int screen) {
	if(currentScreen != screen) {
		hasMoved = (screen != SEARCH_SCREEN);
		currentScreen = screen;
		grappix::Color &c = (screen == MAIN_SCREEN ? spectrumColorMain : spectrumColorSearch);
		Tween::make().to(spectrumColor, c).seconds(0.5);
		Tween::make().to(scrollEffect.alpha, 1.0).seconds(0.5);
	}
}

SongInfo ChipMachine::getSelectedSong() {
	int i = songList.selected();
	if(i < 0)
		return SongInfo();
	return MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
}

void ChipMachine::shuffleSongs(bool format, bool composer, bool collection, int limit) {
	vector<SongInfo> target;
	SongInfo match = (currentScreen == SEARCH_SCREEN) ? getSelectedSong() : dbInfo;

	LOGD("SHUFFLE %s / %s", match.composer, match.format);

	if(!format)
		match.format = "";
	if(!composer)
		match.composer = "";
	if(!collection)
		match.path = "";
	match.title = match.game;

	MusicDatabase::getInstance().getSongs(target, match, limit, true);
	player.clearSongs();
	for(const auto &s : target) {
		if(!endsWith(s.path, ".plist"))
			player.addSong(s);
	}
	showScreen(MAIN_SCREEN);
	player.nextSong();
}

void ChipMachine::updateKeys() {

	haveSearchChars = (iquery->getString().length() > 0);

	searchUpdated = false;
	auto last_selection = songList.selected();

	auto key = screen.get_key();

	if(indexingDatabase)
		return;

	uint32_t event = key;

	VerticalList *currentList = nullptr;
	if(currentScreen == SEARCH_SCREEN)
		currentList = &songList;
	else if(currentScreen == COMMAND_SCREEN)
		currentList = &commandList;

	if(key != Window::NO_KEY) {
		bool ascii = (event >= 'A' && event <= 'Z');
		if(ascii)
			event = tolower(event);
		if(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_RIGHT)) {
			if(ascii)
				event = toupper(event);
			else if(event == Window::DOWN)
				key = Window::UP;
			else
				event |= SHIFT;
		}

		if(screen.key_pressed(Window::CTRL_LEFT) || screen.key_pressed(Window::CTRL_RIGHT)) {
			if(event == Window::DOWN)
				key = Window::PAGEDOWN;
			else if(event == Window::UP)
				key = Window::PAGEUP;
			else
				event |= CTRL;
		}
		if(screen.key_pressed(Window::ALT_LEFT) || screen.key_pressed(Window::ALT_RIGHT))
			event |= ALT;

		if((event & (CTRL | SHIFT)) == 0 && currentList)
			currentList->onKey(key);

		if(event == (Window::RIGHT | SHIFT))
			event = Window::LEFT;

		lastKey = key;
		
		
		if(!smac.put_event(event)) {
			if((key >= ' ' && key <= 'z') || key == Window::LEFT || key == Window::RIGHT ||
			   key == Window::BACKSPACE || key == Window::ESCAPE || key == Window::ENTER) {
				if(currentDialog != nullptr) {
					currentDialog->on_key(event);
				} else if(currentScreen == COMMAND_SCREEN) {
					commandField.on_key(event);
					auto ctext = commandField.getText();
					if(ctext == "")
						clearCommand();
					else {
						matchingCommands.resize(commands.size());
						int j = 0;
						for(int i=0; i<commands.size(); i++) {
							if(toLower(commands[i].name).find(ctext) != string::npos)
								matchingCommands[j++] = commands[i].name;
						}
						matchingCommands.resize(j);
					}
				} else {
					currentScreen = SEARCH_SCREEN;
					if(hasMoved && event != ' ' && event != Window::BACKSPACE)
						iquery->clear();
					hasMoved = false;
					showScreen(SEARCH_SCREEN);
					if(event == Window::BACKSPACE)
						iquery->removeLast();
					else
						iquery->addLetter(tolower(event));
					searchUpdated = true;
				}
			}
		}
		while(smac.actionsLeft() > 0) {
			auto action = smac.next_action();
			LOGD("ACTION %d", action.id);
			commands[action.id].fn();
		}
	}
	if(searchUpdated) {
		searchField.setText(iquery->getString());
		searchField.visible(true);
		topStatus.visible(false);
		songList.setTotal(iquery->numHits());
		searchUpdated = false;
	}

	if(songList.selected() != last_selection && iquery->numHits() > 0) {
		int i = songList.selected();
		SongInfo song = MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
		auto ext = path_extension(song.path);
		bool isoffline = RemoteLoader::getInstance().isOffline(song.path);
		topStatus.setText(format("Format: %s (%s)%s", song.format, ext, isoffline ? "*" : ""));
		searchField.visible(false);
		topStatus.visible(true);
	}
}

} // namespace chipmachine
