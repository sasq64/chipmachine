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

#include <emscripten.h>

void psid_play(short *buf, int size);
const char *psid_get_name();
const char *psid_get_author();
const char *psid_get_copyright();
int psid_tunes(int* default_tune);

static int videomode_is_ntsc;
static int videomode_is_forced;
static int sid;
static int sid_is_forced;

int console_mode = 1;
int vsid_mode = 1;
int video_disabled_mode = 1;

int init_vice(const char *c64Dir) {
	printf("Initing vice from %s", c64Dir);
	maincpu_early_init();
	machine_setup_context();
	drive_setup_context();
	machine_early_init();
	sysfile_init("C64");
	gfxoutput_early_init();
	if(init_resources() < 0) {
		archdep_startup_log_error("Failed to init resources");
		return -1;
	}

	if(resources_set_defaults() < 0) {
		archdep_startup_log_error("Cannot set defaults.\n");
		return -2;
	}

	resources_set_int("SidResidSampling", 0);
	resources_set_int("VICIIVideoCache", 0);
	resources_set_string("Directory", c64Dir);
	if(init_main() < 0) {
		archdep_startup_log_error("Failed to init main");
		return -3;
	}

	fprintf(stderr, "All is OK");
	return 0;
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
    if (videomode_is_forced) {
        resources_set_int("MachineVideoStandard", videomode_is_ntsc ? MACHINE_SYNC_NTSC : MACHINE_SYNC_PAL);
    }
    
    /* Force the SID model if told to in the settings */
    if (sid_is_forced) {
        resources_set_int("SidModel", sid);
    }
}

// worker functions

void init_sid(char *data, int size) {
/*	char name[size+1];
	memcpy(name, data, size);
	name[size] = 0;
	printf(stderr, "Setting dir to %s", name);

	FILE *fp = fopen("data/c64/kernal", "rb");
	if(!fp) {
		emscripten_worker_respond("ACCESS", 7);
		return;
	}
	fclose(fp);
*/
	mkdir("c64", 0);
	FILE *fp = fopen("c64/kernal", "wb");
	fwrite(&data[0], 1, 8192, fp);
	fclose(fp);
	fp = fopen("c64/basic", "wb");
	fwrite(&data[8192], 1, 8192, fp);
	fclose(fp);
	fp = fopen("c64/chargen", "wb");
	fwrite(&data[8192*2], 1, 4096, fp);
	fclose(fp);

	if(init("c64") == 0) {
		emscripten_worker_respond("OK", 3);
	} else {
		emscripten_worker_respond("FAIL", 5);
	}
}

void load_sid_song(char *data, int size) {

	FILE *fp = fopen("worker.sid", "wb");
	fwrite(data, 1, size, fp);
	fclose(fp);
	//char name[size+1];
	//memcpy(name, data, size);
	//name[size] = 0;
	int ret = psid_load_file("worker.sid");
	if (ret == 0) {
		int defaultSong;
		int songs = psid_tunes(&defaultSong);
		c64_song_init();
		emscripten_worker_respond("OK", 3);
	} else
		emscripten_worker_respond("FAIL", 5);
}

//#define BUFSIZE 4096

void play_sid_song(char *data, int size) {
	static char buffer[512*1024];
	int l = atoi(data);
	psid_play((short*)buffer, l/2);
	emscripten_worker_respond(buffer, l);
}

//int main() {
//	return 0;
//}