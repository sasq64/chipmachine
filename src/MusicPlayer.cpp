
#include "MusicPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/format.h>

#include <audioplayer/audioplayer.h>

#include <musicplayer/plugins/MP3Plugin/MP3Plugin.h>
#include <musicplayer/plugins/OpenMPTPlugin/OpenMPTPlugin.h>
#include <musicplayer/plugins/HTPlugin/HTPlugin.h>
#include <musicplayer/plugins/HEPlugin/HEPlugin.h>
#include <musicplayer/plugins/GSFPlugin/GSFPlugin.h>
#include <musicplayer/plugins/NDSPlugin/NDSPlugin.h>
#include <musicplayer/plugins/USFPlugin/USFPlugin.h>
#include <musicplayer/plugins/VicePlugin/VicePlugin.h>
#include <musicplayer/plugins/SexyPSFPlugin/SexyPSFPlugin.h>
#include <musicplayer/plugins/GMEPlugin/GMEPlugin.h>
#include <musicplayer/plugins/SC68Plugin/SC68Plugin.h>
#include <musicplayer/plugins/StSoundPlugin/StSoundPlugin.h>
#include <musicplayer/plugins/AdPlugin/AdPlugin.h>

#ifndef NO_UADE
#include <musicplayer/plugins/UADEPlugin/UADEPlugin.h>
#endif

#include <archive/archive.h>
#include <set>
#include <algorithm>

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
		//player->onMeta([=](const vector<string> &meta, ChipPlayer* player) {			
			//for(const auto &m : meta) {
			//	setMeta(m, player->getMeta(m));
			//}
		//});
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
			//"game", player->getMeta("game"),
			//"composer", player->getMeta("composer"),
			"length", player->getMeta("length")
			//"format", player->getMeta("format")
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

MusicPlayer::MusicPlayer() : fifo(32786), plugins {
		//new ModPlugin {},
		make_shared<RSNPlugin>(this->plugins),
		make_shared<MP3Plugin>(),
		make_shared<OpenMPTPlugin>(),
		make_shared<HTPlugin>(),
		make_shared<HEPlugin>(find_file("data/hebios.bin")),
		make_shared<GSFPlugin>(),
		make_shared<NDSPlugin>(),
		make_shared<USFPlugin>(),
		make_shared<VicePlugin>(find_file("data/c64")),
		make_shared<SexyPSFPlugin>(),
		make_shared<GMEPlugin>(),
		make_shared<SC68Plugin>(find_file("data/sc68")),
		make_shared<StSoundPlugin>(),
		make_shared<AdPlugin>(),
#ifndef NO_UADE
		make_shared<UADEPlugin>()
#endif
	}
{
	dontPlay = playEnded = false;
	AudioPlayer::play([=](int16_t *ptr, int size) mutable {

		if(dontPlay) {
			memset(ptr, 0, size*2);
			return;
		}

		LOCK_GUARD(playerMutex);

		//if(fadeOutPos > 0 && fadeOutPos <= pos) {
		//	player = nullptr;
		//}

		if(!paused && player) {
			int sz = size;

			sub_title = player->getMeta("sub_title");

			silentFrames = fifo.getSilence();

			while(true) {
				if(sz <= 0)
					LOGD("WTF!");
				int rc = player->getSamples((int16_t*)fifo.ptr(), sz);				
				if(rc <= 0) {
					playEnded = true;
					memset(fifo.ptr(), 0, sz*2);
					rc = sz;
				}

				//LOGD("SILENCE %d", fifo.getSilence());
				if(fadeOutPos != 0) {
					fifo.setVolume((fadeOutPos - pos) / (float)fadeLength);
				}

				fifo.processShorts(nullptr, rc);
				pos += rc/2;
				sz -= rc;
				if(fifo.filled() >= size*2) {
					fifo.getShorts(ptr, size);
					lock_guard<mutex> guard(fftMutex);
					fft.addAudio(ptr, size);
					break;
				}
			}

		} else
			memset(ptr, 0, size*2);
	});
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

		player->onMeta([=](const std::vector<std::string> &meta, ChipPlayer *player) {
			
		});

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