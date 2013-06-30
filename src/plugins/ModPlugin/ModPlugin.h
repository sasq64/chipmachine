#ifndef MODPLAYER_H
#define MODPLAYER_H

#include "../../ChipPlugin.h"


class ModPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};


#endif // MODPLAYER_H