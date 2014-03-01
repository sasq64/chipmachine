
extern "C" {
#include "archdep.h"
#include "drive.h"
#include "gfxoutput.h"
#include "init.h"
#include "initcmdline.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "psid.h"
#include "resources.h"
#include "sound.h"
#include "sysfile.h"


void psid_play(short *buf, int size);
const char *psid_get_name();
const char *psid_get_author();
const char *psid_get_copyright();
int psid_tunes(int* default_tune);

}

#include "VicePlugin.h"
#include "../../ChipPlayer.h"
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <set>
#include <algorithm>

int console_mode = 1;
int vsid_mode = 1;
int video_disabled_mode = 1;

namespace chipmachine {

using namespace std;
using namespace utils;


static bool videomode_is_ntsc;
static bool videomode_is_forced;
static int sid;
static bool sid_is_forced;

class VicePlayer : public ChipPlayer {
public:
	static bool init(const string &c64Dir) {
		maincpu_early_init();
		machine_setup_context();
		drive_setup_context();
		machine_early_init();
		sysfile_init("C64");
		gfxoutput_early_init();
		if(init_resources() < 0) {
			archdep_startup_log_error("Failed to init resources");
			return false;
		}

		if(resources_set_defaults() < 0) {
			archdep_startup_log_error("Cannot set defaults.\n");
			return false;
		}

		resources_set_int("SidResidSampling", 0);
		resources_set_int("VICIIVideoCache", 0);
		resources_set_string("Directory", c64Dir.c_str());
		if(init_main() < 0) {
			archdep_startup_log_error("Failed to init main");
			return false;
		}

		return true;
	}

	static void c64_song_init()
	{
	    /* Set default, potentially overridden by reset. */
	    resources_set_int("MachineVideoStandard", videomode_is_ntsc ? MACHINE_SYNC_NTSC : MACHINE_SYNC_PAL);
	    
	    /* Default to 6581 in case tune doesn't specify. */
	    resources_set_int("SidModel", sid);

	    /* Reset C64, which also initializes PSID for us. */
	    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);

	    /* Now force video mode if we are asked to. */
	    if (videomode_is_forced)
	    {
	        resources_set_int("MachineVideoStandard", videomode_is_ntsc ? MACHINE_SYNC_NTSC : MACHINE_SYNC_PAL);
	    }
	    
	    /* Force the SID model if told to in the settings */
	    if (sid_is_forced)
	    {
	        resources_set_int("SidModel", sid);
	    }

	}

	VicePlayer(const string &sidFile) {
		int ret = psid_load_file(sidFile.c_str());
		LOGD("Loaded %s -> %d", sidFile, ret);
		if (ret == 0) {

			int defaultSong;
			int songs = psid_tunes(&defaultSong);

			setMeta(
				"title", psid_get_name(),
				"composer", psid_get_author(),
				"copyright", psid_get_copyright(),
				"songs", songs,
				"startSong", defaultSong-1
			);

			c64_song_init();
		}
	}

	~VicePlayer() {
		LOGD("Viceplayer destroy");
		psid_set_tune(-1);
	}

	virtual int getSamples(int16_t *target, int size) {
		psid_play(target, size);
		return size;
	}

	virtual void seekTo(int song, int seconds) override {
		if(song >= 0) {
			psid_set_tune(song+1);
    		c64_song_init();
		}
	}


};

VicePlugin::VicePlugin(const string &dataDir) {
	VicePlayer::init(dataDir.c_str());
}

VicePlugin::VicePlugin(const unsigned char *data) {
	mkdir("c64", 0777);

	FILE *fp;
	fp = fopen("c64/basic", "wb");
	fwrite(&data[0], 1, 8192, fp);
	fclose(fp);

	fp = fopen("c64/chargen", "wb");
	fwrite(&data[8192], 1, 4096, fp);
	fclose(fp);

	fp = fopen("c64/kernal", "wb");
	fwrite(&data[8192+4096], 1, 8192, fp);
	fclose(fp);
	VicePlayer::init("c64");
}

VicePlugin::~VicePlugin() {
	LOGD("VicePlugin destroy\n");
	machine_shutdown();
}

static const set<string> ext { ".sid", ".psid", ".rsid" , ".2sid", ".mus" };

bool VicePlugin::canHandle(const std::string &name) {
	for(string x : ext) {
		if(utils::endsWith(name, x))
			return true;
	}
	return false;
}

ChipPlayer *VicePlugin::fromFile(const std::string &fileName) {
	return new VicePlayer { fileName };
}

}