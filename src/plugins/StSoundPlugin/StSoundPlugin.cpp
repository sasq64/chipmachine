
#include "StSoundPlugin.h"
#include "StSoundLibrary/StSoundLibrary.h"
#include "../../ChipPlayer.h"

#include <coreutils/utils.h>
//#include <set>
//#include <unordered_map>

using namespace std;

class StSoundPlayer : public ChipPlayer {
public:
	StSoundPlayer(uint8_t *data, int size) {

		ymMusic = ymMusicCreate();
		ymMusicLoadMemory(ymMusic, data, size);

		//ymMusicInfo_t info;
		//ymMusicGetInfo(pMusic,&info);
		//printf("Name.....: %s\n",info.pSongName);
		//printf("Author...: %s\n",info.pSongAuthor);
		//printf("Comment..: %s\n",info.pSongComment);
		//printf("Duration.: %d:%02d\n",info.musicTimeInSec/60,info.musicTimeInSec%60);
		//printf("Driver...: %s\n", info.pSongPlayer);
		//ymMusicSetLoopMode(pMusic,YMTRUE);
		ymMusicPlay(ymMusic);

		//setMetaData("length", ModPlug_GetLength(mod) / 1000);
		//ymMusicStop(ymMusic);

	}
	~StSoundPlayer() override {
		if(ymMusic)
			ymMusicDestroy(ymMusic);
		ymMusic = nullptr;
	}

	virtual int getSamples(int16_t *target, int noSamples) override {

		noSamples /= 2;
		ymMusicCompute(ymMusic, target, noSamples);
		// Mono to stereo
		for(int i=noSamples-1; i>=0; i--) {
			target[i*2] = target[i];
			target[i*2+1] = target[i];
		}
		return noSamples*2;
	}

	virtual void seekTo(int song, int seconds) {
		//if(mod)
		//	ModPlug_Seek(mod, seconds * 1000);
		ymMusicSeek(ymMusic, seconds * 1000);
	}

private:
	YMMUSIC *ymMusic;
};

//static const set<string> supported_ext { "mod", "xm", "s3m" , "okt", "it", "ft" };

bool StSoundPlugin::canHandle(const std::string &name) {
	return utils::path_extention(name) == "ym";
}

ChipPlayer *StSoundPlugin::fromFile(const std::string &fileName) {
	utils::File file { fileName };
	return new StSoundPlayer {file.getPtr(), file.getSize()};
};
