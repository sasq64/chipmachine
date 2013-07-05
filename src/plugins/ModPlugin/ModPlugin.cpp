
#include "ModPlugin.h"
#include "modplug.h"
#include "../../ChipPlayer.h"

#include <utils/utils.h>
#include <set>
#include <unordered_map>

using namespace std;

class ModPlayer : public ChipPlayer {
public:
	ModPlayer(uint8_t *data, int size) {

		ModPlug_Settings settings;
		ModPlug_GetSettings(&settings);
		settings.mChannels = 2;
		settings.mFrequency = 44100;
		settings.mBits = 16;
		settings.mLoopCount = -1;
		ModPlug_SetSettings(&settings);
		mod = ModPlug_Load(data, size);

		setMetaData("length", ModPlug_GetLength(mod) / 1000);
		int type = ModPlug_GetModuleType(mod);

	}
	~ModPlayer() override {
		if(mod)
			ModPlug_Unload(mod);
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		return ModPlug_Read(mod, (void*)target, noSamples*2) / 2;
	}

	virtual void seekTo(int song, int seconds) {
		if(mod)
			ModPlug_Seek(mod, seconds * 1000);
	}

private:
	ModPlugFile *mod;
};

static const set<string> supported_ext { "mod", "xm", "s3m" , "okt", "it", "ft" };

bool ModPlugin::canHandle(const std::string &name) {
	return supported_ext.count(utils::path_extention(name)) > 0;
}

ChipPlayer *ModPlugin::fromFile(const std::string &fileName) {
	utils::File file { fileName };
	return new ModPlayer {file.getPtr(), file.getSize()};
};
