#include "ChipInterface.h"
#include "MusicPlayer.h"
#ifndef TEXTMODE_ONLY
#    include "ChipMachine.h"
#    include <grappix/grappix.h>
#endif
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>
#include <bbsutils/telnetserver.h>
#include <coreutils/environment.h>
#include <coreutils/format.h>
#include <coreutils/searchpath.h>
#include <coreutils/var.h>

#include <audioplayer/audioplayer.h>
#include <musicplayer/plugins/plugins.h>

#include <psf/PSFFile.h>

#ifndef _WIN32
#    include <bbsutils/console.h>
#    define ENABLE_CONSOLE
#endif
#include "CLI11.hpp"

#include "di.hpp"
namespace di = boost::di;

#include <optional>
#include <vector>

#include "version.h"

namespace chipmachine {
void runConsole(std::shared_ptr<bbs::Console> console, ChipInterface& ci);
}

int main(int argc, char* argv[])
{
    Environment::setAppName("chipmachine");

#ifdef CM_DEBUG
    logging::setLevel(logging::Level::Debug);
#else
    logging::setLevel(logging::Level::Warning);
#endif

    srand(time(NULL));

    struct
    {
        std::vector<SongInfo> songs;
        int w = 960;
        int h = 540;
        int port = 12345;
        bool fullScreen = false;
        bool telnetServer = false;
        bool onlyHeadless = false;
        std::string playWhat;
#ifdef TEXTMODE_ONLY
        bool textMode = true;
#else
        bool textMode = false;
#endif
    } options;

    static CLI::App opts{ "Chipmachine " VERSION_STR };

#ifndef TEXTMODE_ONLY
    opts.add_option("--width", options.w, "Width of window");
    opts.add_option("--height", options.h, "Height of window");
    opts.add_flag("-f,--fullscreen", options.fullScreen, "Run in fullscreen");
#endif
    opts.add_flag("-X,--textmode", options.textMode, "Run in textmode");
    opts.add_flag_function(
        "-d",
        [&](size_t count) {
            options.fullScreen = false;
            logging::setLevel(logging::Debug);
        },
        "Debug output");

    opts.add_option("-T,--telnet", options.telnetServer, "Start telnet server");
    opts.add_option("-p,--port", options.port, "Port for telnet server", true);
    opts.add_flag("-K", options.onlyHeadless,
                  "Only play if no keyboard is connected");
    opts.add_option("--play", options.playWhat,
                    "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_option("files", options.songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

    auto search_path = makeSearchPath(
        {
#ifdef __APPLE__
            Environment::getExeDir() / ".." / "Resources",
#else
            Environment::getExeDir(),
#endif
            Environment::getExeDir() / ".." / "chipmachine",
            Environment::getExeDir() / ".." / ".." / "chipmachine",
            Environment::getExeDir() / "..",
            Environment::getExeDir() / ".." / "..", Environment::getAppDir() },
        true);
    LOGD("PATH:%s", search_path);

    auto d = findFile(search_path, "data");

    if (!d) {
        fprintf(stderr, "** Error: Could not find data files\n");
        exit(-1);
    }

    auto workDir = d->parent_path();
    musix::ChipPlugin::createPlugins(workDir / "data");
    AudioPlayer ap{ 44100 };
    const auto injector = di::make_injector(di::bind<AudioPlayer>.to(ap),
                                            di::bind<utils::path>.to(workDir));
    LOGD("WorkDir:%s", workDir);

    if (!options.songs.empty()) {
        int pos = 0;
#ifdef ENABLE_CONSOLE
        auto* c = bbs::Console::createLocalConsole();
#endif
        static auto pl =
            injector.create<std::unique_ptr<chipmachine::MusicPlayer>>();

        while (true) {
            if (pos >= options.songs.size()) return 0;
            pl->playFile(options.songs[pos++].path);
            SongInfo info = pl->getPlayingInfo();
            utils::print_fmt(
                "Playing: %s\n",
                !info.title.empty()
                    ? info.title
                    : utils::path_filename(options.songs[pos - 1].path));
            int tune = 0;
            while (pl->playing()) {
                pl->update();
#ifdef ENABLE_CONSOLE
                if (c) {
                    auto k = c->getKey(100);
                    if (k != bbs::Console::KEY_TIMEOUT) {
                        switch (k) {
                        case bbs::Console::KEY_RIGHT:
                            LOGD("SEEK");
                            pl->seek(tune++);
                            break;
                        case bbs::Console::KEY_ENTER: pl->stop(); break;
                        }
                    }
                }
#endif
            }
        }
        return 0;
    }

    if (options.textMode || options.telnetServer) {

        static auto ci =
            injector.create<std::unique_ptr<chipmachine::ChipInterface>>();
        if (options.textMode) {
#ifndef _WIN32
            logging::setLevel(logging::Error);
            auto console = std::shared_ptr<bbs::Console>(
                bbs::Console::createLocalConsole());
            chipmachine::runConsole(console, *ci);
            if (options.telnetServer)
                std::thread conThread(chipmachine::runConsole, console,
                                      std::ref(*ci));
            else
                chipmachine::runConsole(console, *ci);
#else
            puts("Textmode not supported on Windows");
            exit(0);
#endif
        }
        if (options.telnetServer) {
            auto telnet = std::make_shared<bbs::TelnetServer>(options.port);
            telnet->setOnConnect([&](bbs::TelnetServer::Session& session) {
                try {
                    std::shared_ptr<bbs::Console> console;
                    session.echo(false);
                    auto termType = session.getTermType();
                    LOGD("New telnet connection, TERMTYPE '%s'", termType);

                    if (termType.length() > 0) {
                        console = std::make_shared<bbs::AnsiConsole>(session);
                    } else {
                        console =
                            std::make_shared<bbs::PetsciiConsole>(session);
                    }
                    runConsole(console, *ci);
                } catch (bbs::TelnetServer::disconnect_excpetion& e) {
                    LOGD("Got disconnect");
                }
            });
            telnet->run();
        }
        return 0;
    }
#ifndef TEXTMODE_ONLY
    grappix::screen.setTitle("Chipmachine " VERSION_STR);
    if (options.fullScreen)
        grappix::screen.open(true);
    else
        grappix::screen.open(options.w, options.h, false);

    auto chip_machine =
        injector.create<std::unique_ptr<chipmachine::ChipMachine>>();

    if (!options.playWhat.empty() &&
        (!options.onlyHeadless || !grappix::screen.haveKeyboard()))
        chip_machine->playNamed(options.playWhat);

    grappix::screen.render_loop(
        [&chip_machine](uint32_t delta) {
            chip_machine->update();
            chip_machine->render(delta);
        },
        20);
#endif

    LOGD("Controlled exit");

    return 0;
}
