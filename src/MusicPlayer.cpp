
#include "MusicPlayer.h"
#include "GZPlugin.h"
#include <coreutils/utils.h>
#include <coreutils/format.h>

#include <audioplayer/audioplayer.h>
#include <musicplayer/PSFFile.h>

#include <musicplayer/plugins/plugins.h>

#include <archive/archive.h>
#include <set>
#include <algorithm>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayer::MusicPlayer(const std::string &workDir) : fifo(32786 * 4) {

	AudioPlayer::set_volume(80);
	volume = 0.8;

	ChipPlugin::createPlugins(workDir);
	ChipPlugin::addPlugin(make_shared<RSNPlugin>(ChipPlugin::getPlugins()));
	ChipPlugin::addPlugin(make_shared<GZPlugin>(ChipPlugin::getPlugins()));
	
	dontPlay = playEnded = false;
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {

		if(dontPlay) {
			memset(ptr, 0, size * 2);
			return;
		}

		lock_guard<mutex> guard(fftMutex);
		if(fifo.filled() >= size) {
			fifo.get(ptr, size);

			pos += size / 2;
			fft.addAudio(ptr, size);
		} else
			memset(ptr, 0, size * 2);
	});
}

// Make sure the fifo is filled
void MusicPlayer::update() {

    static vector<int16_t> tempBuf(fifo.size());

	if(!paused && player) {

		sub_title = player->getMeta("sub_title");
		length = player->getMetaInt("length");
		message = player->getMeta("message");
		silentFrames = checkSilence ? fifo.getSilence() : 0;

		while(true) {

			int f = fifo.left();

			if(f < 4096)
				break;

            int rc = player->getSamples(&tempBuf[0], f - 1024);

			if(rc == 0)
				break;

			if(rc < 0) {
				playEnded = true;
				break;
			}
			if(fadeOutPos != 0) {
				fifo.setVolume((fadeOutPos - pos) / (float)fadeLength);
			}

            fifo.put(&tempBuf[0], rc);
            if(fifo.filled() >= fifo.size() / 2) {
				break;
			}
		}
	}
}

MusicPlayer::~MusicPlayer() {
	AudioPlayer::close();
}

void MusicPlayer::seek(int song, int seconds) {
	if(player->seekTo(song, seconds)) {
		if(seconds < 0)
			pos = 0;
		else
			pos = seconds * 44100;
		fifo.clear();
		// length = player->getMetaInt("length");
		updatePlayingInfo();
		currentTune = song;
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

bool MusicPlayer::streamFile(const string &fileName) {
	dontPlay = true;
	silentFrames = 0;

	playingInfo = SongInfo();
	string name = fileName;

	player = nullptr;
	player = fromStream(fileName);

	dontPlay = false;
	playEnded = false;

	if(player) {

		fifo.clear();
		fadeOutPos = 0;
		pause(false);
		pos = 0;
		// updatePlayingInfo();
		message = "";
		length = 0;
		sub_title = "";
		currentTune = playingInfo.starttune;
		return true;
	}
	return false;
}

bool MusicPlayer::playFile(const string &fileName) {

	dontPlay = true;
	silentFrames = 0;

	playingInfo = SongInfo();

	string name = fileName;

	if(endsWith(name, ".rar")) {
		try {
			auto *a = Archive::open(name, "_files");
			for(const auto &s : *a) {
				a->extract(s);
				name = "_files/" + s;
				LOGD("Extracted %s", name);
				break;
			}
		} catch(archive_exception &ae) {
			player = nullptr;
			return false;
		}
	}

	player = nullptr;
	player = fromFile(name);

	dontPlay = false;
	playEnded = false;

	if(player) {

		fifo.clear();
		fadeOutPos = 0;
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
	si.numtunes = player->getMetaInt("songs");
	si.starttune = player->getMetaInt("startSong");
	if(si.starttune == -1) si.starttune = 0;

	length = player->getMetaInt("length");
	message = player->getMeta("message");
	sub_title = player->getMeta("sub_title");
	playingInfo = si;
}

void MusicPlayer::pause(bool dopause) {
	if(dopause)
		AudioPlayer::pause_audio();
	else
		AudioPlayer::resume_audio();
	paused = dopause;
}

string MusicPlayer::getMeta(const string &what) {
	if(what == "message") {
		return message;
	} else if(what == "sub_title") {
		return sub_title;
	}

	if(player)
		return player->getMeta(what);
	else
		return "";
}

uint16_t *MusicPlayer::getSpectrum() {
	LOCK_GUARD(fftMutex);
	auto delay = AudioPlayer::get_delay();
	if(fft.size() > delay) {
		while(fft.size() > delay + 4) {
			fft.popLevels();
		}
		spectrum = fft.getLevels();
		fft.popLevels();
	}
	return &spectrum[0];
}

void MusicPlayer::setVolume(float v) {
	volume = clamp(v);
	AudioPlayer::set_volume(volume * 100);
}

float MusicPlayer::getVolume() const {
	return volume;
}

vector<string> MusicPlayer::getSecondaryFiles(const string &name) {

	File file{name};
	if(file.exists()) {
		PSFFile f{name};
		if(f.valid()) {
			LOGD("IS PSF");
			const string tagNames[] = {"_lib", "_lib2", "_lib3", "_lib4"};
			vector<string> libFiles;
			for(int i = 0; i < 4; i++) {
				auto lib = f.tags()[tagNames[i]];
				if(lib != "") {
					makeLower(lib);
					libFiles.push_back(lib);
				}
			}
			return libFiles;
		}

		for(auto &plugin : ChipPlugin::getPlugins()) {
			if(plugin->canHandle(name)) {
				return plugin->getSecondaryFiles(file);
			}
		}
	}
	return vector<string>();
}

// PRIVATE

shared_ptr<ChipPlayer> MusicPlayer::fromFile(const string &fileName) {
	shared_ptr<ChipPlayer> player;
	string name = fileName;
	utils::makeLower(name);
	checkSilence = true;
	for(auto &plugin : ChipPlugin::getPlugins()) {
		if(plugin->canHandle(name)) {
			LOGD("Playing with %s\n", plugin->name());
			player = shared_ptr<ChipPlayer>(plugin->fromFile(fileName));
			if(!player)
				continue;
			checkSilence = plugin->checkSilence();
			break;
		}
	}
	return player;
}

shared_ptr<ChipPlayer> MusicPlayer::fromStream(const string &fileName) {
	shared_ptr<ChipPlayer> player;
	string name = fileName;
	utils::makeLower(name);
	checkSilence = true;
	for(auto &plugin : ChipPlugin::getPlugins()) {
		if(plugin->canHandle(name)) {
			LOGD("Playing with %s\n", plugin->name());
			player = shared_ptr<ChipPlayer>(plugin->fromStream());
			checkSilence = plugin->checkSilence();
			break;
		}
	}
	return player;
}

void MusicPlayer::putStream(const uint8_t *ptr, int size) {
	player->putStream(ptr, size);
}

void MusicPlayer::setParameter(const std::string &name, int what) {
	player->setParameter(name, what);
}

} // namespace

#ifdef CM_UNIT_TEST

#include "catch.hpp"

TEST_CASE("chipmachine::MusicPlayer", "") {

	using namespace utils;
	using namespace std;
	using namespace chipmachine;

	MusicPlayer mp(".");

	REQUIRE(mp.playFile("music/Amiga/Starbuck - Tennis.mod") == true);


	//REQUIRE(file.getName() == "temp.text"i);
}

#endif
