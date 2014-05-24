#include "TelnetInterface.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <lua/luainterpreter.h>

using namespace std;
using namespace utils;
using namespace bbs;

namespace chipmachine {

TelnetInterface::TelnetInterface(MusicDatabase &db, MusicPlayerList &player) : db(db), player(player) {}

void TelnetInterface::start() {
	telnet = make_unique<TelnetServer>(12345);
	telnet->setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		string termType = session.getTermType();		
		LOGD("New connection, TERMTYPE '%s'", termType);

		unique_ptr<Console> console;
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();
		std::vector<SongInfo> songs;

		LuaInterpreter lip;

		lip.setOuputFunction([&](const std::string &s) {
			console->write(s);
		});

		lip.loadFile("lua/init.lua");

		lip.registerFunction<int, string, int, int, float, int>("Infoscreen.add", [&](const string &q, int x, int y, float scale, int color) -> int {
			return 0;
		});

		lip.registerFunction<void, string>("find", [&](const string &q) {
			songs = db.find(q);
			int i = 0;
			for(const auto &s : songs) {
				console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
			}				
		});

		lip.registerFunction<void, int>("play", [&](int which) {
			player.clearSongs();
			player.addSong(songs[which]);
			player.nextSong();
		});

		while(true) {
			auto l = console->getLine(">");
			auto parts = split(l, " ");
			if(isalpha(parts[0]) && (parts.size() == 1 || parts[1][0] != '=')) {
				l = parts[0] + "(";
				for(int i=1; i<(int)parts.size(); i++) {
					if(i!=1) l += ",";
					l += parts[i];
				}
				l += ")";
				LOGD("Changed to %s", l);
			}
			if(!lip.load(l))
				console->write("** SYNTAX ERROR\n");
		/*
			auto cmd = split(l, " ", 1);
			//LOGD("%s '%s'", cmd[0], cmd[1]);
			if(cmd[0] == "status") {
				//LOGD("%s %s", composer, title);
				//console->write(format("%s - %s\n", composer, title));
			} else if (cmd[0] == "find") {
				songs = db.search(cmd[1]);
				int i = 0;
				for(const auto &s : songs) {
					console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
				}
			} else if (cmd[0] == "play") {
				int which = atoi(cmd[1].c_str());
				play(songs[which]);
				next();
			} else if (cmd[0] == "q") {
				int which = atoi(cmd[1].c_str());
				play(songs[which]);
			} else if (cmd[0] == "next") {
				next();
			}*/
		}
	});
	telnet->runThread();
}

} // namespace chipmachine