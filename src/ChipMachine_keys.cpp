#include "ChipMachine.h"
#include "PlaylistDatabase.h"

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
	LOGIN,
	LAST_ACTION
};

static const uint32_t SHIFT = 0x10000;
static const uint32_t CTRL = 0x20000;

void ChipMachine::setup_rules() {

	using namespace statemachine;

	smac.add(Window::F1, if_equals(currentScreen, SEARCH_SCREEN), SHOW_MAIN);
	smac.add({ Window::F2, Window::UP, Window::DOWN, Window::PAGEUP, Window::PAGEDOWN }, SHOW_SEARCH);

	smac.add(Window::F5, PLAY_PAUSE);
	smac.add("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._0123456789 ", if_true(playlistEdit), ADD_COMMAND_CHAR);
	smac.add("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._0123456789 ", if_not_null(currentDialog), ADD_DIALOG_CHAR);
	smac.add({Window::ENTER, Window::ESCAPE, Window::BACKSPACE}, if_not_null(currentDialog), ADD_DIALOG_CHAR);

	smac.add({ Window::BACKSPACE, Window::LEFT, Window::RIGHT, Window::HOME, Window::END }, if_true(playlistEdit), ADD_COMMAND_CHAR);
	smac.add("abcdefghijklmnopqrstuvwxyz0123456789 ", ADD_SEARCH_CHAR);
	smac.add(Window::BACKSPACE, DEL_SEARCH_CHAR);
	smac.add(Window::ESCAPE, if_true(playlistEdit), CANCEL_COMMAND);
	smac.add(Window::ESCAPE, if_false(haveSearchChars), SHOW_MAIN);
	smac.add(Window::ESCAPE, if_true(haveSearchChars), CLEAR_SEARCH);
	smac.add(Window::F6, NEXT_SONG);
	smac.add(Window::ENTER, if_true(playlistEdit), SELECT_PLAYLIST);
	smac.add(Window::ENTER, if_equals(currentScreen, MAIN_SCREEN), NEXT_SONG);
	smac.add(Window::ENTER, if_equals(currentScreen, SEARCH_SCREEN), PLAY_LIST_SONG);
	smac.add(Window::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_SONG);

	smac.add(Window::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), NEXT_COMPOSER);


	smac.add(Window::F8 | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), EDIT_PLAYLIST);

	smac.add(Window::F7, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_FAVORITE);
	smac.add(Window::F7, if_equals(currentScreen, MAIN_SCREEN), ADD_CURRENT_FAVORITE);
	smac.add(Window::F8, CLEAR_SONGS);
	smac.add(Window::F9, SEND_PLAYLIST);
	smac.add(Window::LEFT, PREV_SUBTUNE);
	smac.add(Window::RIGHT, NEXT_SUBTUNE);
	smac.add(Window::F4, LAYOUT_SCREEN);

	smac.add('-', VOLUME_DOWN);
	smac.add("=+", VOLUME_UP);
}

void ChipMachine::show_main() {
	if(currentScreen != MAIN_SCREEN) {
		currentScreen = MAIN_SCREEN;
		Tween::make().to(spectrumColor, spectrumColorMain).seconds(0.5);
		Tween::make().to(scrollEffect.alpha, 1.0).seconds(0.5);
	}
}

void ChipMachine::show_search() {
	if(currentScreen != SEARCH_SCREEN) {
		currentScreen = SEARCH_SCREEN;
		Tween::make().to(spectrumColor, spectrumColorSearch).seconds(0.5);
		Tween::make().to(scrollEffect.alpha, 0.0).seconds(0.5);
	}
}

SongInfo ChipMachine::get_selected_song() {
	//auto r = MusicDatabase::getInstance().getFullString(iquery.getIndex(songList.selected() - playlists.size()));
	//auto parts = split(r, "\t");
	//SongInfo si(parts[0], "", parts[1], parts[2], parts[3]);
	//return si;
	return MusicDatabase::getInstance().getSongInfo(iquery.getIndex(songList.selected() - playlists.size()));
}


void ChipMachine::update_keys() {

	// Update some flags
	haveSearchChars = (iquery.getString().length() > 0);

	bool searchUpdated = false;
	auto last_selection = songList.selected();

	auto key = screen.get_key();

	uint32_t k = key;
	bool ascii = (k >= 'A' && k <= 'Z');
	if(ascii)
		k = tolower(k);
	if(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_RIGHT)) {
		if(ascii)
			k = toupper(k);
		else
			k |= SHIFT;
	}
	if(screen.key_pressed(Window::CTRL_LEFT) || screen.key_pressed(Window::CTRL_RIGHT))
		k |= CTRL;

	if((k & (CTRL|SHIFT)) == 0 && currentScreen == SEARCH_SCREEN)
		songList.on_key(key);

	if(key != Window::NO_KEY)
		smac.put_event(k);
	auto action = smac.next_action();
	if(action.id != NO_ACTION) {
		LOGD("ACTION %d", action.id);
		string name;
		switch((ChipAction)action.id) {
		case EDIT_PLAYLIST:
			if(songList.selected() < (int)playlists.size())
				editPlaylistName = playlists[songList.selected()];
			else
				editPlaylistName = "";
			commandField->setText(editPlaylistName);
			playlistEdit = true;
			commandField->visible(true);
			searchField->visible(false);
			topStatus->visible(false);
			break;
		case SELECT_PLAYLIST:
			currentPlaylistName = commandField->getText();
			LOGD("OLDNAME %s NEWNAME %s", editPlaylistName, currentPlaylistName);
			if(editPlaylistName == "")
				PlaylistDatabase::getInstance().createPlaylist(currentPlaylistName);
			else if(editPlaylistName != currentPlaylistName)
				PlaylistDatabase::getInstance().renamePlaylist(editPlaylistName, currentPlaylistName);

			commandField->visible(false);
			playlistEdit = false;
			playlistField->setText(currentPlaylistName);
			break;
		case ADD_COMMAND_CHAR:
			commandField->on_key(action.event);
			break;
		case ADD_DIALOG_CHAR:
			currentDialog->on_key(action.event);
			break;
		case CANCEL_COMMAND:
			commandField->visible(false);
			playlistEdit = false;		
			break;
		case PLAY_PAUSE:
			player.pause(!player.isPaused());
			if(player.isPaused()) {
				Tween::make().sine().repeating().to(timeField->add, 1.0).seconds(0.5);
			} else
				Tween::make().to(timeField->add, 0.0).seconds(0.5);
			break;
		case ADD_LIST_SONG:
			if(player.addSong(get_selected_song()))
				songList.select(songList.selected()+1);
			break;
		case PLAY_LIST_SONG:
			if(songList.selected() < (int)playlists.size()) {
				auto name = playlists[songList.selected()];
				PlaylistDatabase::getInstance().getPlaylist(name, [=](const Playlist &pl) {
					player.clearSongs();
					for(const auto &song : pl.songs) {
						player.addSong(song);
					}
					player.nextSong();
				});
			} else
				player.playSong(get_selected_song());
			show_main();
			break;
		case NEXT_COMPOSER:
			{
				string composer;
				int index = songList.selected();
				while(index < songList.size()) {
					auto res = iquery.getResult(index-playlists.size());
					auto parts = split(res, "\t");
					if(composer == "")
						composer = parts[1];
					if(parts[1] != composer)
						break;
					index++;
				}
				songList.select(index);

			}
			break;
		case NEXT_SONG:
			show_main();
			player.nextSong();
			break;
		case SHOW_MAIN:
			show_main();
			break;
		case SHOW_SEARCH:
			if(currentScreen == MAIN_SCREEN) {
				show_search();
				songList.on_key(key);
			} else {
				show_search();
			}
			searchUpdated = true;
			break;
		case ADD_SEARCH_CHAR:
			LOGD("%d %02x", currentScreen, action.event);
			if(currentScreen == MAIN_SCREEN && action.event != ' ')
				iquery.clear();
			show_search();
			iquery.addLetter(tolower(action.event));
			searchUpdated = true;
			break;
		case DEL_SEARCH_CHAR:
			show_search();
			iquery.removeLast();
			searchUpdated = true;
			break;
		case CLEAR_SEARCH:
			iquery.clear();
			searchUpdated = true;
			break;
		case ADD_CURRENT_FAVORITE:
			if(isFavorite) {
				PlaylistDatabase::getInstance().removeFromPlaylist(currentPlaylistName, currentInfo);
			} else {
				PlaylistDatabase::getInstance().addToPlaylist(currentPlaylistName, currentInfo);
			}
			isFavorite = !isFavorite;
			break;
		case ADD_LIST_FAVORITE:
			PlaylistDatabase::getInstance().addToPlaylist(currentPlaylistName, get_selected_song());
			break;
		case NEXT_SUBTUNE:
			if(currentTune < currentInfo.numtunes-1)
				player.seek(currentTune+1);
			break;
		case PREV_SUBTUNE:
			if(currentTune > 0)
				player.seek(currentTune-1);
			break;
		case CLEAR_SONGS:
			player.clearSongs();
			toast("Playlist cleared", 2);
			break;
		case SEND_PLAYLIST:
			if(userName == "") {
				currentDialog = make_shared<Dialog>(screenptr, font, "Login with handle:");
				currentDialog->on_ok([=](const string &text) {
					RemoteLists::getInstance().login(text, [=](int rc) {
						userName = text;
						if(rc)
							toast("Login successful", 2);
						File f { File::getCacheDir() + "login" };
						f.write(userName);
						f.close();
						auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
						RemoteLists::getInstance().sendList(plist.songs, plist.name, [=]() { toast("Uploaded", 2); });
					});

				});
				renderSet.add(currentDialog);
			} else {
				auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
				RemoteLists::getInstance().sendList(plist.songs, plist.name, [=]() { toast("Uploaded", 2); });
			}
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
		case NO_ACTION:
		case LOGIN:
		case LAST_ACTION:
			break;
		}
	}

	if(searchUpdated) {
		searchField->setText(iquery.getString());
		//searchField->color = searchColor;
		searchField->visible(true);
		topStatus->visible(false);
		playlists.clear();
		PlaylistDatabase::getInstance().search(iquery.getString(), playlists);
		songList.set_total(iquery.numHits() + playlists.size());
	}

	if(songList.selected() != last_selection && iquery.numHits() > 0) {
		int i = songList.selected() - playlists.size();
		if(i >= 0) {
			SongInfo song = MusicDatabase::getInstance().getSongInfo(iquery.getIndex(i));
			auto ext = path_extension(song.path);
			topStatus->setText(format("Format: %s (%s)", song.format, ext));
			//searchField->color = Color(formatColor);
		} else
			topStatus->setText("Format: Playlist");
		searchField->visible(false);
		topStatus->visible(true);
	}

}

} // namespace chipmachine

