#include "ChipMachine.h"

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

enum ChipAction {
	NO_ACTION,
	NEXT_SUBTUNE,
	PREV_SUBTUNE,
	PLAY_PAUSE,
	SHOW_MAIN,
	SHOW_SEARCH,
	SHOW_COMMAND,
	ADD_SEARCH_CHAR,
	DEL_SEARCH_CHAR,
	CLEAR_SEARCH,
	NEXT_SONG,
	ADD_LIST_SONG,
	PLAY_LIST_SONG,
	ADD_LIST_FAVORITE,
	ADD_CURRENT_FAVORITE,
	CLEAR_SONGS,
	SELECT_PLAYLIST,
	EDIT_PLAYLIST,
	ADD_COMMAND_CHAR,
	ADD_DIALOG_CHAR,
	CANCEL_COMMAND,
	SEND_PLAYLIST,
	NEXT_COMPOSER,
	LAYOUT_SCREEN,
	VOLUME_UP,
	VOLUME_DOWN,
	QUIT,
	LOGIN,
	DUMP_FAVORITES,
	RANDOM_SHUFFLE,
	FORMAT_SHUFFLE,
	COLLECTION_SHUFFLE,
	COMPOSER_SHUFFLE,
	RESULT_SHUFFLE,
	LAST_ACTION
};

static const uint32_t SHIFT = 0x10000;
static const uint32_t CTRL = 0x20000;
static const uint32_t ALT = 0x40000;

void ChipMachine::setupRules() {

	using namespace statemachine;

	smac.add(Window::F1, SHOW_MAIN);
	smac.add({Window::F2, Window::UP, Window::DOWN, Window::PAGEUP, Window::PAGEDOWN}, SHOW_SEARCH);
	smac.add(Window::F5, PLAY_PAUSE);
	smac.add("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._0123456789 ",
	         if_equals(currentScreen, COMMAND_SCREEN), ADD_COMMAND_CHAR);
	smac.add("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._0123456789 ",
	         if_not_null(currentDialog), ADD_DIALOG_CHAR);
	smac.add({Window::ENTER, Window::ESCAPE, Window::BACKSPACE}, if_not_null(currentDialog),
	         ADD_DIALOG_CHAR);
	smac.add({Window::BACKSPACE, Window::LEFT, Window::RIGHT, Window::HOME, Window::END},
	         if_equals(currentScreen, COMMAND_SCREEN), ADD_COMMAND_CHAR);
	smac.add("abcdefghijklmnopqrstuvwxyz0123456789 ", ADD_SEARCH_CHAR);
	smac.add(Window::BACKSPACE, DEL_SEARCH_CHAR);
	smac.add(Window::ESCAPE, if_true(playlistEdit), CANCEL_COMMAND);
	smac.add({Window::TAB, Window::F3}, SHOW_COMMAND);
	smac.add(Window::ESCAPE, if_false(haveSearchChars), SHOW_MAIN);
	smac.add(Window::ESCAPE, if_true(haveSearchChars), CLEAR_SEARCH);
	smac.add(Window::F6, NEXT_SONG);
	smac.add(Window::ENTER, if_true(playlistEdit), SELECT_PLAYLIST);
	smac.add(Window::ENTER, if_equals(currentScreen, MAIN_SCREEN), NEXT_SONG);
	smac.add(Window::ENTER, if_equals(currentScreen, SEARCH_SCREEN), PLAY_LIST_SONG);
	smac.add(Window::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_SONG);
	smac.add(Window::F9, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_SONG);
	smac.add(Window::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), NEXT_COMPOSER);
	smac.add(Window::F8 | SHIFT, DUMP_FAVORITES);
	smac.add(Window::F7, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_FAVORITE);
	smac.add(Window::F7, if_equals(currentScreen, MAIN_SCREEN), ADD_CURRENT_FAVORITE);
	smac.add(Window::F8, CLEAR_SONGS);
	// smac.add(Window::F9, SEND_PLAYLIST);
	smac.add(Window::LEFT, PREV_SUBTUNE);
	smac.add(Window::RIGHT, NEXT_SUBTUNE);
	smac.add(Window::F4, LAYOUT_SCREEN);
	smac.add(Window::F4 | ALT, QUIT);
	smac.add(Window::ESCAPE | SHIFT, QUIT);
	smac.add('r' | CTRL, RANDOM_SHUFFLE);
	smac.add('f' | CTRL, FORMAT_SHUFFLE);
	smac.add('c' | CTRL, COMPOSER_SHUFFLE);
	smac.add('s' | CTRL, RESULT_SHUFFLE);
	smac.add('o' | CTRL, COLLECTION_SHUFFLE);
	smac.add('1' | CTRL, RANDOM_SHUFFLE);
	smac.add('2' | CTRL, COLLECTION_SHUFFLE);
	smac.add('3' | CTRL, FORMAT_SHUFFLE);
	smac.add('4' | CTRL, COMPOSER_SHUFFLE);
	smac.add('5' | CTRL, RESULT_SHUFFLE);
	smac.add('-', VOLUME_DOWN);
	smac.add("=+", VOLUME_UP);
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
	SongInfo match = (currentScreen == SEARCH_SCREEN)
	                     ? getSelectedSong()
	                     : MusicDatabase::getInstance().lookup(currentInfo.path);

	LOGD("SHUFFLE %s", match.path);

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

	bool searchUpdated = false;
	auto last_selection = songList.selected();

	auto key = screen.get_key();

	if(indexingDatabase)
		return;

	uint32_t event = key;

	VerticalList *currentList = nullptr;
	if(currentScreen == SEARCH_SCREEN)
		currentList = &songList;
	else if(currentScreen == MAIN_SCREEN)
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

		smac.put_event(event);
	}

	auto &pdb = MusicDatabase::getInstance();

	auto action = smac.next_action();
	if(action.id != NO_ACTION) {


		auto pos = player.getPosition();
		//int length = player.getLength();

		// LOGD("ACTION %d", action.id);
		string name;
		switch((ChipAction)action.id) {
		/*  case EDIT_PLAYLIST:
			if(songList.selected() < (int)playlists.size())
				editPlaylistName = playlists[songList.selected()];
			else
				editPlaylistName = "";
			commandField.setText(editPlaylistName);
			playlistEdit = true;
			commandField.visible(true);
			searchField.visible(false);
			topStatus.visible(false);
			break;
		case SELECT_PLAYLIST:
			currentPlaylistName = commandField.getText();
			LOGD("OLDNAME %s NEWNAME %s", editPlaylistName, currentPlaylistName);
			if(editPlaylistName == "")
				PlaylistDatabase::getInstance().createPlaylist(currentPlaylistName);
			else if(editPlaylistName != currentPlaylistName)
				PlaylistDatabase::getInstance().renamePlaylist(editPlaylistName,
				                                               currentPlaylistName);

			commandField.visible(false);
			playlistEdit = false;
			playlistField.setText(currentPlaylistName);
			break;*/
		case ADD_COMMAND_CHAR:
			commandField.on_key(action.event);
			break;
		case ADD_DIALOG_CHAR:
			currentDialog->on_key(action.event);
			break;
		case CANCEL_COMMAND:
			commandField.visible(false);
			playlistEdit = false;
			break;
		case PLAY_PAUSE:
			player.pause(!player.isPaused());
			if(player.isPaused()) {
				Tween::make().sine().repeating().to(timeField.add, 1.0).seconds(0.5);
			} else
				Tween::make().to(timeField.add, 0.0).seconds(0.5);
			break;
		case ADD_LIST_SONG:
			if(player.addSong(getSelectedSong()))
				songList.select(songList.selected() + 1);
			break;
		case PLAY_LIST_SONG:
			/*  if(songList.selected() < (int)playlists.size()) {
				auto name = playlists[songList.selected()];
				PlaylistDatabase::getInstance().getPlaylist(name, [=](const Playlist &pl) {
					player.clearSongs();
					for(const auto &song : pl.songs) {
						player.addSong(song);
					}
					player.nextSong();
				});
			} else */
				player.playSong(getSelectedSong());
			showScreen(MAIN_SCREEN);
			break;
		case NEXT_COMPOSER: {
			string composer;
			int index = songList.selected();
			while(index < songList.size()) {
				auto res = iquery->getResult(index);
				auto parts = split(res, "\t");
				if(composer == "")
					composer = parts[1];
				if(parts[1] != composer)
					break;
				index++;
			}
			songList.select(index);

		} break;
		case NEXT_SONG:
			showScreen(MAIN_SCREEN);
			player.nextSong();
			break;
		case SHOW_MAIN:
			showScreen(MAIN_SCREEN);
			break;
		case SHOW_COMMAND:
			showScreen(COMMAND_SCREEN);
			break;
		case SHOW_SEARCH:
			if(currentScreen == MAIN_SCREEN) {
				showScreen(SEARCH_SCREEN);
				songList.onKey(key);
			} else {
				showScreen(SEARCH_SCREEN);
			}
			searchUpdated = true;
			break;
		case ADD_SEARCH_CHAR:
			// LOGD("%d %02x", currentScreen, action.event);
			if(hasMoved && action.event != ' ')
				iquery->clear();
			hasMoved = false;
			showScreen(SEARCH_SCREEN);
			iquery->addLetter(tolower(action.event));
			searchUpdated = true;
			break;
		case DEL_SEARCH_CHAR:
			hasMoved = false;
			showScreen(SEARCH_SCREEN);
			iquery->removeLast();
			searchUpdated = true;
			break;
		case CLEAR_SEARCH:
			iquery->clear();
			searchUpdated = true;
			break;
		case ADD_CURRENT_FAVORITE:
			if(isFavorite) {
				pdb.removeFromPlaylist(currentPlaylistName, currentInfo);
			} else {
				pdb.addToPlaylist(currentPlaylistName, currentInfo);
			}
			isFavorite = !isFavorite;
			favIcon.visible(isFavorite);
			break;
		case ADD_LIST_FAVORITE:
			pdb.addToPlaylist(currentPlaylistName, getSelectedSong());
			break;
		//case DUMP_FAVORITES:
		//	pdb.dumpPlaylist(currentPlaylistName, "playlists");
		//	break;
		case NEXT_SUBTUNE:
			if(currentInfo.numtunes == 0)
				player.seek(-1, pos + 10);
			else
			if(currentTune < currentInfo.numtunes - 1)
				player.seek(currentTune + 1);
			break;
		case PREV_SUBTUNE:
			if(currentInfo.numtunes == 0)
				player.seek(-1, pos - 10);
			else	
			if(currentTune > 0)
				player.seek(currentTune - 1);
			break;
		case CLEAR_SONGS:
			player.clearSongs();
			toast("Playlist cleared", 2);
			break;
		case SEND_PLAYLIST:
#ifdef USE_REMOTELISTS
			if(userName == "") {
				currentDialog = make_shared<Dialog>(screenptr, font, "Login with handle:");
				currentDialog->on_ok([=](const string &text) {
					RemoteLists::getInstance().login(text, [=](int rc) {
						userName = text;
						if(rc)
							toast("Login successful", 2);
						File f{File::getCacheDir() + "login"};
						f.write(userName);
						f.close();
						auto plist =
						    PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
						RemoteLists::getInstance().sendList(plist.songs, plist.name,
						                                    [=]() { toast("Uploaded", 2); });
					});

				});
				renderSet.add(currentDialog);
			} else {
				auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
				RemoteLists::getInstance().sendList(plist.songs, plist.name,
				                                    [=]() { toast("Uploaded", 2); });
			}
#endif
			break;
		case VOLUME_UP:
			player.setVolume(player.getVolume() + 0.1);
			showVolume = 30;
			break;
		case VOLUME_DOWN:
			player.setVolume(player.getVolume() - 0.1);
			showVolume = 30;
			break;
		case LAYOUT_SCREEN:
			layoutScreen();
			break;
		case QUIT:
			screen.close();
			break;
		case RANDOM_SHUFFLE:
			toast("Random shuffle!", 2);
			shuffleSongs(false, false, false, 100);
			break;
		case COMPOSER_SHUFFLE:
			toast("Composer shuffle!", 2);
			shuffleSongs(false, true, false, 1000);
			break;
		case FORMAT_SHUFFLE:
			toast("Format shuffle!", 2);
			shuffleSongs(true, false, false, 100);
			break;
		case COLLECTION_SHUFFLE:
			toast("Collection shuffle!", 2);
			shuffleSongs(false, false, true, 100);
			break;
		case RESULT_SHUFFLE:
			toast("Result shuffle!", 2);
			player.clearSongs();
			for(int i = 0; i < iquery->numHits(); i++) {
				auto res = iquery->getResult(i);
				LOGD("%s", res);
				auto parts = split(res, "\t");

				int f = atoi(parts[3].c_str()) & 0xff;
				if(f == PLAYLIST)
					continue;

				SongInfo song;
				song.title = parts[0];
				song.composer = parts[1];
				song.path = "index::" + parts[2];
				player.addSong(song, true);
			}
			showScreen(MAIN_SCREEN);
			player.nextSong();
			break;
		case NO_ACTION:
		case LOGIN:
		case LAST_ACTION:
			break;
		}
	}

	if(searchUpdated) {
		searchField.setText(iquery->getString());
		// searchField->color = searchColor;
		searchField.visible(true);
		topStatus.visible(false);
		//PlaylistDatabase::getInstance().search(iquery->getString(), playlists);
		songList.setTotal(iquery->numHits());
	}

	if(songList.selected() != last_selection && iquery->numHits() > 0) {
		int i = songList.selected();
		SongInfo song = MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
		auto ext = path_extension(song.path);
		bool isoffline = RemoteLoader::getInstance().isOffline(song.path);
		topStatus.setText(format("Format: %s (%s)%s", song.format, ext, isoffline ? "*" : ""));
		// searchField->color = Color(formatColor);
		searchField.visible(false);
		topStatus.visible(true);
	}
}

} // namespace chipmachine
