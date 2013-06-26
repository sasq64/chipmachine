
#include "SC68Plugin.h"
#include "../../ChipPlayer.h"
#include <utils/utils.h>

#include <string.h>

#include <sc68/sc68.h>
#include <sc68/msg68.h>
extern "C" {
int unice68_depacker(void * dest, const void * src);
int unice68_get_depacked_size(const void * buffer, int * p_csize);
}
#include <set>
#include <unordered_map>
#include <string>

using namespace std;


static void write_debug(int level, void *cookie, const char *fmt, va_list list) {
	static char temp[1024];	
	vsprintf(temp, fmt, list);
	LOGD(temp);
}

class SC68Player : public ChipPlayer {
public:
	SC68Player(uint8_t *data, int size, string dataDir) : sc68(nullptr), dataDir(dataDir), valid(false) {


		string head = string((char*)data, 0, 4);

		if(head == "ICE!") {
			int dsize = unice68_get_depacked_size(data, NULL);
			LOGD("Unicing %d bytes to %d bytes", size, dsize);
			uint8_t *ptr = new uint8_t [ dsize ];
			int res = unice68_depacker(ptr, data);
			if(res == 0) {
				valid = load(ptr, dsize);
			}

			delete [] ptr;

		} else {
			valid = load(data, size);
		}

	}

	bool load(uint8_t *ptr, int size) {

		sc68_init_t init68;
		memset(&init68, 0, sizeof(init68));
		init68.msg_handler = (sc68_msg_t)write_debug;

#ifdef _DEBUG
		init68.debug = vfprintf;
		init68.debug_cookie = stderr;
#endif

		if(sc68_init(&init68) != 0) {
			LOGW("Init failed");
			return false;
		}

		sc68 = sc68_create(NULL);
		sc68_set_user(sc68, dataDir.c_str());

		if(sc68_verify_mem(ptr, size) < 0) {
			LOGW("Verify mem failed");
			sc68_destroy(sc68);
			sc68_shutdown();
			return false;
		}

		if (sc68_load_mem(sc68, ptr, size)) {
			LOGW("Load mem failed");
			sc68_destroy(sc68);
			sc68_shutdown();
			return false;
		}

		trackChanged = false;

		sc68_play(sc68, 0, 0);
		if(sc68_process(sc68, NULL, 0) < 0) {
			LOGW("Process failed");
			sc68_destroy(sc68);
			sc68_shutdown();
			return false;
		}

		defaultTrack = sc68_play(sc68, -1, 0);

		if(defaultTrack == 0)
			defaultTrack = 1;

		currentTrack = defaultTrack;

		return true;
	}

	~SC68Player() override {
		if(sc68)
			sc68_destroy(sc68);
		sc68 = nullptr;
		sc68_shutdown();
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		const char *err = nullptr;
		while (NULL != (err = sc68_error_get(sc68))) {
			LOGW("ERROR: %s", err);
		}

		/* Set track number : command line is prior to config force-track */
		if(currentTrack < 0) {
			currentTrack = 0;
			if(sc68_play(sc68, currentTrack, 0)) {
				return -1;
			}
		}

		int n = noSamples/2;

		int code = sc68_process(sc68, target, &n);

		if(!trackChanged && (code & SC68_CHANGE)) {
			LOGD("Ending track");
			return -1;
		}

		trackChanged = false;

		if(code == SC68_ERROR) {
			return -1;
		}

		return noSamples;
	}

	virtual void seekTo(int song, int seconds) {

		if(song >= 0) {
			currentTrack = song+1;
			if(sc68_play(sc68, currentTrack, 0)) {
				currentTrack = -1;
				return;
			}
			trackChanged = true;
		}
		
		if(seconds >= 0) {
			int status;
			sc68_seek(sc68, seconds * 1000, &status);
		}
	}

private:
	sc68_t *sc68;
	sc68_music_info_t info;
	int currentTrack;
	int defaultTrack;
	bool trackChanged;
	string dataDir;
	bool valid;
};

static const set<string> supported_ext { "sndh", "sc68", "snd" };

bool SC68Plugin::canHandle(const std::string &name) {
	return supported_ext.count(utils::path_extention(name)) > 0;
}

ChipPlayer *SC68Plugin::fromFile(const std::string &fileName) {
	utils::File file { fileName };
	return new SC68Player {file.getPtr(), file.getSize(), dataDir};
};
