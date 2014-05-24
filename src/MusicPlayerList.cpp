#include "MusicPlayerList.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

using namespace std;
using namespace utils;

namespace chipmachine {

void MusicPlayerList::addSong(const SongInfo &si) {
	lock_guard<mutex> guard(plMutex);
	playList.push_back(si);
}

void MusicPlayerList::clearSongs() {
	lock_guard<mutex> guard(plMutex);
	playList.clear();
}

void MusicPlayerList::nextSong() {
	lock_guard<mutex> guard(plMutex);
	if(playList.size() > 0) {
		mp.stop();
		state = WAITING;
	}
}

void MusicPlayerList::playFile(const std::string &fileName) {
	lock_guard<mutex> guard(plMutex);
	mp.playFile(fileName);
	state = PLAY_STARTED;
	length = mp.getLength();
	auto si = mp.getPlayingInfo();
	if(si.title != "")
		currentInfo.title = si.title;
	if(si.composer != "")
		currentInfo.composer = si.composer;
	if(si.format != "")
		currentInfo.format = si.format;
}

MusicPlayerList::State MusicPlayerList::update() {
	if(state == PLAYING && !mp.playing()) {
		LOGD("#### Music ended");
		if(playList.size() == 0)
			state = STOPPED;
		else
			state = WAITING;
	}

	if(state == PLAY_STARTED) {
		state = PLAYING;
	}

	if(state == WAITING && playList.size() > 0) {
		{
			lock_guard<mutex> guard(plMutex);
			state = STARTED;
			currentInfo = playList.front();
			playList.pop_front();
			//pos = 0;
		}
		LOGD("##### New song: %s", currentInfo.path);

		auto proto = split(currentInfo.path, ":");
		if(proto.size() > 0 && (proto[0] == "http" || proto[0] == "ftp")) {
			webgetter.getURL(currentInfo.path, [=](const WebGetter::Job &job) {
				playFile(job.getFile());
			});
		} else {
			playFile(currentInfo.path);
		}
	}

	return state;
}

uint16_t *MusicPlayerList::getSpectrum() {
	return mp.getSpectrum();
}

int MusicPlayerList::spectrumSize() {
	return mp.spectrumSize();
}

SongInfo MusicPlayerList::getInfo(int index) {
	if(index == 0)
		return currentInfo;
	else
		return playList[index-1];
}

int MusicPlayerList::getLength() {
	return length;
}

int MusicPlayerList::getPosition() {
	return mp.getPosition();
}

int MusicPlayerList::listSize() {
	return playList.size();
}

}