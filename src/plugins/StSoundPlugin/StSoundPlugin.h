#ifndef STPLAYER_H
#define STPLAYER_H

#include "../../ChipPlugin.h"


class StSoundPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};


#endif // STPLAYER_H