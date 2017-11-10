#include "MusicPlayer.h"
#include "ChipInterface.h"
#ifndef TEXTMODE_ONLY
#  include "ChipMachine.h"
#  include <grappix/grappix.h>
#endif
#include <musicplayer/PSFFile.h>
#include <coreutils/format.h>
#include <coreutils/var.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#ifndef _WIN32
#  include <bbsutils/console.h>
#  define ENABLE_CONSOLE
#endif
#include "version.h"
#include "CLI11.hpp"
#include <vector>

using namespace std;
using namespace chipmachine;
using namespace bbs;
using namespace utils;

namespace chipmachine {
	void runConsole(shared_ptr<Console> console, ChipInterface &ci);
}

int main(int argc, char *argv[]) {

#ifdef CM_DEBUG
	logging::setLevel(logging::DEBUG);
#else
	logging::setLevel(logging::WARNING);
#endif

	vector<SongInfo> songs;
	int w = 960;
	int h = 540;
    int port = 12345;
	bool fullScreen = false;
	bool telnetServer = false;
    bool onlyHeadless = false;
	string playWhat;
#ifdef TEXTMODE_ONLY
    bool textMode = true;
#else
	bool textMode = false;
#endif

    CLI::App opts{"Chipmachine 1.3"};

#ifndef TEXTMODE_ONLY
    opts.add_option("--width", w, "Width of window");
    opts.add_option("--height", h, "Height of window");
    opts.add_flag("-f,--fullscreen", fullScreen, "Run in fullscreen");
#endif
    opts.add_flag("-X,--textmode", textMode, "Run in textmode");
    opts.add_flag_function("-d", [&](size_t count) {
        fullScreen = false;
		logging::setLevel(logging::DEBUG);
        }, "Debug output");

    opts.add_option("-T,--telnet", telnetServer, "Start telnet server");
    opts.add_option("-p,--port", port, "Port for telnet server", 12345);
    opts.add_flag("-K", onlyHeadless, "Only play if no keyboard is connected");
    opts.add_option("--play", playWhat, "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_option("files", songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

	string path = File::makePath({
#ifdef __APPLE__
	    (File::getExeDir() / ".." / "Resources").resolve(),
#else
	    File::getExeDir(),
#endif
	    (File::getExeDir() / ".." / "chipmachine").resolve(),
	    (File::getExeDir() / ".." / ".." / "chipmachine").resolve(),
	    (File::getExeDir() / "..").resolve(),
	    (File::getExeDir() / ".." / "..").resolve(),
	    File::getAppDir()
	});
	LOGD("PATH:%s", path);

	string workDir = File::findFile(path, "data").getDirectory();

	if(workDir == "") {
		fprintf(stderr, "** Error: Could not find data files\n");
		exit(-1);
	}

	LOGD("WorkDir:%s", workDir);

	if(songs.size() > 0) {
		int pos = 0;
#ifdef ENABLE_CONSOLE
		Console *c = Console::createLocalConsole();
#endif
		MusicPlayer pl(workDir);
		while(true) {
			if(pos >= songs.size())
				return 0;
			pl.playFile(songs[pos++].path);
			SongInfo info = pl.getPlayingInfo();
			print_fmt("Playing: %s\n",
			          info.title != "" ? info.title : utils::path_filename(songs[pos - 1].path));
			int tune = 0;
			while(pl.playing()) {
				pl.update();
#ifdef ENABLE_CONSOLE
				if(c) {
					auto k = c->getKey(100);
					if(k != Console::KEY_TIMEOUT) {
						switch(k) {
						case Console::KEY_RIGHT:
							LOGD("SEEK");
							pl.seek(tune++);
							break;
						case Console::KEY_ENTER:
							pl.stop();
							break;
						}
					}
				}
#endif
			}
		}
		return 0;
	}

	if(textMode || telnetServer) {

		ChipInterface ci(workDir);
		if(textMode) {
#ifndef _WIN32
			logging::setLevel(logging::ERROR);
			auto console = std::shared_ptr<bbs::Console>(bbs::Console::createLocalConsole());
			runConsole(console, ci);
			if(telnetServer)
				std::thread conThread(runConsole, console, std::ref(ci));
			else
				runConsole(console, ci);
#else
			puts("Textmode not supported on Windows");
			exit(0);
#endif
		}
		if(telnetServer) {
			auto telnet = std::make_shared<bbs::TelnetServer>(port);
			telnet->setOnConnect([&](bbs::TelnetServer::Session &session) {
				try {
					std::shared_ptr<bbs::Console> console;
					session.echo(false);
					auto termType = session.getTermType();
					LOGD("New telnet connection, TERMTYPE '%s'", termType);

					if(termType.length() > 0) {
						console = std::make_shared<bbs::AnsiConsole>(session);
					} else {
						console = std::make_shared<bbs::PetsciiConsole>(session);
					}
					runConsole(console, ci);
				} catch(bbs::TelnetServer::disconnect_excpetion &e) {
					LOGD("Got disconnect");
				}
			});
			telnet->run();
		}
		return 0;
	}
#ifndef TEXTMODE_ONLY
	grappix::screen.setTitle("Chipmachine " VERSION_STR);
	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(w, h, false);

	static chipmachine::ChipMachine app(workDir);
	if(playWhat != "" && (!onlyHeadless || !grappix::screen.haveKeyboard()))
		app.playNamed(playWhat);

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
#endif
	return 0;
}
