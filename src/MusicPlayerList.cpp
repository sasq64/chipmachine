#include "MusicPlayerList.h"

#include <musicplayer/PSFFile.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList() {
	playerThread = thread([=]() {
		state = STOPPED;
		while(true) {
			update();
			sleepms(100);
		}
	});
	//playerThread.start();
}

void MusicPlayerList::addSong(const SongInfo &si) {
	LOCK_GUARD(plMutex);
	playList.push_back(si);
}

void MusicPlayerList::clearSongs() {
	LOCK_GUARD(plMutex);
	playList.clear();
}

void MusicPlayerList::nextSong() {
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
	if(si.composer != "")
		currentInfo.composer = si.composer;
	if(si.format != "")
		currentInfo.format = si.format;
	if(si.length > 0)
		currentInfo.length = si.length;
	currentInfo.numtunes = si.numtunes;
	currentInfo.starttune = si.starttune;
}

void MusicPlayerList::seek(int song, int seconds) {
	mp.seek(song, seconds);
	updateInfo();
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
	return currentInfo.length;
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
	if(fileName != "")
		mp.playFile(fileName);
	updateInfo();
	state = PLAY_STARTED;
}

void MusicPlayerList::update() {

	if(state == PLAYING && !mp.playing()) {
		LOCK_GUARD(plMutex);
		LOGD("#### Music ended");
		if(playList.size() == 0)
			state = STOPPED;
		else
			state = WAITING;
	}

	if(state == PLAY_STARTED) {
		state = PLAYING;
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
			//pos = 0;
		}
		LOGD("##### New song: %s", currentInfo.path);

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
							if(path_extension(lib) == "2sflib") {
								// We assume that the case of Nintendo DS libs are incorrect
								makeLower(lib);
							}
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