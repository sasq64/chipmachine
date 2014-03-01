#ifndef UADEPLUGIN_H
#define UADEPLUGIN_H

#include "../../ChipPlugin.h"

namespace chipmachine {

class UADEPlugin : public ChipPlugin {
public:
	virtual std::string name() const override { return "UADE"; }
	//UADEPlugin();
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

}

#endif // UADEPLUGIN_H