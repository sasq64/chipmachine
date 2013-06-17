#ifndef VICE_PLAYER_H
#define VICE_PLAYER_H

#include "../../ChipPlugin.h"

class ChipPlayer;

#include <string>

class VicePlugin : public ChipPlugin {
public:
	VicePlugin();
	virtual ~VicePlugin();
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

#endif // VICE_PLAYER_H