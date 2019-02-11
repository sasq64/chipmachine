
#include "catch.hpp"

#include "src/MusicDatabase.h"
#include "src/ChipMachine.h"
#include "src/MusicPlayerList.h"
#include "src/modutils.h"

#include <audioplayer/audioplayer.h>

#include "src/di.hpp"

using namespace chipmachine;
namespace di = boost::di;

#include <coreutils/log.h>
#include <musicplayer/chipplugin.h>
#include <string>
#include <array>

TEST_CASE("ditest", "[machine]")
{
    struct App
    {
        //App(MusicPlayerList& mp) : mp(mp) {}
        //MusicPlayerList& mp;
        ChipMachine& cm;
    };

    //MusicDatabase db;
    //db.initFromLua("");

    logging::setLevel(logging::Level::Verbose);
    AudioPlayer ap{44100};
    const auto injector = di::make_injector(
           // di::bind<AudioPlayer>.to(ap),
           // di::bind<MusicDatabase>.to(db),
            di::bind<utils::path>.to(".")
            );

    musix::ChipPlugin::createPlugins("data");
        grappix::screen.open(64, 64, false);

    App cm = injector.create<App>();

    /* App a = injector.create<App>(); */
    /* a.mp.addSong({"music/Amiga/Nuke - Loader.mod"}); */
    /* a.mp.nextSong(); */
    /* while(true) { */
    /*     //a.mp.update(); */
    /*     utils::sleepms(1); */
    /* } */
}
