
#include "ModPlugin.h"
#include "modplug.h"
#include "../../ChipPlayer.h"
#include "../../utils.h"

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

		//metaData[ChipPlayer::LENGTH] = to_string(ModPlug_GetLength() / 1000));
		//metaData[ChipPlayer::TITLE] = ModPlug_GetName(mod);
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

	//virtual unordered_map<string, string> getMetaData() {
	//	return metaData;
	//}


private:
	ModPlugFile *mod;
	//private unordered_map<string, string>;
};

static const set<string> ext { ".mod", ".xm", ".s3m" , ".okt", ".it", ".ft" };

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
