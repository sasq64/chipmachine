#include <memory>
#include <musicplayer/chipplugin.h>
#include <string>

#include "../sol2/sol.hpp"

class YoutubePlugin : public musix::ChipPlugin
{
public:
    YoutubePlugin(sol::state& lua) : lua(lua)
    {
        plugin = musix::ChipPlugin::getPlugin("ffmpeg");
    }

    virtual musix::ChipPlayer* fromFile(const std::string& fileName) override
    {
        LOGD("Youtube plugin %s", fileName);
        std::string x = lua["on_parse_youtube"](fileName);
        LOGD("LUA reports %s", x);
        if (x == "")
            return nullptr;
        auto player = plugin->fromFile(x);
        LOGD("OK");
        auto dpos = x.find("dur=");
        if (dpos != std::string::npos) {
            int length = atoi(x.substr(dpos + 4).c_str());
            player->setMeta("length", length);
        }
        return player;
    }

    virtual bool canHandle(const std::string& name) override
    {
        return utils::startsWith(name, "http") &&
               name.find("youtu") != std::string::npos;
    }

    virtual std::string name() const override { return "youtube"; }

    sol::state& lua;
    std::shared_ptr<musix::ChipPlugin> plugin;
};

void initYoutube(sol::state& lua)
{
    musix::ChipPlugin::addPlugin(std::make_shared<YoutubePlugin>(lua));
}
