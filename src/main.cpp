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

#include <musicplayer/PSFFile.h>

#ifndef _WIN32
#    include <bbsutils/console.h>
#    define ENABLE_CONSOLE
#endif
#include "CLI11.hpp"

#include "di.hpp"
namespace di = boost::di;

#include "version.h"
#include <optional>
#include <vector>

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

    static CLI::App opts{ "Chipmachine 1.3" };

#ifndef TEXTMODE_ONLY
    opts.add_option("--width", w, "Width of window");
    opts.add_option("--height", h, "Height of window");
    opts.add_flag("-f,--fullscreen", fullScreen, "Run in fullscreen");
#endif
    opts.add_flag("-X,--textmode", textMode, "Run in textmode");
    opts.add_flag_function("-d",
                           [&](size_t count) {
                               fullScreen = false;
                               logging::setLevel(logging::Debug);
                           },
                           "Debug output");

    opts.add_option("-T,--telnet", telnetServer, "Start telnet server");
    opts.add_option("-p,--port", port, "Port for telnet server", true);
    opts.add_flag("-K", onlyHeadless, "Only play if no keyboard is connected");
    opts.add_option("--play", playWhat,
                    "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_option("files", songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

    auto path = makeSearchPath(
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
    LOGD("PATH:%s", path);

    auto d = findFile(path, "data");

    if (!d) {
        fprintf(stderr, "** Error: Could not find data files\n");
        exit(-1);
    }

    auto workDir = d->parent_path();

    LOGD("WorkDir:%s", workDir);

    if (!songs.empty()) {
        int pos = 0;
#ifdef ENABLE_CONSOLE
        auto* c = bbs::Console::createLocalConsole();
#endif
        // auto pl =
        // std::make_unique<chipmachine::MusicPlayer>(workDir.string());
        const auto injector = di::make_injector(di::bind<utils::path>.to("."));

        musix::ChipPlugin::createPlugins("data");

        static auto pl =
            injector.create<std::unique_ptr<chipmachine::MusicPlayer>>();

        while (true) {
            if (pos >= songs.size()) return 0;
            pl->playFile(songs[pos++].path);
            SongInfo info = pl->getPlayingInfo();
            utils::print_fmt("Playing: %s\n",
                             !info.title.empty()
                                 ? info.title
                                 : utils::path_filename(songs[pos - 1].path));
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

    if (textMode || telnetServer) {

        const auto injector =
            di::make_injector(di::bind<utils::path>.to(workDir));

        musix::ChipPlugin::createPlugins("data");

        static auto ci =
            injector.create<std::unique_ptr<chipmachine::ChipInterface>>();
        if (textMode) {
#ifndef _WIN32
            logging::setLevel(logging::Error);
            auto console = std::shared_ptr<bbs::Console>(
                bbs::Console::createLocalConsole());
            chipmachine::runConsole(console, *ci);
            if (telnetServer)
                std::thread conThread(chipmachine::runConsole, console,
                                      std::ref(*ci));
            else
                chipmachine::runConsole(console, *ci);
#else
            puts("Textmode not supported on Windows");
            exit(0);
#endif
        }
        if (telnetServer) {
            auto telnet = std::make_shared<bbs::TelnetServer>(port);
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
    if (fullScreen)
        grappix::screen.open(true);
    else
        grappix::screen.open(w, h, false);

    struct App
    {
        App(chipmachine::ChipMachine& c) : cm(c) {}
        chipmachine::ChipMachine& cm;
    };

    AudioPlayer ap{ 44100 };
    const auto injector = di::make_injector(di::bind<AudioPlayer>.to(ap),
                                            // di::bind<MusicDatabase>.to(db),
                                            di::bind<utils::path>.to("."));

    musix::ChipPlugin::createPlugins("data");

    static App app = injector.create<App>();

    // static chipmachine::ChipMachine app(workDir);
    if (!playWhat.empty() && (!onlyHeadless || !grappix::screen.haveKeyboard()))
        app.cm.playNamed(playWhat);

    grappix::screen.render_loop(
        [](uint32_t delta) {
            app.cm.update();
            app.cm.render(delta);
        },
        20);
#endif

    LOGD("Controlled exit");

    return 0;
}
