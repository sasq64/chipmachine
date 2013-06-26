#ifndef SC68PLAYER_H
#define SC68PLAYER_H

#include "../../ChipPlugin.h"

class SC68Plugin : public ChipPlugin {
public:
	SC68Plugin(const std::string &dataDir) : dataDir(dataDir) {}
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;

	void setDataDir(const std::string &dataDir) {
		this->dataDir = dataDir;
	}
private:
	std::string dataDir;
};


#endif // SC68PLAYER_H