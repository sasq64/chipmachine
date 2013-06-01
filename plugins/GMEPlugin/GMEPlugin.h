#ifndef GME_PLAYER_H
#define GME_PLAYER_H

#include "../../ChipPlugin.h"

class GMEPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};


#endif // GME_PLAYER_H