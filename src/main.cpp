// #ifdef LINUX
// #define BACKWARD_HAS_BFD 1
// #include <backward-cpp/backward.hpp>
// namespace backward {
// 	backward::SignalHandling sh;
// }
// #endif
//#define ENABLE_TELNET

#include "MusicPlayer.h"
#include "ChipInterface.h"
#ifndef TEXTMODE_ONLY
#include "ChipMachine.h"
#include <grappix/grappix.h>
#endif
#include <musicplayer/PSFFile.h>
#include <coreutils/format.h>
#include <coreutils/var.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#ifndef _WIN32
#include <bbsutils/console.h>
#define ENABLE_CONSOLE
#endif

#include "../docopt/docopt.h"
#include <vector>

using namespace std;
using namespace chipmachine;
using namespace bbs;
using namespace utils;

namespace chipmachine {
	void runConsole(shared_ptr<Console> console, ChipInterface &ci);
}

static const char USAGE[] =
R"(chipmachine.
	Usage:
      chipmachine [options]
      chipmachine [-d] <files>...

    Options:
      -d                Debug output)"
#ifndef TEXTMODE_ONLY
R"(
      -f, --fullscreen    Run in Fullscreen
      --width <width>     Width of window
      --height <height>   Height of window
      -X, --textmode      Run in textmode)"
#endif
R"(
      -T, --telnet      Start telnet server
      -p <port>         Telnet server port (default 12345)
)";

int main(int argc, char *argv[]) {

#ifdef CM_DEBUG
	logging::setLevel(logging::DEBUG);
#else
	logging::setLevel(logging::WARNING);
#endif

	vector<SongInfo> songs;
	int w = 960;
	int h = 540;
	bool fullScreen = false;
	bool telnetServer = false;
#ifdef TEXTMODE_ONLY
    bool textMode = true;
#else
	bool textMode = false;
#endif

	auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "Chipmachine 1.3");
                                                  
#ifndef TEXTMODE_ONLY                       
	if(args["--width"])
		w = args["--width"].asLong();
	if(args["--height"])
		h = args["--height"].asLong();
	fullScreen = args["--fullscreen"].asBool();
	textMode = args["--textmode"].asBool();
#endif
	if(args["-d"].asBool()) {
		fullScreen = false;
		logging::setLevel(logging::DEBUG);
    }
	telnetServer = args["--telnet"].asBool();
	
	if(args["<files>"]) {
		const auto &sl = args["<files>"].asStringList();
		std::copy(sl.begin(), sl.end(), std::back_inserter(songs));
	}

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
			while(pl.playing()) {
				pl.update();
#ifdef ENABLE_CONSOLE
				if(c) {
					auto k = c->getKey(100);
					if(k != Console::KEY_TIMEOUT) {
						switch(k) {
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
#ifndef _WIN32
		if(textMode) {
			logging::setLevel(logging::ERROR);
			auto console = std::shared_ptr<bbs::Console>(bbs::Console::createLocalConsole());
			runConsole(console, ci);
			if(telnetServer)
				std::thread conThread(runConsole, console, std::ref(ci));
			else
				runConsole(console, ci);
		}
#else
		puts("Textmode not supported on Windows");
		exit(0);
#endif
		if(telnetServer) {
			auto telnet = std::make_shared<bbs::TelnetServer>(12345);
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
	if(fullScreen)
		grappix::screen.open(true);
	else
		grappix::screen.open(w, h, false);

	static chipmachine::ChipMachine app(workDir);

	grappix::screen.render_loop([](uint32_t delta) {
		app.update();
		app.render(delta);
	}, 20);
#endif
	return 0;
}
