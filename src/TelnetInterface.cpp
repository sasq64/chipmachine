#include "TelnetInterface.h"
#include "ChipMachine.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <lua/luainterpreter.h>

using namespace std;
using namespace utils;
using namespace bbs;

namespace chipmachine {

typedef std::unordered_map<string, string> strmap;

TelnetInterface::TelnetInterface(ChipMachine &cm) : chipmachine(cm) {}

void TelnetInterface::start() {
	telnet = make_unique<TelnetServer>(12345);
	telnet->setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		string termType = session.getTermType();		
		LOGD("New connection, TERMTYPE '%s'", termType);

		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();
		LuaInterpreter lip;

		console->write("### CHIPMACHINE LUA INTERPRETER\n");

		lip.setOuputFunction([&](const std::string &s) {
			console->write(s);
		});

		lip.registerFunction<void, string>("scrolltext", [=](const string &t) {
			chipmachine.set_scrolltext(t);
		});

		lip.registerFunction<vector<strmap>, string>("find", [=](const string &q) -> vector<strmap> {

			vector<int> result;
			vector<strmap> fullres;
			result.reserve(100);

			auto &db = MusicDatabase::getInstance();

			db.search(q, result, 100);
			//int i = 1;
			//console->write(format("GOT %d hits\n", result.size()));
			for(const auto &r : result) {
				SongInfo song = db.getSongInfo(r);
				//console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
				strmap s;
				s["path"] = song.path;
				s["title"] = song.title;
				s["composer"] = song.composer;
				fullres.push_back(s);
			}				
			return fullres;
		});

		lip.registerFunction<void, string>("play_file", [&](const std::string &path) {
			auto &player = chipmachine.music_player();
			SongInfo song(path);
			player.playSong(song);
		});

		lip.registerFunction<void, strmap>("play_song", [&](const strmap &s) {
			auto &player = chipmachine.music_player();
			SongInfo song(s.at("path"), "", s.at("title"), s.at("composer"));
			player.playSong(song);
		});

		lip.registerFunction<void>("next_song", [&]() {
			auto &player = chipmachine.music_player();
			player.nextSong();
		});

		lip.registerFunction<strmap>("get_playing_song", [&]() {
			auto &player = chipmachine.music_player();
			SongInfo song = player.getInfo();
			strmap s;
			s["path"] = song.path;
			s["title"] = song.title;
			s["composer"] = song.composer;
			return s;
		});

		lip.registerFunction<void, string, int>("toast", [=](const string &t, int type) {
			grappix::screen.run_safely([&]() {
				chipmachine.toast(t, type);
			});
		});

		lip.loadFile("lua/init.lua");

		while(true) {
			auto l = console->getLine(">");
			auto parts = split(l, " ");
			if(isalpha(parts[0]) && (parts.size() == 1 || parts[1][0] != '=')) {
				l = parts[0] + "(";
				for(int i=1; i<(int)parts.size(); i++) {
					if(isalpha(parts[i]))
						parts[i] = format("'%s'", parts[i]);
					if(i!=1) l += ",";
					l += parts[i];
				}
				l += ")";
				LOGD("Changed to %s", l);
			}
			try {
				if(!lip.load(l))
					console->write("** SYNTAX ERROR\n");
			} catch (lua_exception &e) {
				console->write(format("** %s\n", e.what()));
			}

		}
	});
	telnet->runThread();
}

} // namespace chipmachine