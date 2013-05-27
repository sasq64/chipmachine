
#ifndef PSXPLUGIN_H
#define PSXPLUGIN_H

extern "C" {
#include "sexypsf/driver.h"
}
#include "common/Fifo.h"

static Fifo *sexyFifo;

void sexyd_update(unsigned char *pSound, long lBytes)
{
	if(sexyFifo)
		sexyFifo->putBytes((char*)pSound, lBytes);
}


class PSXPlayer : public ChipPlayer {
public:
	PSXPlayer(const std::string &fileName) : fifo(1024 * 128) {
		char temp[1024];
		strcpy(temp, fileName.c_str());
		psfInfo = sexy_load(temp);
		sexyFifo = &fifo;
	}

	int getSamples(int16_t *target, int noSamples) override {
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

class PSFPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override {
		return utils::endsWith(name, ".psf");
	}

	virtual ChipPlayer *fromFile(utils::File &file) override {
		return new PSXPlayer {file.getName()};
	}
};

#endif // PSXPLUGIN_H