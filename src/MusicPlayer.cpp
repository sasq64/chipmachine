
#include "MusicPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/format.h>

#include <audioplayer/audioplayer.h>

#include <musicplayer/plugins/plugins.h>

#include <archive/archive.h>
#include <set>
#include <algorithm>

//#ifdef RASPBERRYPI
//#define AUDIO_DELAY 12
//#else
#define AUDIO_DELAY 2
//#endif

using namespace std;
using namespace utils;

namespace chipmachine {

class RSNPlayer : public ChipPlayer {
public:
	RSNPlayer(const vector<string> &l, shared_ptr<ChipPlugin> plugin) : songs(l), plugin(plugin) {
		LOGD("Playing with %s", plugin->name());
		player = shared_ptr<ChipPlayer>(plugin->fromFile(l[0]));
		if(player == nullptr)
			throw player_exception();
		setMeta("title", player->getMeta("title"),
			"sub_title", player->getMeta("sub_title"),
			"game", player->getMeta("game"),
			"composer", player->getMeta("composer"),
			"length", player->getMeta("length"),
			"format", player->getMeta("format"),
			"songs", l.size());
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		if(player)
			return player->getSamples(target, noSamples);
		return 0;
	}

	virtual bool seekTo(int song, int seconds) {
		player = nullptr;
		player = shared_ptr<ChipPlayer>(plugin->fromFile(songs[song]));
		if(player) {
			setMeta("sub_title", player->getMeta("sub_title"),
				"length", player->getMeta("length")
			);
			if(seconds > 0)
				player->seekTo(-1, seconds);
			return true;
		}
		return false;
	}
private:
	vector<string> songs;
	shared_ptr<ChipPlayer> player;
	shared_ptr<ChipPlugin> plugin;

};

class RSNPlugin : public ChipPlugin {
public:
	RSNPlugin(vector<shared_ptr<ChipPlugin>> &plugins) : plugins(plugins) {}

	virtual string name() const { return "RSNPlugin"; }

	virtual ChipPlayer *fromFile(const string &fileName) {

		static const set<string> song_formats { "spc", "psf", "minipsf", "psf2", "minipsf2", "miniusf", "dsf", "minidsf", "mini2sf", "minigsf" };

		vector<string> l;
		makedir(".rsn");
		for(auto f : File(".rsn").listFiles())
			f.remove();

		if(!File::exists(fileName))
			return nullptr;

		try {
			auto *a = Archive::open(fileName, ".rsn", Archive::TYPE_RAR);
			for(auto s : *a) {
				a->extract(s);
				if(song_formats.count(path_extension(s)) > 0) {				
					LOGD("Found %s", s);
					l.push_back(string(".rsn/") + s);
				}
			};
			delete a;
		} catch (archive_exception &e) {
			return nullptr;
		}

		sort(l.begin(), l.end());

		if(l.size() > 0) {
			for(auto name : l) {
				utils::makeLower(name);
				for(auto plugin : plugins) {
					if(plugin->name() != "UADE" && plugin->canHandle(name)) {
						try {
							return new RSNPlayer(l, plugin);
						} catch(player_exception &e) {
							LOGD("FAILED");
						}
					}
				}
			}
		}
		return nullptr;

	};



bool canHandle(const string &name) {
	static const set<string> supported_ext { "rsn", "rps", "rdc", "rds", "rgs", "r64" };
	return supported_ext.count(utils::path_extension(name)) > 0;
}

private:
	vector<shared_ptr<ChipPlugin>> &plugins;

};

static std::string find_file(const std::string &name) {
	auto f = File::findFile(current_exe_path() + ":" + File::getAppDir(), name);
	return f.getName();
}

MusicPlayer::MusicPlayer() : fifo(32786)/*, plugins {
		make_shared<MP3Plugin>(),
		make_shared<RSNPlugin>(this->plugins),
		make_shared<OpenMPTPlugin>(),
		make_shared<HTPlugin>(),
		make_shared<HEPlugin>(find_file("data/hebios.bin")),
		make_shared<GSFPlugin>(),
		make_shared<NDSPlugin>(),
		make_shared<USFPlugin>(),
		make_shared<VicePlugin>(find_file("data/c64")),
		make_shared<GMEPlugin>(),
		make_shared<SC68Plugin>(find_file("data/sc68")),
		make_shared<StSoundPlugin>(),
		make_shared<AdPlugin>(),
#ifndef NO_UADE
		make_shared<UADEPlugin>()
#endif
	} */
{

	plugins.push_back(make_shared<RSNPlugin>(plugins));
	ChipPlugin::createPlugins(find_file("data"), plugins);

	dontPlay = playEnded = false;
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {

		if(dontPlay) {
			memset(ptr, 0, size*2);
			return;
		}

		LOCK_GUARD(playerMutex);
		if(fifo.filled() >= size) {
			fifo.get(ptr, size);
			lock_guard<mutex> guard(fftMutex);
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

			//LOGD("LEFT %d", fifo.left());
			int f = fifo.left() - 128;

			if(f < 0)
				break; 
			int rc = player->getSamples(tempBuf, f);

			//LOGD("GOT %d", fifo.left());

			if(rc <= 0) {
				playEnded = true;
				break;
			}
			//if(tempBuf[32768] != 0x59)
			//	LOGE("CORRUPTION");


			//LOGD("SILENCE %d", fifo.getSilence());
			if(fadeOutPos != 0) {
				fifo.setVolume((fadeOutPos - pos) / (float)fadeLength);
			}

			//fifo.processShorts(nullptr, rc);
			//LOGD("Putting %d samples", rc);
			fifo.put(tempBuf, rc);
			pos += rc;
			if(fifo.filled() >= fifo.size()/2) {
				break;
			}

		}

	} else
		sleepms(100);

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
		//LOGD("GET");
		spectrum = fft.getLevels();
		fft.popLevels();

	} //else LOGD("WAIT");
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