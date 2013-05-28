
#include "ModPlugin.h"
#include "modplug.h"
#include "ChipPlayer.h"

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

	int getSamples(int16_t *target, int noSamples) override {
		return ModPlug_Read(mod, (void*)target, noSamples*2) / 2;
	}

private:
	ModPlugFile *mod;
};


bool ModPlugin::canHandle(const std::string &name) {
	return utils::endsWith(name, ".mod") || utils::endsWith(name, ".xm");
}

ChipPlayer *ModPlugin::fromFile(utils::File &file) {
	return new ModPlayer {file.getPtr(), file.getSize()};
};
