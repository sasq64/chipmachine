#include <musicplayer/chipplugin.h>
#include <luainterpreter/luainterpreter.h>
#include <string>
#include <memory>

using namespace chipmachine;

class YoutubePlugin : public ChipPlugin {
public:
	YoutubePlugin(LuaInterpreter &lua) : lua(lua) { plugin = ChipPlugin::getPlugin("ffmpeg"); }

	virtual ChipPlayer *fromFile(const std::string &fileName) override {
		LOGD("Youtube plugin %s", fileName);
		std::string x = lua.call<std::string>(std::string("on_parse_youtube"), fileName);
		LOGD("LUA reports %s", x);
		if(x == "")
			return nullptr;
		auto player = plugin->fromFile(x);
		LOGD("OK");
		auto dpos = x.find("dur=");
		if(dpos != std::string::npos) {
			int length = atoi(x.substr(dpos + 4).c_str());
			player->setMeta("length", length);
		}
		return player;
	}

	virtual bool canHandle(const std::string &name) override {
		return utils::startsWith(name, "http") && name.find("youtu") != std::string::npos;
	}

	virtual std::string name() const override { return "youtube"; }

	LuaInterpreter& lua;
	std::shared_ptr<ChipPlugin> plugin;
};

void initYoutube(LuaInterpreter& lua) {
	ChipPlugin::addPlugin(std::make_shared<YoutubePlugin>(lua));
}

