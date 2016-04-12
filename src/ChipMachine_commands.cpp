#include "ChipMachine.h"


using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

void ChipMachine::setupCommands() {

	auto cmd = [=](const string &name, const function<void()> &f) {
		commands.emplace_back(name, f);
	};
	
	cmd("test_dialog", [=]() { 
		currentDialog = make_shared<Dialog>(screenptr, font, "Type something:");
		overlay.add(currentDialog);
	});
	
	cmd("show_main", [=]() { showScreen(MAIN_SCREEN); });
	
	
	cmd("close_dialog", [=]() { 
		if(currentDialog)
			currentDialog->remove();
		currentDialog = nullptr;
	});

	commands.emplace_back("show_search", [=]() {
		if(currentScreen != SEARCH_SCREEN) {
			showScreen(SEARCH_SCREEN);
			songList.onKey((grappix::Window::key)lastKey);
		} else {
			showScreen(SEARCH_SCREEN);
		}
		searchUpdated = true;
	});

	commands.emplace_back("show_command", [=]() {
		if(currentScreen != COMMAND_SCREEN)
			lastScreen = currentScreen;
		showScreen(COMMAND_SCREEN);
	});

	commands.emplace_back("toggle_command", [=]() {
		if(currentScreen != COMMAND_SCREEN) {
			lastScreen = currentScreen;
			showScreen(COMMAND_SCREEN);
		} else
			showScreen(lastScreen);
	});
	
	commands.emplace_back("play_pause", [=]() {
		player.pause(!player.isPaused());
		if(player.isPaused()) {
			Tween::make().sine().repeating().to(timeField.add, 1.0).seconds(0.5);
		} else
			Tween::make().to(timeField.add, 0.0).seconds(0.5);
	});

	commands.emplace_back("enque_song", [=]() {
		if(player.addSong(getSelectedSong()))
			songList.select(songList.selected() + 1);
	});

	commands.emplace_back("add_current_favorite", [=]() {
		auto song = dbInfo;
		if(currentTune != song.starttune)
			song.starttune = currentTune;
		else
			song.starttune = -1;
		if(isFavorite) {
			MusicDatabase::getInstance().removeFromPlaylist(currentPlaylistName, song);
		} else {
			MusicDatabase::getInstance().addToPlaylist(currentPlaylistName, song);
		}
		isFavorite = !isFavorite;
		uint32_t alpha = isFavorite ? 0xff : 0x00;
		Tween::make().to(favIcon.color, Color(favColor | (alpha << 24))).seconds(0.25);
		//favIcon.visible(isFavorite);
	});

	commands.emplace_back("add_list_favorite", [=]() {
		MusicDatabase::getInstance().addToPlaylist(currentPlaylistName, getSelectedSong());
	});
	
	commands.emplace_back("clear_filter", [=]() {
		filter = "";
		searchUpdated = true;
	});

	commands.emplace_back("set_collection_filter", [=]() {
		
		const auto &song = getSelectedSong();        
		auto p = split(song.path, "::");
		if(p.size() < 2)
			return;
		filter = p[0];
		searchUpdated = true;
			
	});
		
	commands.emplace_back("play_song", [=]() {
		player.playSong(getSelectedSong());
		showScreen(MAIN_SCREEN);
	});

	commands.emplace_back("next_composer", [=]() {
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
	});

	commands.emplace_back("next_song", [=]() {
		showScreen(MAIN_SCREEN);
		player.nextSong();
	});

	commands.emplace_back("clear_search", [=]() {
		//iquery->clear();
		searchField.setText("");
		searchUpdated = true;
	});

	commands.emplace_back("execute_selected_command", [=]() {
		int i = commandList.selected();
		commandList.select(-1);
		showScreen(lastScreen);
		auto it = std::find(commands.begin(), commands.end(), *matchingCommands[i]);
		if(it != commands.end())
			it->fn();
	});

	commands.emplace_back("next_subtune", [=]() {
		if(currentInfo.numtunes == 0)
			player.seek(-1, player.getPosition() + 10);
		else if(currentTune < currentInfo.numtunes - 1)
			player.seek(currentTune + 1);
	});

	commands.emplace_back("prev_subtune", [=]() {
		if(currentInfo.numtunes == 0)
			player.seek(-1, player.getPosition() - 10);
		else if(currentTune > 0)
			player.seek(currentTune - 1);
	});

	commands.emplace_back("clear_songs", [=]() {
		player.clearSongs();
		toast("Playlist cleared", 2);
	});

	commands.emplace_back("volume_up", [=]() {
		player.setVolume(player.getVolume() + 0.1);
		showVolume = 30;
	});

	commands.emplace_back("volume_down", [=]() {
		player.setVolume(player.getVolume() - 0.1);
		showVolume = 30;
	});

	commands.emplace_back("layout_screen", [=]() { layoutScreen(); });

	commands.emplace_back("quit", [=]() { screen.close(); });

	commands.emplace_back("random_shuffle", [=]() {
		toast("Random shuffle!", 2);
		shuffleSongs(false, false, false, 100);
	});

	commands.emplace_back("composer_shuffle", [=]() {
		toast("Composer shuffle!", 2);
		shuffleSongs(false, true, false, 1000);
	});

	commands.emplace_back("format_shuffle", [=]() {
		toast("Format shuffle!", 2);
		shuffleSongs(true, false, false, 100);
	});

	commands.emplace_back("collection_shuffle", [=]() {
		toast("Collection shuffle!", 2);
		shuffleSongs(false, false, true, 100);
	});

	commands.emplace_back("result_shuffle", [=]() {
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
	});
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
				auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
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
}
}
