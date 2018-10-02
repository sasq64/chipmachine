#include "catch.hpp"

#include "src/MusicDatabase.h"
#include "src/MusicPlayer.h"

#include "src/modutils.h"

#include <coreutils/log.h>
#include <musicplayer/chipplugin.h>
#include <string>
#include <array>

TEST_CASE("modutils", "[machine]")
{
    auto x = getTypeAndBase("/blaj/mdat.gurgle%tjosan");
    REQUIRE(x == std::make_tuple("mdat", "gurgle%tjosan"));

    x = getTypeAndBase("/blaj/skurk.mannen.x.mod");
    REQUIRE(x == std::make_tuple("mod", "skurk.mannen.x"));

    x = getTypeAndBase("/blaj/mod/mdat/hejsan hoppsan.whatever");
    REQUIRE(x == std::make_tuple("whatever", "hejsan hoppsan"));

    REQUIRE(getBaseName("/asda/das/test.mod") == "test.mod");
    REQUIRE(getTypeFromName("gurgle.format") == "format");
    REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
    REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
    REQUIRE(getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%"
                            "2fSunTronic%2fTSM%2fmsx-intro.sun") == "sun");
    REQUIRE(
        getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%2fTFMX%"
                        "2fChris Huelsbeck%2fmdat.apidya (level 3)") == "mdat");
}

TEST_CASE("music database", "")
{
    using namespace chipmachine;

    auto& mdb = MusicDatabase::getInstance();
    REQUIRE(mdb.initFromLua(utils::path(".")) == true);
    auto q = mdb.createQuery();
}

TEST_CASE("music player", "")
{
    chipmachine::MusicPlayer mp{"."};
    mp.playFile("music/Amiga/Nuke - Loader.mod");
    mp.update();
}
#include <musicplayer/plugins/plugins.h>
#include <numeric>

template <typename PLUGIN, typename... ARGS>
bool testPlugin(std::string const& dir, std::string const& exclude, const ARGS&... args)
{
    std::array<int16_t, 8192> buffer;
    PLUGIN plugin{args...};
	printf("---- %s ----\n", plugin.name().c_str());
    logging::setLevel(logging::Level::Warning);
    for (auto f : utils::File{dir}.listFiles()) {
		if(exclude != "" && f.getName().find(exclude) != std::string::npos)
			continue;

        int64_t sum = 0;
        printf("Trying %s\n", f.getName().c_str());
        auto* player = plugin.fromFile(f.getName());
        if (player) {
            //puts("Player created");
			int count = 15;
            while (sum == 0 && count != 0) {
                int rc = player->getSamples(&buffer[0], buffer.size());
                // REQUIRE(rc > 0);
                if (rc > 0) {
                    sum = std::accumulate(&buffer[0], &buffer[rc], (int64_t)0);
                    // REQUIRE(sum != 0);
                    if (sum != 0) {
                        break;
                    }
                    count--;
                } else
                    break;
            }
            delete player;
        }
        printf("#### Playing %s : %s\n", f.getName().c_str(), 
                player ?
                (sum == 0 ? "NO SOUND" : "OK")
                : "FAILED"
                );
    }
    return true;
}

TEST_CASE("gme", "[music]")
{
    testPlugin<musix::GMEPlugin>("testmus/gme/working", "");
}

TEST_CASE("adlib", "[music]")
{
    testPlugin<musix::AdPlugin>("testmus/adlib", "", "data");
}

TEST_CASE("uade", "[music]")
{
    testPlugin<musix::UADEPlugin>("testmus/uade", "smp", "data");
}

TEST_CASE("openmpt", "[music]")
{
    testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", "");
}

TEST_CASE("gsf", "[music]")
{
    testPlugin<musix::GSFPlugin>("testmus/gsf", "lib");
}

TEST_CASE("nds", "[music]")
{
    testPlugin<musix::NDSPlugin>("testmus/nds", "lib");
}

TEST_CASE("psx", "[music]")
{
    testPlugin<musix::HEPlugin>("testmus/psx", "lib", "data/hebios.bin");
}

TEST_CASE("zx", "[music]")
{
    testPlugin<musix::AyflyPlugin>("testmus/zx", "");
}

