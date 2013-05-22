
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include "modplug.h"

#include <vector>
#include <string>
#include <memory>
//#include <thread>
//#include <mutex>

#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "zip.h"
#include "common/Fifo.h"
#include "utils.h"

#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif


typedef unsigned int uint;
using namespace std;
using namespace utils;


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

	int getSamples(short *target, int noSamples) override {
		return ModPlug_Read(mod, (void*)target, noSamples*2) / 2;
	}

private:
	ModPlugFile *mod;
};

extern "C" {
#include "sexypsf/driver.h"
}

static Fifo *sexyFifo;

void sexyd_update(unsigned char *pSound, long lBytes)
{
	if(sexyFifo)
		sexyFifo->putBytes((char*)pSound, lBytes);
}


class PSXPlayer : public ChipPlayer {
public:
	PSXPlayer(const string &fileName) : fifo(1024 * 128) {
		char temp[1024];
		strcpy(temp, fileName.c_str());
		psfInfo = sexy_load(temp);
		sexyFifo = &fifo;
	}

	int getSamples(short *target, int noSamples) override {
		while(fifo.filled() < noSamples*2) {
			int rc = sexy_execute();
			if(rc <= 0)
				return rc;
		}
		if(fifo.filled() == 0)
			return 0;

		return fifo.getShorts(target, noSamples);
	}
private:
	Fifo fifo;
	PSFINFO *psfInfo;
};

class PlayerSystem : public PlayerFactory {
	ChipPlayer *fromFile(File &file) override {
		if(file.getName().rfind(".psf") != string::npos)
			return new PSXPlayer {file.getName()};
		return new ModPlayer {file.getPtr(), file.getSize()};
	}
};

/*
Should handle; url parsing, http gets, archive extraction, local caching

URL FORMAT:

[file:/|http:/|]<filePath>:<SongPath>:<songStart>
eg
http://somesite.com/music/sids.zip:/Hubbard/commando.sid:2
file://mdat.monkey+smp.monkey



*/

class ZipFile {
public:
	ZipFile(const string &fileName) {
		zipFile = zip_open(fileName.c_str(), 0, NULL);
	}

	~ZipFile() {
		if(zipFile)
			close();
	}

	void close() {
		zip_close(zipFile);
		zipFile = nullptr;
	}

	void extract(const string &name) {
		//int e = zip_name_locate(zipFile, name, ZIP_FL_NOCASE);

		int no = zip_get_num_files(zipFile);
		for(int i=0; i<no; i++) {
			struct zip_stat sz;
			zip_stat_index(zipFile, i, 0, &sz);
			printf("Extracting %s (%d)\n", sz.name, (int)sz.size);
			struct zip_file *zf = zip_fopen_index(zipFile, i, 0);
			uint8_t *ptr = new uint8_t [sz.size] ;
			/*int rc =*/ zip_fread(zf, ptr, sz.size);
			zip_fclose(zf);
			delete [] ptr;
		}
		//const char *name = zip_get_name(zipFile, e, 0 /* ZIP_FL_UNCHANGED */ );
	}

private:
	struct zip *zipFile;
};

class Extractor {
	Extractor(const string &fileName) {

	};
};


int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("Modplayer test\n");


	URLPlayer urlPlayer {argv[1], new PlayerSystem()};
	ChipPlayer *player = &urlPlayer; 

	AudioPlayerNative ap;

	int bufSize = 4096;
	vector<short> buffer(bufSize);
	while(true) {
		int rc = player->getSamples(&buffer[0], bufSize);
		if(rc > 0)
			ap.writeAudio(&buffer[0], rc);
	}

	//waveOutClose(hWaveOut);
	return 0;

}