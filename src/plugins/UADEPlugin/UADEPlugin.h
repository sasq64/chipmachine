#ifndef UADEPLUGIN_H
#define UADEPLUGIN_H

#include "../../ChipPlugin.h"

class UADEPlugin : public ChipPlugin {
public:
	UADEPlugin();
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};


#endif // UADEPLUGIN_H