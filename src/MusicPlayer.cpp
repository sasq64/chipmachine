
#include "MusicPlayer.h"

#include <coreutils/utils.h>

#include <audioplayer/audioplayer.h>

#include <musicplayer/plugins/ModPlugin/ModPlugin.h>
#include <musicplayer/plugins/VicePlugin/VicePlugin.h>
#include <musicplayer/plugins/SexyPSFPlugin/SexyPSFPlugin.h>
#include <musicplayer/plugins/GMEPlugin/GMEPlugin.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <musicplayer/plugins/UADEPlugin/UADEPlugin.h>


using namespace std;

namespace chipmachine {

MusicPlayer::MusicPlayer() : plugins {
		new ModPlugin {},
		new VicePlugin {"data/c64"},
		new SexyPSFPlugin {},
		new GMEPlugin {},
		new SC68Plugin {"data/sc68"},
		new UADEPlugin {}
	}
{
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {
		lock_guard<mutex> guard(m);
/*		if(toPlay != "") {
			player = nullptr;
			player = fromFile(toPlay);
			toPlay = "";
			if(player) {
				pos = 0;
				length = player->getMetaInt("length");
			}
		} */
		if(player) {
			int rc = player->getSamples(ptr, size);
			if(rc > 0)
				fft.addAudio(ptr, size);
			if(rc < 0) {
				player = nullptr;
				LOGD("GOT %d", rc);
			}
			else pos += size/2;
		} else
			memset(ptr, 0, size*2);
	});
}

void MusicPlayer::seek(int song, int seconds) {
	lock_guard<mutex> guard(m);
	player->seekTo(song, seconds);
}

void MusicPlayer::playFile(const std::string &fileName) {
	lock_guard<mutex> guard(m);
	//toPlay = fileName;
	player = nullptr;
	player = fromFile(fileName);
	//toPlay = "";
	if(player) {
		pos = 0;
		length = player->getMetaInt("length");
	}
}

SongInfo MusicPlayer::getPlayingInfo() {
	lock_guard<mutex> guard(m);
	SongInfo si;
	si.title = player->getMeta("title");
	si.composer = player->getMeta("composer");
	return si;
}


shared_ptr<ChipPlayer> MusicPlayer::fromFile(const std::string &fileName) {
	shared_ptr<ChipPlayer> player;
	string name = fileName;
	utils::makeLower(name);
	for(auto *plugin : plugins) {
		if(plugin->canHandle(name)) {
			printf("Playing with %s\n", plugin->name().c_str());
			player = shared_ptr<ChipPlayer>(plugin->fromFile(fileName));
			break;
		}
	}
	return player;
}

}