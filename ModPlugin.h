#ifndef MODPLAYER_H
#define MODPLAYER_H

#include "ChipPlayer.h"
#include "modplug.h"


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


class ModPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override {
		return endsWith(name, ".mod") || endsWith(name, ".xm");
	}

	virtual ChipPlayer *fromFile(File &file) override {
		return new ModPlayer {file.getPtr(), file.getSize()};
	}
};


#endif // MODPLAYER_H