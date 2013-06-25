
#include "UADEPlugin.h"

#include "../../ChipPlayer.h"
#include "../../utils.h"

#include <uade/uade.h>

#include <set>
#include <unordered_map>

/*
extern "C" {
	void init_uade();
	int get_samples(uint8_t *target, int bytes);
	int play_song(const char *name);
	void exit_song();
	void set_song(int song);
}*/

using namespace std;

class UADEPlayer : public ChipPlayer {
public:
	UADEPlayer() : valid(false), state(nullptr)  {
	}

	bool load(string fileName) {

		state = uade_new_state(nullptr);
		if(uade_play(fileName.c_str(), -1, state) == 1) {
			songInfo = uade_get_song_info(state);
			setMetaData("songs", songInfo->subsongs.max - songInfo->subsongs.min + 1);
			setMetaData("startsong", songInfo->subsongs.def - songInfo->subsongs.min);
			setMetaData("length", songInfo->duration);
			setMetaData("title", songInfo->modulename);
			setMetaData("format", songInfo->formatname);

			valid = true;
		} 
		return valid;

	}

	~UADEPlayer() override {
		//if(valid)
		//	exit_song();
	   uade_stop(state);
	   uade_cleanup_state(state);
	   state = nullptr;
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		//int rc = get_samples((uint8_t*)target, noSamples * 2);
		ssize_t rc = uade_read(target, noSamples*2, state); 
		if(rc > 0)
			return rc/2;
		return rc;
	}

	virtual void seekTo(int song, int seconds) {
		uade_seek(UADE_SEEK_SUBSONG_RELATIVE, seconds, song + songInfo->subsongs.min, state);
		//set_song(song);	
	}

	//virtual unordered_map<string, string> getMetaData() {
	//	return metaData;
	//}

private:
	bool valid;
	struct uade_state *state;
	const struct uade_song_info *songInfo;
	//ModPlugFile *mod;
	//private unordered_map<string, string>;
};

//static const set<string> ext { ".mod", ".xm", ".s3m" , ".okt", ".it", ".ft" };

bool UADEPlugin::canHandle(const std::string &name) {

	return true;

	/*for(string x : ext) {
		if(utils::endsWith(name, x))
			return true;
	}*/
	//return false;
	//utils::endsWith(name, ".mod") || utils::endsWith(name, ".xm");
}

ChipPlayer *UADEPlugin::fromFile(const std::string &fileName) {
//	utils::File file { fileName };
	auto *player = new UADEPlayer();
	if(!player->load(fileName)) {
		delete player;
		player = nullptr;
	}
	return player;
};

UADEPlugin::UADEPlugin() {
	//init_uade();
}
