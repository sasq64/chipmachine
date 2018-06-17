#include "TelnetInterface.h"
#include "ChipMachine.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <grappix/window.h>

#include <luainterpreter/luainterpreter.h>

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

using namespace std;
using namespace utils;
using namespace bbs;

namespace chipmachine {

typedef std::unordered_map<string, string> strmap;

void TelnetInterface::stop() {
	telnet->stop();
}

using grappix::Window;

static unordered_map<int, int> key_translate = {
    {Console::KEY_UP, Window::UP},
    {Console::KEY_DOWN, Window::DOWN},
    {Console::KEY_PAGEUP, Window::PAGEUP},
    {Console::KEY_PAGEDOWN, Window::PAGEDOWN},
    {Console::KEY_ENTER, Window::ENTER},
    {Console::KEY_ESCAPE, Window::ESCAPE},
    {Console::KEY_BACKSPACE, Window::BACKSPACE},
};

void TelnetInterface::start() {
	telnet = make_shared<TelnetServer>(12345);
	telnet->setOnConnect([&](TelnetServer::Session &session) {
		try {
			session.echo(false);
			string termType = session.getTermType();
			LOGD("New telnet connection, TERMTYPE '%s'", termType);

			if(termType.length() > 0) {
				console = make_shared<AnsiConsole>(session);
			} else {
				console = make_shared<PetsciiConsole>(session);
			}
			runClient(console);
		} catch(TelnetServer::disconnect_excpetion &e) {
		}
	});
	telnet->runThread();
}

void TelnetInterface::runClient(shared_ptr<Console> console) {

	while(true) {
		int key = console->getKey(100);
		if(key != Console::KEY_TIMEOUT) {
			if(key_translate.count(key))
				key = key_translate[key];
			putEvent<grappix::KeyEvent>(key);
		}
	}

	console->flush();

	LuaInterpreter lip;

	console->write("### CHIPMACHINE LUA INTERPRETER\n");

	lip.setOuputFunction([&](const std::string &s) { console->write(s); });

	lip.registerFunction("scrolltext", [=](const string &t) {
		// chipmachine.set_scrolltext(t);
	});

	lip.registerFunction("find", [=](const string &q) -> vector<strmap> {

		vector<int> result;
		vector<strmap> fullres;
		result.reserve(100);

		auto &db = MusicDatabase::getInstance();

		db.search(q, result, 100);
		// int i = 1;
		// console->write(format("GOT %d hits\n", result.size()));
		for(const auto &r : result) {
			SongInfo song = db.getSongInfo(r);
			// console->write(format("%02d. %s - %s (%s)\n", i++, s.composer, s.title, s.format));
			strmap s;
			s["path"] = song.path;
			s["title"] = song.title;
			s["composer"] = song.composer;
			fullres.push_back(s);
		}
		return fullres;
	});

	lip.registerFunction("play_file", [&](const std::string &path) {
		SongInfo song(path);
		player.playSong(song);
	});

	lip.registerFunction("play_song", [&](const strmap &s) {
		SongInfo song(s.at("path"), "", s.at("title"), s.at("composer"));
		player.playSong(song);
	});

	lip.registerFunction("next_song", [&]() { player.nextSong(); });

	lip.registerFunction("get_playing_song", [&]() -> strmap {
		SongInfo song = player.getInfo();
		strmap s;
		s["path"] = song.path;
		s["title"] = song.title;
		s["composer"] = song.composer;
		return s;
	});

	lip.registerFunction("toast", [=](const string &t, int type) {
		// grappix::screen.run_safely([&]() {
		// chipmachine.toast(t, type);
		//});
	});

	lip.loadFile("lua/init.lua");

	while(true) {
		auto l = console->getLine(">");
		auto parts = split(l, " ");
		if(isalpha(parts[0]) && (parts.size() == 1 || parts[1][0] != '=')) {
			l = parts[0] + "(";
			for(int i = 1; i < (int)parts.size(); i++) {
				if(isalpha(parts[i]))
					parts[i] = format("'%s'", parts[i]);
				if(i != 1)
					l += ",";
				l += parts[i];
			}
			l += ")";
			LOGD("Changed to %s", l);
		}
		try {
			if(!lip.load(l))
				console->write("** SYNTAX ERROR\n");
		} catch(lua_exception &e) {
			console->write(format("** %s\n", e.what()));
		}
	}
}

} // namespace chipmachine
