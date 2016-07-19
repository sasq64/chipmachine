#include "MusicPlayerList.h"
#include "MusicPlayer.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <algorithm>
#include <unordered_map>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList(const std::string &workDir) : mp(std::make_shared<MusicPlayer>(workDir)) {
	SET_STATE(STOPPED);
	wasAllowed = true;
	permissions = 0xff;
	quitThread = false;
	playerThread = thread([=]() {
		while(!quitThread) {

			// Call onThisThread delayed lambdas
			plMutex.lock();
			if(funcs.size()) {
				auto q = funcs;
				funcs.clear();
				plMutex.unlock();
				for(auto &f : q)
					f();
			} else
				plMutex.unlock();

			// Update music player
			update();

			// Transfer state to shared
			shared.paused = mp->isPaused();
			shared.playing = mp->playing();
			shared.volume = mp->getVolume();
			shared.length = mp->getLength();
			shared.position = mp->getPosition();
			shared.playlist_size = playList.size();

			if(multiSongs.size())
				shared.tune = multiSongNo;
			else
				shared.tune = mp->getTune();

			{ LOCK_GUARD(plMutex);
				shared.info[0] = dbInfo;
				shared.info[1] = currentInfo;
				shared.sub_title = mp->getMeta("sub_title");
				shared.message = mp->getMeta("message");

				if(playList.size() > 0)
					shared.info[2] = playList.getSong(0);
				if(updateSong) {
					shared.songFiles = songFiles;
					updateSong = false;
				}
			}
			for(int i=0; i<10; i++) {
				RemoteLoader::getInstance().update();
				sleepms(10);
			}
		}
	});
}

void MusicPlayerList::addSong(const SongInfo &si, bool shuffle) {

	onThisThread([=] {
		if(shuffle) {
			playList.insertAt(rand() % (playList.size() + 1), si);
		} else {
			LOGD("PUSH %s/%s (%s)", si.title, si.composer, si.path);
			playList.push_back(si);
		}
	});
	//return true;
}

void MusicPlayerList::clearSongs() {
	onThisThread( [=] { playList.clear(); });
}

void MusicPlayerList::nextSong() {
	onThisThread( [=] {
		if(playList.size() > 0) {
			SET_STATE(WAITING);
		}
	});
}

void MusicPlayerList::pause(bool dopause) {
	if(!(permissions & CAN_PAUSE))
		return;
	onThisThread([=] {
		mp->pause(dopause);
	});
}

void MusicPlayerList::setVolume(float volume) {
	onThisThread([=] {
	   	mp->setVolume(volume);
	});
}

void MusicPlayerList::stop() {
	onThisThread([=] {
		SET_STATE(STOPPED);
		mp->stop();
	});
}

void MusicPlayerList::playSong(const SongInfo &si) {
	onThisThread( [=] {
		dbInfo = currentInfo = si;
		SET_STATE(PLAY_NOW);
	});
}

void MusicPlayerList::seek(int song, int seconds) {
	onThisThread([=] {
		if(multiSongs.size()) {
			LOGD("CHANGED MULTI");
			state = PLAY_MULTI;
			multiSongNo = song;
			return;
		}
		mp->seek(song, seconds);
		if(song >= 0)
			changedSong = true;
	});
}

uint16_t *MusicPlayerList::getSpectrum() {
	return mp->getSpectrum();
}

int MusicPlayerList::spectrumSize() {
	return mp->spectrumSize();
}

SongInfo MusicPlayerList::getInfo(int index) {
	LOCK_GUARD(plMutex);
	if(index > 1)
		return SongInfo();
	return shared.info[index+1];
}

SongInfo MusicPlayerList::getDBInfo() {
	LOCK_GUARD(plMutex);
	return shared.info[0];
}

int MusicPlayerList::getLength() {
	return shared.length; //mp.getLength(); // currentInfo.length;
}

int MusicPlayerList::getPosition() {
	return shared.position; //mp.getPosition();
}

int MusicPlayerList::listSize() {
	return shared.playlist_size;
}

/// PRIVATE

void MusicPlayerList::updateInfo() {
	auto si = mp->getPlayingInfo();
	if(si.format != "")
		currentInfo.format = si.format;
	if(!multiSongs.size()) {
		currentInfo.numtunes = si.numtunes;
		currentInfo.starttune = si.starttune;
	}
}

bool MusicPlayerList::handlePlaylist(const string &fileName) {

	playList.clear();
	File f{fileName};

	auto lines = f.getLines();

	// Remove lines with comment character
	lines.erase(std::remove_if(lines.begin(), lines.end(), [=](const string &l) {
		            return l[0] == ';';
		        }), lines.end());
	for(const string &s : lines) {
		playList.push_back(SongInfo(s));
	}

    if(playList.size() == 0)
        return false;
/*
	lookup(playList.front());
	if(playList.front().path == "") {
		LOGD("Could not lookup '%s'", playList.front().path);
		putError("Bad song in playlist");
		return false;
	}
	SET_STATE(WAITING); */
	return true;
}

bool MusicPlayerList::playFile(const std::string &fn) {
	auto fileName = fn;
	if(fileName == "")
		return false;
	auto ext = toLower(path_extension(fileName));
	if(ext == "pls" || currentInfo.format == "PLS") {
		File f{fileName};

		auto lines = f.getLines();
		vector<string> result;
		for(auto &l : lines) {
			if(startsWith(l, "File1="))
				result.push_back(l.substr(6));
		}
		currentInfo.path = result[0];
		currentInfo.format = "MP3";
		playCurrent();
		return false;

	} else if(ext == "m3u" || currentInfo.format == "M3U") {
		File f{fileName};

		auto lines = f.getLines();

		// Remove lines with comment character
		lines.erase(std::remove_if(lines.begin(), lines.end(), [=](const string &l) {
			            return l == "" || l[0] == '#';
			        }), lines.end());
		currentInfo.path = lines[0];
		currentInfo.format = "MP3";
		playCurrent();
		return false;

	} else if(ext == "plist") {
		handlePlaylist(fileName);
		SET_STATE(WAITING);
		return true;
	} else if(ext == "jb") {
		// Jason Brooke fix
		string newName = fileName.substr(0, fileName.find_last_of('.')) + ".jcb";
		if(!File::exists(newName))
			File::copy(fileName, newName);
		fileName = newName;
	}

	if(mp->playFile(fileName)) {
#ifdef USE_REMOTELISTS
		if(reportSongs)
			RemoteLists::getInstance().songPlayed(currentInfo.path);
#endif
		LOGD("STARRTUNE %d", currentInfo.starttune);
		if(currentInfo.starttune >= 0)
			mp->seek(currentInfo.starttune);
		changedSong = false;
		LOGD("CHANGED MULTI:%s", changedMulti ? "YES" : "NO");
		if(!changedMulti) {
			updateInfo();
			LOGD("STATE: Play started");
			SET_STATE(PLAY_STARTED);
		} else
			SET_STATE(PLAYING);

		changedMulti = false;
		return true;
	} else 
		putError("Could not play song");
	return false;
}

void MusicPlayerList::putError(const std::string &error) {
	LOCK_GUARD(plMutex);
	errors.push_back(error);
	SET_STATE(ERROR);
}

void MusicPlayerList::setPartyMode(bool on, int lockSec, int graceSec) {
	partyMode = on;
	partyLockDown = false;
	setPermissions(0xffff);
	lockSeconds = lockSec;
	graceSeconds = graceSec;
}

void MusicPlayerList::update() {

	mp->update();


	if(partyMode) {
		auto p = getPosition();
		if(partyLockDown) {
			if(p >= lockSeconds) {
				setPermissions(0xffff);
				partyLockDown = false;
			}
		} else {
			if(p >= graceSeconds && p < lockSeconds) {
				partyLockDown = true;
				setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
			}
		}
	}

	if(state == PLAY_NOW) {
		SET_STATE(STARTED);
		// LOGD("##### PLAY NOW: %s (%s)", currentInfo.path, currentInfo.title);
		multiSongs.clear();
		playedNext = false;
		playCurrent();
	}

	if(state == PLAY_MULTI) {
		SET_STATE(STARTED);
		currentInfo.path = multiSongs[multiSongNo];
		changedMulti = true;
		playCurrent();
	}

	if(state == PLAYING || state == PLAY_STARTED) {

		auto pos = mp->getPosition();
		auto length = mp->getLength();

		if(cueSheet) {
			cueTitle = cueSheet->getTitle(pos);
		}

		if(!changedSong && playList.size() > 0) {
			if(!mp->playing()) {
				if(playList.size() == 0)
					SET_STATE(STOPPED);
				else
					SET_STATE(WAITING);
			} else if((length > 0 && pos > length) && pos > 7) {
				LOGD("STATE: Song length exceeded");
				mp->fadeOut(3.0);
				SET_STATE(FADING);
			} else if(detectSilence && mp->getSilence() > 44100 * 6 && pos > 7) {
				LOGD("STATE: Silence detected");
				mp->fadeOut(0.5);
				SET_STATE(FADING);
			}
		} else if(partyLockDown) {
			if((length > 0 && pos > length) || mp->getSilence() > 44100 * 6) {
				partyLockDown = false;
				setPermissions(0xffff);
			}
		}
	}

	if(state == FADING) {
		if(mp->getFadeVolume() <= 0.01) {
			LOGD("STATE: Music ended");
			if(playList.size() == 0)
				SET_STATE(STOPPED);
			else
				SET_STATE(WAITING);
		}
	}

	if(state == LOADING) {
		if(files == 0) {
			RemoteLoader::getInstance().cancel();
			playFile(loadedFile);
		}
	}

	if(state == WAITING && (playList.size() > 0)) {
		SET_STATE(STARTED);
		playedNext = true;
		dbInfo = currentInfo = playList.front();
		playList.pop_front();

		if(playList.size() > 0) {
			// Update info for next song from
			//lookup(playList.front());
		}
			
		// pos = 0;
		LOGD("Next song from queue : %s (%d)", currentInfo.path, currentInfo.starttune);
		if(partyMode) {
			partyLockDown = true;
			setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
		}
		multiSongs.clear();
		playCurrent();
	}
}

/// Main work horse - Load and play the song/playlist/stream/etc in currentInfo
void MusicPlayerList::playCurrent() {

	SET_STATE(LOADING);
	
	songFiles.clear();
	
	RemoteLoader &loader = RemoteLoader::getInstance();
	loader.cancel();

	string prefix, path;

	// Resolve path to a single song (expandning playlists, multi songs, products etc)
	while(true) {
		LOGD("Trying to play '%s'", currentInfo.path);
		auto parts = split(currentInfo.path, "::", 2);
		if(parts.size() == 2) {
			prefix = parts[0];
			path = parts[1];
		} else
			path = currentInfo.path;

		// If song is a playlist, load it into the playQueue
		if(prefix == "playlist") {
			if(!handlePlaylist(path)) {
				SET_STATE(ERROR);
				return;
			}
			currentInfo = playList.front();
			continue;
		}

		// A multi song is a group of songs that is treated like a single song with subtunes
		if(startsWith(path, "MULTI:")) {
			multiSongs = split(path.substr(6), "\t");
			if(prefix != "") {
				for(string &m : multiSongs) {
					m = prefix + "::" + m;
				}
			}
			multiSongNo = 0;
			currentInfo.path = multiSongs[0];
			currentInfo.numtunes = multiSongs.size();
			continue;
		}

		// Get updated information from parent, if available.
		// ... may modify the play queue
		if(lookup(currentInfo)) {
			if(playList.size() > 0)
				currentInfo = playList.front();
			else {
				SET_STATE(ERROR);
				return;
			}
			continue;
		}
		dbInfo = currentInfo;
		break;
	}

	// We have our info -- figure out what it is and load it

	auto ext = path_extension(path);
	makeLower(ext);

	detectSilence = true;
	if(ext == "mp3")
		detectSilence = false;

	cueSheet = nullptr;
	cueTitle = "";
	updateSong = true;

	if(File::exists(currentInfo.path)) {
		LOGD("PLAYING LOCAL FILE %s", currentInfo.path);
		songFiles = { File(currentInfo.path) };
		loadedFile = currentInfo.path;
		files = 0;
		return;
	}

	loadedFile = "";
	files = 0;

	string cueName = "";
	if(prefix == "bitjam")
		cueName = currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) + ".cue";
	else if(prefix == "demovibes")
		cueName = toLower(currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) + ".cue");

	if(cueName != "") {
		loader.load(cueName, [=](File cuefile) {
			if(cuefile)
				cueSheet = make_shared<CueSheet>(cuefile);
		});
	}
	
	if(startsWith(currentInfo.path, "pouet::")) {
		loadedFile = currentInfo.path.substr(7);
		files = 0;
		return;
	}

	if(currentInfo.format != "M3U" && (ext == "mp3" || toLower(currentInfo.format) == "mp3")) {

		if(mp->streamFile("dummy.mp3")) {
			SET_STATE(PLAY_STARTED);
			LOGD("### Start Stream");
			loader.stream(currentInfo.path, [=](int what, const uint8_t *ptr, int n) -> bool {
				if(what == RemoteLoader::PARAMETER) {
					mp->setParameter((char *)ptr, n);
				} else {
					mp->putStream(ptr, n);
				}
				return true;
			});
		}

		return; 
	}

	bool isStarTrekker = (currentInfo.path.find("Startrekker") != string::npos);

	// Known music formats with 2 files
	static const std::unordered_map<string, string> fmt_2files = {
	    {"mdat", "smpl"},   // TFMX
	    {"sng", "ins"},     // Richard Joseph
	    {"jpn", "smp"},     // Jason Page PREFIX
	    {"dum", "ins"},     // Rob Hubbard 2
	    {"adsc", "adsc.as"} // Audio Sculpture
	};
	string ext2;
	if(fmt_2files.count(ext) > 0)
		ext2 = fmt_2files.at(ext);
	if(isStarTrekker)
		ext2 = "mod.nt";
	if(ext2 != "") {
		files++;
		auto smpl_file = currentInfo.path.substr(0, currentInfo.path.find_last_of('.') + 1) + ext2;
		LOGD("Loading secondary (sample) file '%s'", smpl_file);
		loader.load(smpl_file, [=](File f) {
			updateSong = true;
			if(!f) {
				putError("Could not load secondary file");
			};
			songFiles.push_back(f);
			files--;
		});
	}

	// LOGD("LOADING:%s", currentInfo.path);
	files++;
	loader.load(currentInfo.path, [=](File f0) {
		LOGD("File %s", f0.getName());
		if(!f0) {
			putError("Could not load file");
			files--;
			return;
		}
		updateSong = true;
		songFiles.push_back(f0);
		loadedFile = f0.getName();
		auto ext = toLower(path_extension(loadedFile));
		LOGD("Loaded file '%s'", loadedFile);
		auto parentDir = File(path_directory(loadedFile));
		auto fileList = mp->getSecondaryFiles(f0);
		for(auto s : fileList) {
			File target = parentDir / s;
			if(!target.exists()) {
				files++;
				RemoteLoader &loader = RemoteLoader::getInstance();
				auto url = path_directory(currentInfo.path) + "/" + s;
				loader.load(url, [=](File f) {
					if(!f) {
						putError("Could not load file");
					} else {
						LOGD("Copying secondary file to %s", target.getName());
						File::copy(f.getName(), target);
						updateSong = true;
						songFiles.push_back(target);
					}
					files--;
				});
			} else
				songFiles.push_back(target);
		}

		files--;
	});
}

} // namespace

#ifdef CM_UNIT_TEST

#include "catch.hpp"

TEST_CASE("chipmachine::MusicPlayerList", "") {

	using namespace utils;
	using namespace std;
	using namespace chipmachine;

	auto& loader = RemoteLoader::getInstance();

	loader.registerSource("modland", "http://localhost:8000/MODLAND/", "");

	MusicPlayerList mp(".");
	mp.setLookupFunction([=](SongInfo &info, MusicPlayerList::PlayQueue &pq) -> bool {
		// Prevent using MusicDatabase
		return false;
	});

	SongInfo info("modland::Protracker/Horace Wimp/1990.mod");

	mp.playSong(info);

	int counter = 20;
	while(counter--) {
		utils::sleepms(100);
	}
	

	//REQUIRE(file.getName() == "temp.text"i);
}

#endif
