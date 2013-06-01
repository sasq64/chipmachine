extern "C" {
#include "sexypsf/driver.h"
}
#include "SexyPSFPlugin.h"
#include "../../common/Fifo.h"

#include "../../ChipPlayer.h"
#include "../../utils.h"

static Fifo *sexyFifo;

void sexyd_update(unsigned char *pSound, long lBytes)
{
	if(sexyFifo)
		sexyFifo->putBytes((char*)pSound, lBytes);
}

class SexyPSFPlayer : public ChipPlayer {
public:
	SexyPSFPlayer(const std::string &fileName) : fifo(1024 * 128) {
		char *temp = new char [fileName.length()+1];
		strcpy(temp, fileName.c_str());
		psfInfo = sexy_load(temp);
		sexyFifo = &fifo;
		delete [] temp;
	}

	~SexyPSFPlayer() {
		sexy_freepsfinfo(psfInfo);
		sexy_shutdown();
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

	virtual void seekTo(int song, int seconds) override {
	}

private:
	Fifo fifo;
	PSFINFO *psfInfo;
};


bool SexyPSFPlugin::canHandle(const std::string &name) override {
	return utils::endsWith(name, ".psf");
}

ChipPlayer *SexyPSFPlugin::fromFile(const std::string &name) override {
	return new SexyPSFPlayer { name };
}
