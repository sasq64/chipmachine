#ifndef VICE_PLAYER_H
#define VICE_PLAYER_H

#include "ChipPlugin.h"
#include "utils.h"

class ChipPlayer;

#include <string>

class VicePlugin : public ChipPlugin {
public:
	VicePlugin();
	virtual bool canHandle(const std::string &name);
	virtual ChipPlayer *fromFile(utils::File &file) override;
};

#endif // VICE_PLAYER_H