#include "ChipMachine.h"

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

void ChipMachine::addKey(uint32_t key, statemachine::Condition cond, const std::string &cmd) {

	auto screen = currentScreen;
	bool onMain = false;
	bool onSearch = false;

	// Figure out if the condition checks for main or search screen, used for printing
	// info about the key combination below
	currentScreen = NO_SCREEN;
	if(!cond.check()) {
		currentScreen = MAIN_SCREEN;
		onMain = cond.check();
		currentScreen = SEARCH_SCREEN;
		onSearch = cond.check();
	}
	currentScreen = screen;

	auto it = std::find(commands.begin(), commands.end(), cmd);
	if(it != commands.end()) {
		smac.add(key, cond, static_cast<uint32_t>(std::distance(commands.begin(), it)));
		if(key == grappix::Window::BACKSPACE)
			return;
		// If this is the first key we map to this command, give it a name
		if(it->shortcut == "") {
			std::string name;
			if(key & SHIFT)
				name += "shift+";
			if(key & ALT)
				name += "alt+";
			if(key & CTRL)
				name += "ctrl+";
			key &= 0xffff;
			if(key < 0x100)
				name.append(1, tolower(key));
			else if(key <= grappix::Window::F12)
				name += utils::toLower(key_names[key - 0x100]);
			if(onSearch)
				name += " [search]";
			if(onMain)
				name += " [main]";
			it->shortcut = name;
		}
	}
}

void ChipMachine::setupRules() {

	using namespace statemachine;

	addKey(Window::F1, "show_main");
	addKey(Window::F2, "show_search");
	addKey({Window::UP, Window::DOWN, Window::PAGEUP, Window::PAGEDOWN},
	       if_equals(currentScreen, MAIN_SCREEN), "show_search");
	addKey(Window::F5, "play_pause");
	addKey(Window::F3, "show_command");

	addKey(Window::BACKSPACE, if_equals(currentScreen, SEARCH_SCREEN) && if_null(currentDialog) &&
	                              if_false(haveSearchChars),
	       "clear_filter");

	addKey(Window::ESCAPE, if_not_null(currentDialog), "close_dialog");

	addKey(Window::ESCAPE, if_false(haveSearchChars), "show_main");
	addKey(Window::ESCAPE, if_true(haveSearchChars), "clear_search");
	addKey(Window::F6, "next_song");
	addKey(Window::ENTER, if_equals(currentScreen, MAIN_SCREEN), "next_song");
	addKey(Window::ENTER, if_equals(currentScreen, SEARCH_SCREEN), "play_song");
	addKey(Window::ENTER, if_equals(currentScreen, COMMAND_SCREEN), "execute_selected_command");
	addKey(Window::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), "enque_song");
	addKey(Window::F9, if_equals(currentScreen, SEARCH_SCREEN), "enque_song");
	addKey(Window::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), "next_composer");
	addKey(Window::F7, if_equals(currentScreen, SEARCH_SCREEN), "add_list_favorite");
	addKey(Window::F7, if_equals(currentScreen, MAIN_SCREEN), "add_current_favorite");
	addKey(Window::F8, "clear_songs");
	addKey(Window::LEFT, if_not_equals(currentScreen, COMMAND_SCREEN) && if_null(currentDialog),
	       "prev_subtune");
	addKey(Window::RIGHT, if_not_equals(currentScreen, COMMAND_SCREEN) && if_null(currentDialog),
	       "next_subtune");
	addKey(Window::F4, "layout_screen");
	addKey(Window::ESCAPE | SHIFT, "quit");
	addKey(Window::F4 | ALT, "quit");

	addKey('d' | CTRL, "download_current");
	addKey('z' | CTRL, "next_screenshot");

	addKey('r' | CTRL, "random_shuffle");
	addKey('f' | CTRL, "format_shuffle");
	addKey('c' | CTRL, "composer_shuffle");
	addKey('s' | CTRL, "result_shuffle");
	addKey('o' | CTRL, "collection_shuffle");
	addKey('-', "volume_down");
	addKey({'+', '='}, "volume_up");
	addKey(Window::TAB, "toggle_command");
	std::string empty("");
	addKey('i' | CTRL, if_equals(filter, empty), "set_collection_filter");
	addKey('i' | CTRL, if_not_equals(filter, empty), "clear_filter");
}

void ChipMachine::showScreen(Screen screen) {
	if(currentScreen != screen) {
		hasLeftSearch = (screen != SEARCH_SCREEN);
		currentScreen = screen;
		if(screen == MAIN_SCREEN) {
			Tween::make().to(spectrumColor, spectrumColorMain).seconds(0.5);
			Tween::make().to(scrollEffect.alpha, 1.0).seconds(0.5);
		} else {
			Tween::make().to(spectrumColor, spectrumColorSearch).seconds(0.5);
			Tween::make().to(scrollEffect.alpha, 0.0).seconds(0.5);
		}
	}
}

SongInfo ChipMachine::getSelectedSong() {
	int i = songList.selected();
	if(i < 0)
		return SongInfo();
	return MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
}

void ChipMachine::shuffleSongs(int what, int limit) {
	vector<SongInfo> target;
	SongInfo match = (currentScreen == SEARCH_SCREEN) ? getSelectedSong() : dbInfo;

	LOGD("SHUFFLE %s / %s", match.composer, match.format);

	if((what & SHUFFLE_FORMAT) == 0)
		match.format = "";
	if((what & SHUFFLE_COMPOSER) == 0)
		match.composer = "";
	if((what & SHUFFLE_FORMAT) == 0)
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
						for(int i = 0; i < commands.size(); i++) {
							if(toLower(commands[i].name).find(ctext) != string::npos)
								matchingCommands[j++] = &commands[i];
						}
						matchingCommands.resize(j);
					}
					commandList.setTotal(matchingCommands.size());
				} else {
					if(hasLeftSearch && event != ' ' && event != Window::BACKSPACE)
						searchField.setText("");
					hasLeftSearch = false;
					showScreen(SEARCH_SCREEN);
					if(event >= 0x20 && event <= 0xff)
						event = tolower(event);
					searchField.on_key(event);
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

	if(songList.selected() != last_selection && iquery->numHits() > 0) {
		int i = songList.selected();
		SongInfo song = MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
		auto ext = path_extension(song.path);
		bool isoffline = RemoteLoader::getInstance().isOffline(song.path);
		topStatus.setText(format("Format: %s (%s)%s", song.format, ext, isoffline ? "*" : ""));
		searchField.visible(false);
		filterField.visible(false);
		topStatus.visible(true);
	}

	if(searchUpdated) {
		auto s = searchField.getText();
		if(s[0] == '\\') {
			int pos = s.find(' ');
			if(pos != string::npos) {
				auto f = s.substr(1, pos - 1);
				if(f != filter) {
					filter = f;
					s = s.substr(pos + 1);
					searchField.setText(s);
				}
			}
		}

		if(filter != filterField.getText()) {
			LOGD("Filter now %s", filter);
			filterField.setText(filter);
			MusicDatabase::getInstance().setFilter(filter);
			iquery->invalidate();
		}

		iquery->setString(s);
		searchField.visible(true);
		filterField.visible(true);
		searchField.pos.x = filterField.pos.x + filterField.getWidth() + 5;
		LOGD("X NOW %d", songField.pos.x);
		topStatus.visible(false);
		songList.setTotal(iquery->numHits());
		searchUpdated = false;
	}
}

} // namespace chipmachine
