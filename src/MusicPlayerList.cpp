#include "MusicPlayerList.h"

#include <musicplayer/PSFFile.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList() {
	state = STOPPED;
	playerThread = thread([=]() {
		while(true) {
			update();
			sleepms(100);
		}
	});
	//playerThread.start();
}

void MusicPlayerList::addSong(const SongInfo &si) {
	if(!(permissions & CAN_ADD_SONG))
		return;
	LOCK_GUARD(plMutex);
	playList.push_back(si);
}

void MusicPlayerList::clearSongs() {
	if(!(permissions & CAN_CLEAR_SONGS))
		return;
	LOCK_GUARD(plMutex);
	playList.clear();
}

void MusicPlayerList::nextSong() {
	if(!(permissions & CAN_SWITCH_SONG))
		return;
	LOCK_GUARD(plMutex);
	if(playList.size() > 0) {
		//mp.stop();
		state = WAITING;
	}
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
	if(!(permissions & CAN_SEEK))
		return;
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


void MusicPlayerList::playFile(const std::string &fileName) {
	//LOCK_GUARD(plMutex);
	if(fileName != "") {
		if(mp.playFile(fileName)) {
			changedSong = false;
			updateInfo();
			state = PLAY_STARTED;
		} else {
			state = STOPPED;
		}
	}
}

void MusicPlayerList::update() {

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
			if(length > 0 && pos > length) {
				LOGD("#### SONGLENGTH");
				mp.fadeOut(3.0);
				state = FADING;
			} else
			if(mp.getSilence() > 44100*6) {
				LOGD("############# SILENCE");
				mp.fadeOut(0.5);
				state = FADING;
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

		auto proto = split(currentInfo.path, ":");
		if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
			state = LOADING;
			loadedFile = "";
			auto ext = path_extension(currentInfo.path);
			makeLower(ext);
			LOGD("EXT: %s", ext);
			files = 1;
			if(ext == "mdat") {
				files++;
				auto smpl_file = path_directory(currentInfo.path) + "/" + path_basename(currentInfo.path) + ".smpl";
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

							// HACK BECAUSE WINDOWS (USERS) IS (ARE) STUPID
							//if(path_extension(lib) == "2sflib") {
								// We assume that the case of Nintendo DS libs are incorrect
							//	makeLower(lib);
							//}
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
				} else
					LOGD("Song failed");
				files--;
			});
		} else {
			playFile(currentInfo.path);
		}
	}
}


}