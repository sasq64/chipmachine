
#include "MusicPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/format.h>

#include <audioplayer/audioplayer.h>

#include <musicplayer/plugins/plugins.h>

#include <archive/archive.h>
#include <set>
#include <algorithm>

#ifdef RASPBERRYPI
#define AUDIO_DELAY 12
#elif defined SDL_AUDIO
#define AUDIO_DELAY 2
#else
#define AUDIO_DELAY 18
#endif

using namespace std;
using namespace utils;

namespace chipmachine {

static std::string find_file(const std::string &name) {
	auto f = File::findFile(current_exe_path() + ":" + File::getAppDir(), name);
	return f.getName();
}

MusicPlayer::MusicPlayer() : fifo(32786) {

	ChipPlugin::createPlugins(find_file("data"), plugins);
	plugins.insert(plugins.begin(), make_shared<RSNPlugin>(plugins));

	dontPlay = playEnded = false;
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {

		if(dontPlay) {
			memset(ptr, 0, size*2);
			return;
		}

		if(fifo.filled() >= size) {
			fifo.get(ptr, size);

			lock_guard<mutex> guard(fftMutex);
			pos += size/2;
			fft.addAudio(ptr, size);	
		} else
			memset(ptr, 0, size*2);
	});
}

// Make sure the fifo is filled
void MusicPlayer::update() {

	static int16_t *tempBuf = nullptr;
	if(!tempBuf)
		tempBuf = new int16_t [32768];

	LOCK_GUARD(playerMutex);

	if(!paused && player) {

		sub_title = player->getMeta("sub_title");

		silentFrames = fifo.getSilence();

		while(true) {

			int f = fifo.left() - 128;

			if(f < 128)
				break; 
			int rc = player->getSamples(tempBuf, f);

			if(rc <= 0) {
				playEnded = true;
				break;
			}
			if(fadeOutPos != 0) {
				fifo.setVolume((fadeOutPos - pos) / (float)fadeLength);
			}

			fifo.put(tempBuf, rc);
			if(fifo.filled() >= fifo.size()/2) {
				break;
			}
		}

	}
}

MusicPlayer::~MusicPlayer() {
	AudioPlayer::close();
}

void MusicPlayer::seek(int song, int seconds) {
	LOCK_GUARD(playerMutex);
	if(player) {
		if(seconds < 0)
			pos = 0;
		else
			pos = seconds * 44100;
		if(player->seekTo(song, seconds)) {
			//length = player->getMetaInt("length");
			updatePlayingInfo();
			currentTune = song;
		}
	}
}

int MusicPlayer::getSilence() {
	return silentFrames;
}

// fadeOutPos music
void MusicPlayer::fadeOut(float secs) {
	fadeLength = secs * 44100;
	fadeOutPos = pos + fadeLength;
}


bool MusicPlayer::playFile(const string &fileName) {

	dontPlay = true;
	silentFrames = 0;

	{
		LOCK_GUARD(infoMutex);
		playingInfo = SongInfo();
	}

	string name = fileName;

	if(endsWith(name, ".rar")) {
		try {
			auto *a = Archive::open(name, "_files");\
			for(const auto &s : *a) {
				a->extract(s);
				name = "_files/" + s;
				LOGD("Extracted %s", name);
				break;
			}
		} catch(archive_exception &ae) {
			LOCK_GUARD(playerMutex);
			player = nullptr;
			return false;
		}
	}

	LOCK_GUARD(playerMutex);
	//toPlay = name;
	player = nullptr;
	player = fromFile(name);
	//toPlay = "";

	dontPlay = false;
	playEnded = false;

	if(player) {

		fifo.clear();
		fadeOutPos = 0;
		//changedSong = false;
		pause(false);
		pos = 0;
		updatePlayingInfo();
		currentTune = playingInfo.starttune;
		return true;
	}
	return false;
}

void MusicPlayer::updatePlayingInfo() {
	SongInfo si;
	if(player) {

		auto game = player->getMeta("game");
		si.title = player->getMeta("title");
		if(game != "") {
			if(si.title != "") {
				si.title = format("%s (%s)", game, si.title);
			} else
			si.title = game;
		}

		si.composer = player->getMeta("composer");
		si.format = player->getMeta("format");
		//si.length = player->getMetaInt("length");
		si.numtunes = player->getMetaInt("songs");
		si.starttune = player->getMetaInt("startSong");

		length = player->getMetaInt("length");
		message = player->getMeta("message");
		sub_title = player->getMeta("sub_title");
	}
	{
		LOCK_GUARD(infoMutex);
		playingInfo = si;
	}
}

void MusicPlayer::pause(bool dopause) {
	if(dopause) AudioPlayer::pause_audio();
	else AudioPlayer::resume_audio();
	paused = dopause;
}

string MusicPlayer::getMeta(const string &what) {
	if(what == "message") {
		LOCK_GUARD(infoMutex);
		return message;
	}

	if(what == "sub_title") {
		LOCK_GUARD(infoMutex);
		return sub_title;
	}

	//if(what == "song") {
	//	LOCK_GUARD(infoMutex);
	//	return current_song;
	//}

	LOCK_GUARD(playerMutex);
	if(player)
		return player->getMeta(what);
	else
		return "";
}

uint16_t *MusicPlayer::getSpectrum() {
	LOCK_GUARD(fftMutex);
	if(fft.size() > AUDIO_DELAY) {
		while(fft.size() > AUDIO_DELAY*2)
			fft.popLevels();
		spectrum = fft.getLevels();
		fft.popLevels();

	}
	return &spectrum[0];

}

// PRIVATE

shared_ptr<ChipPlayer> MusicPlayer::fromFile(const string &fileName) {
	shared_ptr<ChipPlayer> player;
	string name = fileName;
	utils::makeLower(name);
	for(auto &plugin : plugins) {
		if(plugin->canHandle(name)) {
			LOGD("Playing with %s\n", plugin->name());
			player = shared_ptr<ChipPlayer>(plugin->fromFile(fileName));
			break;
		}
	}
	return player;
}


}