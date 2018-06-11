#include "modutils.h"
#include "ChipMachine.h"

#include <coreutils/environment.h>

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

void ChipMachine::setupCommands() {

	auto cmd = [=](const string &name, const function<void()> &f) {
		commands.emplace_back(name, f);
	};

	cmd("show_main", [=] { showScreen(MAIN_SCREEN); });

	cmd("show_search", [=]() {
		if(currentScreen != SEARCH_SCREEN) {
			showScreen(SEARCH_SCREEN);
			songList.onKey(lastKey);
		} else {
			showScreen(SEARCH_SCREEN);
		}
		searchUpdated = true;
	});

	cmd("show_command", [=] {
		if(currentScreen != COMMAND_SCREEN)
			lastScreen = currentScreen;
		showScreen(COMMAND_SCREEN);
	});

	cmd("toggle_command", [=] {
		if(currentScreen != COMMAND_SCREEN) {
			lastScreen = currentScreen;
			showScreen(COMMAND_SCREEN);
		} else
			showScreen(lastScreen);
	});

	cmd("download_current", [=] {
		auto target = Environment::getHomeDir() / "Downloads";
		makedir(target);

		auto files = player.getSongFiles();
		if(files.size() == 0)
			return;
		for(const auto &fromFile : files) {
            fs::path from = fromFile.getName();
			string fileName;
			string title = currentInfo.title;
			string composer = currentInfo.composer;
			if(composer == "" || composer == "?")
				composer = "Unknown";
			if(title == "")
				title = currentInfo.game;
			auto ext = path_extension(from);
			if(title == "" || endsWith(ext, "lib"))
				fileName = from.string();
			else
				fileName = format("%s - %s.%s", composer, title, ext);
			auto to = target / fileName;
			LOGD("Downloading to '%s'", to.string());
			std::error_code ec;
			fs::copy(from, to, ec);
			if(ec) {
				to = target / from.filename();
				fs::copy(from, to, ec);
			}
		}
		toast("Downloaded file");
	});

	cmd("play_pause", [=] {
		player.pause(!player.isPaused());
		if(player.isPaused()) {
			Tween::make().sine().repeating().to(timeField.add, 1.0).seconds(0.5);
		} else
			Tween::make().to(timeField.add, 0.0).seconds(0.5);
	});

	cmd("enque_song", [=] {
		if(haveSelection()) {
			player.addSong(getSelectedSong());
			songList.select(songList.selected() + 1);
		}
	});

	cmd("next_screenshot", [=] {
		nextScreenshot();
	});
	
	cmd("add_current_favorite", [=] {
		auto song = dbInfo;
		//if(currentTune != song.starttune)
			song.starttune = currentTune;
		//else
		//	song.starttune = -1;
		if(isFavorite) {
			MusicDatabase::getInstance().removeFromPlaylist(currentPlaylistName, song);
		} else {
			MusicDatabase::getInstance().addToPlaylist(currentPlaylistName, song);
		}
		isFavorite = !isFavorite;
		uint32_t alpha = isFavorite ? 0xff : 0x00;
		Tween::make().to(favIcon.color, Color(favColor | (alpha << 24))).seconds(0.25);
		// favIcon.visible(isFavorite);
	});

	cmd("add_list_favorite", [=] {
		if(haveSelection())
			MusicDatabase::getInstance().addToPlaylist(currentPlaylistName, getSelectedSong());
	});

	cmd("clear_filter", [=] {
		filter = "";
		searchUpdated = true;
	});

	cmd("set_collection_filter", [=] {

		const auto &song = getSelectedSong();
		auto p = split(song.path, "::");
		if(p.size() < 2)
			return;
		filter = p[0];
		searchUpdated = true;

	});

	cmd("play_song", [=] {
		if(haveSelection()) {
			player.playSong(getSelectedSong());
			showScreen(MAIN_SCREEN);
		}
	});

	cmd("next_composer", [=] {
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

	cmd("next_song", [=] {
		showScreen(MAIN_SCREEN);
		player.nextSong();
	});

	cmd("clear_search", [=] {
		if(searchField.getText() == "")
			showScreen(MAIN_SCREEN);
		else {
			searchField.setText("");
			searchUpdated = true;
		}
	});

	cmd("clear_command", [=] {
		LOGD("CMD %s", commandField.getText());
		if(commandField.getText() == "")
			showScreen(MAIN_SCREEN);
		else {
			commandField.setText("");
			clearCommand();
			commandList.setTotal(matchingCommands.size());
		}
	});
	
	cmd("execute_selected_command", [=] {
		int i = commandList.selected();
		if(matchingCommands.size() == 0)
			return;
		commandList.select(-1);
		showScreen(lastScreen);
		auto it = std::find(commands.begin(), commands.end(), *matchingCommands[i]);
		if(it != commands.end())
			it->fn();
	});

	cmd("next_subtune", [=] {
		if(currentInfo.numtunes == 0)
			player.seek(-1, player.getPosition() + 10);
		else if(currentTune < currentInfo.numtunes - 1)
			player.seek(currentTune + 1);
	});

	cmd("prev_subtune", [=] {
		if(currentInfo.numtunes == 0)
			player.seek(-1, player.getPosition() - 10);
		else if(currentTune > 0)
			player.seek(currentTune - 1);
	});

	cmd("clear_songs", [=] {
		player.clearSongs();
		toast("Playlist cleared");
	});

	cmd("volume_up", [=] {
		player.setVolume(player.getVolume() + 0.1);
		showVolume = 30;
	});

	cmd("volume_down", [=] {
		player.setVolume(player.getVolume() - 0.1);
		showVolume = 30;
	});

	cmd("layout_screen", [=] { layoutScreen(); });

	cmd("quit", [=] { screen.close(); });

	cmd("random_shuffle", [=] {
		toast("Random shuffle!");
		shuffleSongs(false, false, false, 100);
	});

	cmd("composer_shuffle", [=] {
		toast("Composer shuffle!");
		shuffleSongs(false, true, false, 1000);
	});

	cmd("format_shuffle", [=] {
		toast("Format shuffle!");
		shuffleSongs(true, false, false, 100);
	});

	cmd("collection_shuffle", [=] {
		toast("Collection shuffle!");
		shuffleSongs(false, false, true, 100);
	});

	cmd("result_shuffle", [=] {
		toast("Result shuffle!");
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

	cmd("close_dialog", [=] {
		if(currentDialog)
			currentDialog->remove();
		currentDialog = nullptr;
	});

	cmd("test_dialog", [=] {
		currentDialog = make_shared<Dialog>(screenptr, font, "Type something:");
		overlay.add(currentDialog);
	});

#ifdef USE_REMOTELISTS
	if(userName == "") {
		currentDialog = make_shared<Dialog>(screenptr, font, "Login with handle:");
		currentDialog->on_ok([=](const string &text) {
			RemoteLists::getInstance().login(text, [=](int rc) {
				userName = text;
				if(rc)
					toast("Login successful", 2);
				File f{Environment::getCacheDir() + "login"};
				f.write(userName);
				f.close();
				auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
				RemoteLists::getInstance().sendList(plist.songs, plist.name,
				                                    [=] { toast("Uploaded", 2); });
			});

		});
		renderSet.add(currentDialog);
	} else {
		auto plist = PlaylistDatabase::getInstance().getPlaylist(currentPlaylistName);
		RemoteLists::getInstance().sendList(plist.songs, plist.name,
		                                    [=] { toast("Uploaded", 2); });
	}
#endif
}

} // namespace
