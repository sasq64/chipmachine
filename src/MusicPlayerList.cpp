#include "MusicPlayerList.h"

#include <musicplayer/PSFFile.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList() {
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
	if(pos >= 0) {
		if(playList.size() >= pos)
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
		if(mp.playFile(fileName)) {

			PlayTracker::getInstance().play(currentInfo.path);

			changedSong = false;
			updateInfo();
			LOGD("PLAY STARTED");
			state = PLAY_STARTED;
			return true;			
		} else {
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
		LOGD("##### PLAY NOW: %s (%s)", currentInfo.path, currentInfo.title);
		playCurrent();
	}

	if(state == PLAYING || state == PLAY_STARTED) {

		auto pos = mp.getPosition();
		auto length = mp.getLength();
		if(!changedSong && playList.size() > 0) {
			//LOGD("%d vs %d (SIL %d)", pos, length, mp.getSilence());
			if(!mp.playing()) {
				if(playList.size() == 0)
					state = STOPPED;
				else
					state = WAITING;
			} else 
			if((length > 0 && pos > length) || pos > 7*44100) {
				LOGD("#### SONGLENGTH");
				mp.fadeOut(3.0);
				state = FADING;
			} else
			if(mp.getSilence() > 44100*6) {
				LOGD("############# SILENCE");
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
		if(mp.getVolume() <= 0.01) {
			LOCK_GUARD(plMutex);
			LOGD("#### Music ended");
			if(playList.size() == 0)
				state = STOPPED;
			else
				state = WAITING;
		}
	}

	//if(state == PLAY_STARTED) {
	//	state = PLAYING;
	//}

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
			LOGD("INFO SET");
			playList.pop_front();
			//pos = 0;
		}
		LOGD("##### New song: %s (%s)", currentInfo.path, currentInfo.title);
		if(partyMode) {
			partyLockDown = true;
			setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
		}

		playCurrent();
	}
}

//	private static String [] pref0 = new String [] { "MDAT", "TFX", "SNG", "RJP", "JPN", "DUM", "mdat", "tfx", "sng", "rjp", "jpn", "dum" };
//	private static String [] pref1 = new String [] { "SMPL", "SAM", "INS", "SMP", "SMP", "INS", "smpl", "sam", "ins", "smp", "smp", "ins" };

static std::unordered_map<string, string> fmt_2files = {
	{ "mdat", "smpl" }, // TFMX
	{ "sng", "ins" }, // Richard Joseph
	{ "jpn", "smp" }, // Jason Page PREFIX
	{ "dum", "ins" }, // Rob Hubbard 2
};

void MusicPlayerList::playCurrent() {
	auto proto = split(currentInfo.path, ":");
	if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
		state = LOADING;
		loadedFile = "";
		auto ext = path_extension(currentInfo.path);
		makeLower(ext);
		LOGD("EXT: %s", ext);
		files = 1;

		auto ext2 = fmt_2files[ext];

		if(ext2 != "") {
		//if(ext == "mdat") {
			files++;
			auto smpl_file = path_directory(currentInfo.path) + "/" + path_basename(currentInfo.path) + "." + ext2; //".smpl";
			LOGD("LOADING %s", smpl_file);

			webgetter.getURL(smpl_file, [=](const WebGetter::Job &job) {
				files--;
			});
		}
		webgetter.getURL(currentInfo.path, [=](const WebGetter::Job &job) {
			LOGD("Got file");
			if(job.getReturnCode() == 0) {
				loadedFile = job.getFile();
				LOGD("loadedFile %s", loadedFile);
				PSFFile f { loadedFile };
				if(f.valid()) {
					auto lib = f.tags()["_lib"];
					if(lib != "") {
						auto lib_target = path_directory(loadedFile) + "/" + lib;
						auto lib_url = path_directory(currentInfo.path) + "/" + lib;
						files++;
						webgetter.getURL(lib_url, [=](const WebGetter::Job &job) {
							if(job.getReturnCode() == 0) {
								LOGD("Got lib file %s, copying to %s", job.getFile(), lib_target);
								File::copy(job.getFile(), lib_target);
							}
							files--;
						});
					}
				}
			} else {
				LOCK_GUARD(plMutex);
				errors.push_back("Song download failed");
				LOGD("Song failed");
			}
			files--;
		});
	} else {
		if(!playFile(currentInfo.path)) {
			LOCK_GUARD(plMutex);
			errors.push_back("Could not play song");
			LOGD("Song failed");
		}
	}
}


}