
#include "GMEPlugin.h"
#include "../../ChipPlayer.h"
#include <coreutils/utils.h>

#include "gme/gme.h"

#include <set>

using namespace std;

namespace chipmachine {

class GMEPlayer : public ChipPlayer {
public:
	GMEPlayer(const string &fileName) : emu(nullptr), started(false) {

		gme_err_t err = gme_open_file(fileName.c_str(), &emu, 44100);
		if(!err) {
		   	gme_info_t* track0;
		    //gme_info_t* track1;
			gme_track_info(emu, &track0, 0);
			//int track_count = gme_track_count(emu);

			//meta["title"] = track0->game;

			setMeta(
				"title", track0->game,
				"composer", track0->author,
				"copyright", track0->copyright,
				"length", track0->length,
				"songTitle", track0->song,
				"format", track0->system,
				"songs", gme_track_count(emu)
			);
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
			gme_info_t* track;
			gme_track_info(emu, &track, song);
			setMeta("songTitle", track->song);

			/* gme_err_t err = */ gme_start_track(emu, song);
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

static const set<string> supported_ext = { "emul", "spc", "gym", "nsf", "nsfe", "gbs", "ay", "sap", "vgm", "vgz", "hes", "kss" };

bool GMEPlugin::canHandle(const std::string &name) {
	return supported_ext.count(utils::path_extention(name)) > 0;
}

ChipPlayer *GMEPlugin::fromFile(const std::string &name) {
	return new GMEPlayer { name };
};

} // namespace chipmachine
