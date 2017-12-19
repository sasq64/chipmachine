#pragma once

#include <musicplayer/chipplugin.h>

namespace chipmachine {

class GZPlugin : public musix::ChipPlugin {
public:
	GZPlugin() {}
	GZPlugin(std::vector<std::shared_ptr<musix::ChipPlugin>> &plugins) : plugins(plugins) {}
	virtual std::string name() const override { return "GZPlugin"; }
	virtual musix::ChipPlayer *fromFile(const std::string &fileName) override;

	virtual bool canHandle(const std::string &name) override;

private:
	std::vector<std::shared_ptr<musix::ChipPlugin>> plugins;
};

}