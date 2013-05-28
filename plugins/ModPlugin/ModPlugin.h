#ifndef MODPLAYER_H
#define MODPLAYER_H

#include "ChipPlugin.h"
#include "utils.h"

class ModPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(utils::File &file) override;
};


#endif // MODPLAYER_H