
#include "GMEPlugin.h"
#include "../../ChipPlayer.h"
#include "../../utils.h"

#include "gme/gme.h"

#include <set>

using namespace std;

class GMEPlayer : public ChipPlayer {
public:
	GMEPlayer(const string &fileName) : emu(nullptr), started(false) {

		gme_err_t err = gme_open_file(fileName.c_str(), &emu, 44100);
		if(!err) {
		    //gme_info_t* track0;
		    //gme_info_t* track1;
			//err = gme_track_info(emu, &track0, 0);
			int track_count = gme_track_count(emu);
		}


	}
	~GMEPlayer() override {
		if(emu)
			gme_delete(emu);
	}

	int getSamples(int16_t *target, int noSamples) override {
		gme_err_t err;
		if(!started) {
			 err = gme_start_track(emu, 0);
			started = true;
		}

		if(gme_track_ended(emu)) {
			return -1;
		}

		err = gme_play(emu, noSamples, target);

		return noSamples;
	}

	virtual void seekTo(int song, int seconds) override {
		if(song >= 0) {
			gme_err_t err = gme_start_track(emu, song);
			started = true;
			//gme_info_t* track;
			//err = gme_track_info(info->emu, &track, tune);
		    //gme_free_info(info->lastTrack);
			//info->lastTrack = track;
		}
		if(seconds >= 0)
			gme_seek(emu, seconds);
	}

private:
	Music_Emu *emu;
	bool started;
};

static const set<string> ext = { ".spc", ".gym", ".nsf", ".nsfe", ".gbs", ".ay", ".sap", ".vgm", ".vgz", ".hes", ".kss" };

bool GMEPlugin::canHandle(const std::string &name) {
	for(string x : ext) {
		if(utils::endsWith(name, x))
			return true;
	}
	return false;
}

ChipPlayer *GMEPlugin::fromFile(const std::string &name) {
	return new GMEPlayer { name };
};
