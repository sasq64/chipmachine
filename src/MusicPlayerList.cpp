#include "MusicPlayerList.h"

#include <musicplayer/PSFFile.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <algorithm>
#include <unordered_map>

using namespace std;
using namespace utils;

namespace chipmachine {


MusicPlayerList::MusicPlayerList(const std::string &workDir) : mp(workDir) { //: webgetter(File::getCacheDir() + "_webfiles") {
	state = STOPPED;
	wasAllowed = true;
	permissions = 0xff;
	quitThread = false;
	playerThread = thread([=]() {
		while(!quitThread) {
			update();
			sleepms(100);
		}
	});
	//playerThread.start();
}

bool MusicPlayerList::checkPermission(int flags) {
	if(!(permissions & flags)) {
		wasAllowed = false;
		return false;
	}
	return true;
}

bool MusicPlayerList::addSong(const SongInfo &si, int pos) {

	if(!checkPermission(CAN_ADD_SONG)) return false;
	LOCK_GUARD(plMutex);

	//LOGD("Add song %s %s %s %s", si.path, si.title, si.composer, si.format);

	if(pos >= 0) {
		if((int)playList.size() >= pos)
			playList.insert(playList.begin() + pos, si);
	} else {
		if(partyMode) {
			if(playList.size() >= 50) {
				wasAllowed = false;
				return false;
			}
			playList.insert(playList.begin() + (rand() % (playList.size()+1)), si);
		} else
			playList.push_back(si);
	}
	return true;
}

void MusicPlayerList::clearSongs() {
	if(!checkPermission(CAN_CLEAR_SONGS)) return;
	LOCK_GUARD(plMutex);
	playList.clear();
}

void MusicPlayerList::nextSong() {
	if(!checkPermission(CAN_SWITCH_SONG)) return;
	LOCK_GUARD(plMutex);
	if(playList.size() > 0) {
		//mp.stop();
		state = WAITING;
	}
}

void MusicPlayerList::playSong(const SongInfo &si) {
	if(!checkPermission(CAN_SWITCH_SONG)) return;
	LOCK_GUARD(plMutex);
	currentInfo = si;
	state = PLAY_NOW;
}

void MusicPlayerList::updateInfo() {
	LOCK_GUARD(plMutex);
	auto si = mp.getPlayingInfo();
	if(si.title != "")
		currentInfo.title = si.title;
	LOGD("UPDATE title %s", si.title);
	if(si.composer != "")
		currentInfo.composer = si.composer;
	if(si.format != "")
		currentInfo.format = si.format;
	//if(si.length > 0)
	//	currentInfo.length = si.length;
	currentInfo.numtunes = si.numtunes;
	currentInfo.starttune = si.starttune;
}

void MusicPlayerList::seek(int song, int seconds) {
	if(!checkPermission(CAN_SEEK)) return;
	mp.seek(song, seconds);
	if(song >= 0)
		changedSong = true;

	//updateInfo();
}

uint16_t *MusicPlayerList::getSpectrum() {
	return mp.getSpectrum();
}

int MusicPlayerList::spectrumSize() {
	return mp.spectrumSize();
}

SongInfo MusicPlayerList::getInfo(int index) {
	LOCK_GUARD(plMutex);
	if(index == 0)
		return currentInfo;
	else
		return playList[index-1];
}

int MusicPlayerList::getLength() {
	return mp.getLength();//currentInfo.length;
}

int MusicPlayerList::getPosition() {
	return mp.getPosition();
}

int MusicPlayerList::listSize() {
	return playList.size();
}


/// PRIVATE


bool MusicPlayerList::playFile(const std::string &fileName) {
	//LOCK_GUARD(plMutex);
	if(fileName != "") {

		if(path_extension(fileName) == "plist") {
			clearSongs();
			File f { fileName };

			auto lines = f.getLines();

			lines.erase(std::remove_if(lines.begin(), lines.end(), [=](const string &l) {
				if(l[0] == ';') {
					return true;
				}
				return false;
			}), lines.end());
/*
			if(lines.size() > 10) {
				std::random_device rd;
			    std::shuffle(lines.begin(), lines.end(), rd);
			}
*/
			for(const string &s : lines) {
				addSong(SongInfo(s));
			}
			SongInfo &si = playList.front();
			auto path = si.path;
			si = MusicDatabase::getInstance().lookup(si.path);
			if(si.path == "") {
				LOGD("Could not lookup '%s'", path);
				errors.push_back("Bad song in playlist");
				state = STOPPED;
				return false;
			}
			state = WAITING;
			return true;
		}

		if(mp.playFile(fileName)) {
			if(reportSongs)
				RemoteLists::getInstance().songPlayed(currentInfo.path);

			changedSong = false;
			updateInfo();
			LOGD("STATE: Play started");
			state = PLAY_STARTED;
			return true;
		} else {
			errors.push_back("Could not play song");
			state = STOPPED;
		}
	}
	return false;
}

void MusicPlayerList::setPartyMode(bool on, int lockSec, int graceSec) {
	partyMode = on;
	partyLockDown = false;
	setPermissions(0xffff);
	lockSeconds = lockSec;
	graceSeconds = graceSec;
}

void MusicPlayerList::update() {

	mp.update();

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
		state = STARTED;
		//LOGD("##### PLAY NOW: %s (%s)", currentInfo.path, currentInfo.title);
		playCurrent();
	}

	if(state == PLAYING || state == PLAY_STARTED) {

		auto pos = mp.getPosition();
		auto length = mp.getLength();
		if(!changedSong && playList.size() > 0) {
			if(!mp.playing()) {
				if(playList.size() == 0)
					state = STOPPED;
				else
					state = WAITING;
			} else
			if((length > 0 && pos > length) && pos > 7) {
				LOGD("STATE: Song length exceeded");
				mp.fadeOut(3.0);
				state = FADING;
			} else
			if(mp.getSilence() > 44100*6 && pos > 7) {
				LOGD("STATE: Silence detected");
				mp.fadeOut(0.5);
				state = FADING;
			}
		} else if(partyLockDown) {
			if((length > 0 && pos > length) || mp.getSilence() > 44100*6) {
				partyLockDown = false;
				setPermissions(0xffff);
			}
		}
	}

	if(state == FADING) {
		if(mp.getFadeVolume() <= 0.01) {
			LOCK_GUARD(plMutex);
			LOGD("STATE: Music ended");
			if(playList.size() == 0)
				state = STOPPED;
			else
				state = WAITING;
		}
	}

	if(state == LOADING) {
		if(files == 0) {
			playFile(loadedFile);
		}
	}

	if(state == WAITING && playList.size() > 0) {
		{
			LOCK_GUARD(plMutex);
			state = STARTED;
			currentInfo = playList.front();
			playList.pop_front();

			if(playList.size() > 0) {
				SongInfo &si = playList.front();
				si = MusicDatabase::getInstance().lookup(si.path);
			}

			//pos = 0;
		}
		LOGD("Next song from queue : %s", currentInfo.path);
		if(partyMode) {
			partyLockDown = true;
			setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
		}

		playCurrent();
	}
}

void MusicPlayerList::playCurrent() {
	// Music formats with 2 files
	static const std::unordered_map<string, string> fmt_2files = {
		{ "mdat", "smpl" }, // TFMX
		{ "sng", "ins" }, // Richard Joseph
		{ "jpn", "smp" }, // Jason Page PREFIX
		{ "dum", "ins" }, // Rob Hubbard 2
	};

	state = LOADING;

	if(File::exists(currentInfo.path)) {
		loadedFile = currentInfo.path;
		files = 0;
		return;
	}

	loadedFile = "";

	auto ext = path_extension(currentInfo.path);
	makeLower(ext);
	//LOGD("EXT: %s", ext);
	files = 1;
	string ext2;
	if(fmt_2files.count(ext) > 0)
		ext2 = fmt_2files.at(ext);

	RemoteLoader &loader = RemoteLoader::getInstance();

	if(ext == "mp3" || toLower(currentInfo.format) == "mp3") {

		shared_ptr<Streamer> streamer = mp.streamFile("dummy.mp3");

		if(streamer) {

		 	files = 0;
		 	state = PLAY_STARTED;
		 	loader.stream(currentInfo.path, [=](uint8_t *ptr, int size) {
		 		streamer->put(ptr, size);
		 	});
		}
	 	return;
	}

	if(ext2 != "") {
		files++;
		auto smpl_file = path_directory(currentInfo.path) + "/" + path_basename(currentInfo.path) + "." + ext2;
		LOGD("Loading secondary (sample) file '%s'", smpl_file);
		loader.load(smpl_file, [=](File f) {
			files--;
		});
	}

	// LOGD("LOADING:%s", currentInfo.path);
	loader.load(currentInfo.path, [=](File f0) {
		//LOGD("Got file");
		loadedFile = f0.getName();
		LOGD("Loaded file '%s'", loadedFile);
		PSFFile f { loadedFile };
		if(f.valid()) {
			auto lib = f.tags()["_lib"];
			if(lib != "") {
				auto lib_target = path_directory(loadedFile) + "/" + lib;
				makeLower(lib);
				auto lib_url = path_directory(currentInfo.path) + "/" + lib;
				files++;
				LOGD("Loading library file '%s'");
				RemoteLoader &loader = RemoteLoader::getInstance();
				loader.load(lib_url, [=](File f) {
					LOGD("Got lib file %s, copying to %s", f.getName(), lib_target);
					File::copy(f.getName(), lib_target);
					files--;
				});
			}
		}
		files--;
	});
}

}