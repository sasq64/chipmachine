#ifndef CHIP_PLUGIN_H
#define CHIP_PLUGIN_H

#include <string>

class ChipPlayer;

class ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) = 0;
	virtual ChipPlayer *fromFile(const std::string &fileName) = 0;
};

#endif // CHIP_PLUGIN_H