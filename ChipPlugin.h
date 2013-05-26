#ifndef CHIP_PLUGIN_H
#define CHIP_PLUGIN_H

#include <string>
#include <utils.h>

class ChipPlayer;

class ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) = 0;
	virtual ChipPlayer *fromFile(utils::File &file) = 0;
};

#endif // CHIP_PLUGIN_H