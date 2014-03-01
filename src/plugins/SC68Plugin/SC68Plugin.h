#ifndef SC68PLAYER_H
#define SC68PLAYER_H

#include "../../ChipPlugin.h"

namespace chipmachine {

class SC68Plugin : public ChipPlugin {
public:
	virtual std::string name() const override { return "SC68"; }
	SC68Plugin(const std::string &dataDir) : dataDir(dataDir) {}
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;

	void setDataDir(const std::string &dataDir) {
		this->dataDir = dataDir;
	}
private:
	std::string dataDir;
};

}

#endif // SC68PLAYER_H