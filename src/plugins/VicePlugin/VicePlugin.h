#ifndef VICE_PLAYER_H
#define VICE_PLAYER_H

#include "../../ChipPlugin.h"

#include <string>

namespace chipmachine {

class VicePlugin : public ChipPlugin {
public:
	VicePlugin(const std::string &dataDir);
	VicePlugin(const unsigned char *data);
	virtual ~VicePlugin();
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

}

#endif // VICE_PLAYER_H