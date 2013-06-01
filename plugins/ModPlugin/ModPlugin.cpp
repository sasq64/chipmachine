
#include "ModPlugin.h"
#include "modplug.h"
#include "../../ChipPlayer.h"
#include "../../utils.h"

#include <set>

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
	}
	~ModPlayer() override {
		if(mod)
			ModPlug_Unload(mod);
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		return ModPlug_Read(mod, (void*)target, noSamples*2) / 2;
	}

	virtual void seekTo(int song, int seconds) {
	}


private:
	ModPlugFile *mod;
};

static const set<string> ext { ".mod", ".xm", ".s3m" , ".okt", ".it" };

bool ModPlugin::canHandle(const std::string &name) {
	for(string x : ext) {
		if(utils::endsWith(name, x))
			return true;
	}
	return false;
	//utils::endsWith(name, ".mod") || utils::endsWith(name, ".xm");
}

ChipPlayer *ModPlugin::fromFile(const std::string &fileName) {
	utils::File file { fileName };
	return new ModPlayer {file.getPtr(), file.getSize()};
};
