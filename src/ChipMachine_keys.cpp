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
	LAST_ACTION
};

static const uint32_t SHIFT = 0x10000;
static const uint32_t CTRL = 0x20000;

void ChipMachine::setup_rules() {

	using namespace statemachine;

	smac.add(Window::F1, if_equals(currentScreen, SEARCH_SCREEN), SHOW_MAIN);
	smac.add({ Window::F2, Window::BACKSPACE, Window::UP, Window::DOWN, Window::PAGEUP, Window::PAGEDOWN }, SHOW_SEARCH);

	smac.add(Window::F5, if_equals(currentScreen, MAIN_SCREEN), PLAY_PAUSE);
	smac.add("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ", ADD_SEARCH_CHAR);
	smac.add(Window::BACKSPACE, DEL_SEARCH_CHAR);
	smac.add(Window::ESCAPE, if_false(haveSearchChars), SHOW_MAIN);
	smac.add(Window::ESCAPE, if_true(haveSearchChars), CLEAR_SEARCH);
	smac.add(Window::F6, NEXT_SONG);
	smac.add(Window::ENTER, if_equals(currentScreen, MAIN_SCREEN), NEXT_SONG);
	smac.add(Window::ENTER, if_equals(currentScreen, SEARCH_SCREEN), PLAY_LIST_SONG);
	smac.add(Window::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_SONG);

	smac.add(Window::F7, if_equals(currentScreen, SEARCH_SCREEN), ADD_LIST_FAVORITE);
	smac.add(Window::F7, if_equals(currentScreen, MAIN_SCREEN), ADD_CURRENT_FAVORITE);
	smac.add(Window::F8, CLEAR_SONGS);
	smac.add(Window::LEFT, PREV_SUBTUNE);
	smac.add(Window::RIGHT, NEXT_SUBTUNE);

}

void ChipMachine::show_main() {
	if(currentScreen != MAIN_SCREEN) {
		currentScreen = MAIN_SCREEN;
		make_tween().to(spectrumColor, spectrumColorMain).seconds(0.5);
		make_tween().to(scrollEffect.alpha, 1.0).seconds(0.5);
	}
}

void ChipMachine::show_search() {
	if(currentScreen != SEARCH_SCREEN) {
		currentScreen = SEARCH_SCREEN;
		make_tween().to(spectrumColor, spectrumColorSearch).seconds(0.5);
		make_tween().to(scrollEffect.alpha, 0.0).seconds(0.5);
	}
}

SongInfo ChipMachine::get_selected_song() {
	auto r = iquery.getFull(songList.selected() - playlists.size());
	auto parts = split(r, "\t");
	SongInfo si(parts[0], "", parts[1], parts[2], parts[3]);
	return si;
}


void ChipMachine::update_keys() {

	// Update some flags
	onMainScreen = (currentScreen == MAIN_SCREEN);
	onSearchScreen = (currentScreen == SEARCH_SCREEN);
	haveSearchChars = (iquery.getString().length() > 0);

	bool searchUpdated = false;
	auto last_selection = songList.selected();

	auto key = screen.get_key();
	//if(key == Window::NO_KEY)
	//	return;

	if(currentScreen == SEARCH_SCREEN)
		songList.on_key(key);

	uint32_t k = key;

	if(screen.key_pressed(Window::SHIFT_LEFT) || screen.key_pressed(Window::SHIFT_RIGHT))
		k |= SHIFT;
	if(screen.key_pressed(Window::CTRL_LEFT) || screen.key_pressed(Window::CTRL_RIGHT))
		k |= CTRL;

	if(key != Window::NO_KEY)
		smac.put_event(k);
	auto action = smac.next_action();

	switch((ChipAction)action.id) {
	case PLAY_PAUSE:
		player.pause(!player.isPaused());
		if(player.isPaused()) {
			make_tween().sine().repeating().to(timeField->add, 1.0).seconds(0.5);
		} else
			make_tween().to(timeField->add, 0.0).seconds(0.5);
		break;
	case ADD_LIST_SONG:
		if(player.addSong(get_selected_song()))
			songList.select(songList.selected()+1);
		break;
	case PLAY_LIST_SONG:
		if(songList.selected() < playlists.size()) {
			auto name = playlists[songList.selected()];
			auto pl = PlaylistDatabase::getInstance().getPlaylist(name);
			player.clearSongs();
			for(const auto &song : pl.songs) {
				player.addSong(song);
			}
			player.nextSong();
		} else
			player.playSong(get_selected_song());
		show_main();
		break;
	case NEXT_SONG:
		show_main();
		player.nextSong();
		break;
	case SHOW_MAIN:
		show_main();
		break;
	case SHOW_SEARCH:
		show_search();
		break;
	case ADD_SEARCH_CHAR:
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
			PlaylistDatabase::getInstance().removeFromPlaylist("Favorites", currentInfo);
		} else {
			PlaylistDatabase::getInstance().addToPlaylist("Favorites", currentInfo);
		}
		isFavorite = !isFavorite;
		break;
	case ADD_LIST_FAVORITE:
		PlaylistDatabase::getInstance().addToPlaylist("Favorites", get_selected_song());
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
	}

	if(searchUpdated) {
		searchField->setText("#" + iquery.getString());
		searchField->color = searchColor;
		playlists.clear();
		PlaylistDatabase::getInstance().search(iquery.getString(), playlists);
		songList.set_total(iquery.numHits() + playlists.size());
	}

	if(songList.selected() != last_selection && iquery.numHits() > 0) {
		auto p = iquery.getFull(songList.selected() - playlists.size());
		auto parts = split(p, "\t");
		auto ext = path_extension(parts[0]);
		searchField->setText(format("Format: %s (%s)", parts[3], ext));
		searchField->color = Color(formatColor);
	}

}

} // namespace chipmachine

