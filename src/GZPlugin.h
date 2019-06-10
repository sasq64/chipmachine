#pragma once

#include <musicplayer/chipplugin.h>

namespace chipmachine {

class GZPlugin : public musix::ChipPlugin
{
public:
    GZPlugin() = default;
    explicit GZPlugin(std::vector<std::shared_ptr<musix::ChipPlugin>>& plugins)
        : plugins(plugins)
    {}
    [[nodiscard]] virtual std::string name() const override
    {
        return "GZPlugin";
    }
    musix::ChipPlayer* fromFile(const std::string& fileName) override;

    bool canHandle(const std::string& name) override;

private:
    std::vector<std::shared_ptr<musix::ChipPlugin>> plugins;
};

} // namespace chipmachine
