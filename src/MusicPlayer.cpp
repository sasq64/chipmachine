
#include "MusicPlayer.h"

#include <coreutils/utils.h>

#include <audioplayer/audioplayer.h>

#include <musicplayer/plugins/ModPlugin/ModPlugin.h>
#include <musicplayer/plugins/VicePlugin/VicePlugin.h>
#include <musicplayer/plugins/SexyPSFPlugin/SexyPSFPlugin.h>
#include <musicplayer/plugins/GMEPlugin/GMEPlugin.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <musicplayer/plugins/StSoundPlugin/StSoundPlugin.h>
#include <musicplayer/plugins/AdPlugin/AdPlugin.h>
#include <musicplayer/plugins/UADEPlugin/UADEPlugin.h>


using namespace std;

namespace chipmachine {

MusicPlayer::MusicPlayer() : fifo(32786), plugins {
		new ModPlugin {},
		new VicePlugin {"data/c64"},
		new SexyPSFPlugin {},
		new GMEPlugin {},
		new SC68Plugin {"data/sc68"},
		new StSoundPlugin {},
		new AdPlugin {},
		new UADEPlugin {}
	}
{
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {
		lock_guard<mutex> guard(m);


		if(fadeOut > 0 && fadeOut <= pos) {
			player = nullptr;
		}

		if(!paused && player) {
			int sz = size;

			if(fadeOut == 0) {
				if(length > 0 && pos/44100 > length) {
					LOGD("#### SONGLENGTH");
					fadeOut = pos + 44100*3;
				}
				if(fifo.getSilence() > 44100*5) {
					LOGD("############# SILENCE");
					//fadeOut = pos + 44100*3;
					fadeOut = pos;
				}
			}

			while(true) {
				if(sz <= 0)
					LOGD("WTF!");
				int rc = player->getSamples((int16_t*)fifo.ptr(), sz);				
				if(rc <= 0) {
					LOGD("RC %d", rc);
					player = nullptr;
					break;
				}

				//LOGD("SILENCE %d", fifo.getSilence());
				if(fadeOut != 0)
					fifo.setVolume((fadeOut - pos) / (3.0*44100.0));
				fifo.putShorts(nullptr, rc);
				pos += rc/2;
				sz -= rc;
				if(fifo.filled() >= size*2) {
					fifo.getShorts(ptr, size);
					fft.addAudio(ptr, size);
					break;
				}
			}

		} else
			memset(ptr, 0, size*2);
	});
}

void MusicPlayer::seek(int song, int seconds) {
	lock_guard<mutex> guard(m);
	if(player) {
		if(seconds < 0)
			pos = 0;
		else
			pos = seconds * 44100;
		player->seekTo(song, seconds);
		length = player->getMetaInt("length");
	}
}

void MusicPlayer::playFile(const std::string &fileName) {
	lock_guard<mutex> guard(m);
	//toPlay = fileName;
	player = nullptr;
	player = fromFile(fileName);
	//toPlay = "";
	if(player) {

		fifo.clear();
		fadeOut = 0;
		pause(false);
		pos = 0;
		length = player->getMetaInt("length");
	}
}

SongInfo MusicPlayer::getPlayingInfo() {
	lock_guard<mutex> guard(m);
	SongInfo si;
	if(player) {
		si.title = player->getMeta("title");
		si.composer = player->getMeta("composer");
		si.format = player->getMeta("format");
		si.length = player->getMetaInt("length");
		si.numtunes = player->getMetaInt("songs");
		si.starttune = player->getMetaInt("startSong");
	}
	return si;
}

void MusicPlayer::pause(bool dopause) {
	if(dopause) AudioPlayer::pause_audio();
	else AudioPlayer::resume_audio();
	paused = dopause;
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