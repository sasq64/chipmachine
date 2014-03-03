
#include "ChipPlayer.h"

#include <ModPlugin/ModPlugin.h>
#include <VicePlugin/VicePlugin.h>
#include <SexyPSFPlugin/SexyPSFPlugin.h>
#include <GMEPlugin/GMEPlugin.h>
#include <SC68Plugin/SC68Plugin.h>
#include <UADEPlugin/UADEPlugin.h>

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <bbsutils/console.h>
#include <audioplayer/audioplayer.h>
#include <sqlite3/database.h>

#include <cstdio>
#include <vector>
#include <string>
#include <unordered_set>

using namespace chipmachine;
using namespace std;
using namespace utils;
using namespace bbs;

class PlayerSystem  {
public:
	ChipPlayer *fromFile(File &file) {

		string name = file.getName();
		makeLower(name);
		for(auto *plugin : plugins) {
			if(plugin->canHandle(name)) {
				//print_fmt("Playing with %s\n", plugin->name());
				fflush(stdout);
				return plugin->fromFile(file.getName());
			}
		}
		return nullptr;
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

private:
	vector<ChipPlugin*> plugins;
};

/*
		if parts[0] == 'Ad Lib' or parts[0] == 'Video Game Music':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]
		if parts[0] == 'YM' and parts[1] == 'Synth Pack':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]

		if parts[2].startswith('coop-') :
			parts = [parts[0]] + [parts[1] + '/' + parts[2]] + parts[3:]
*/
void index_db(sqlite3db::Database &db) {

	static const unordered_set<string> ownDirFormats = {
		"Dreamcast Sound Format",
		"Gameboy Sound Format",
		"Nintendo Sound Format",
		"Nintendo SPC",
		"Video Game Music",
		"Ultra64 Sound Format",
		"Playstation Sound Format"
	};

	db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, format STRING, path STRING)");

	auto query = db.query("INSERT INTO song (title, game, composer, format, path) VALUES (?, ?, ?, ?, ?)");

	db.exec("BEGIN TRANSACTION");
	File file { "allmods.txt" };
	for(const auto &s : file.getLines()) {

		auto path = split(s, "\t")[1];
		auto parts = split(path, "/");
		auto l = parts.size();
		if(l < 3) {
			LOGD("%s", path);
			continue;
		}

		int i = l-1;
		bool ownDir = (ownDirFormats.count(parts[0]) > 0);

		string title = path_basename(parts[i--]);
		string game;

		if(ownDir && l >= 4) {
			game = parts[i--];
		}

		string composer = parts[i--];

		if(composer == "- unknown")
			composer = "?";

		if(composer.substr(0, 5) == "coop-")
			composer = parts[i--] + "+" + composer.substr(5);

		string format = parts[i--];

		query.bind(title, game, composer, format, path);
		
		query.step();
	}
	db.exec("COMMIT");
}

int main(int argc, char* argv[]) {

	sqlite3db::Database db { "modland.db" };

	//index_db(db);
	//return 0;

	//if(argc < 2) {
	//	printf("%s [musicfiles...]\n", argv[0]);
	//	return 0;
	//}

	setvbuf(stdout, NULL, _IONBF, 0);
	logging::setLevel(logging::ERROR);

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {"data/c64"});
	psys.registerPlugin(new SexyPSFPlugin {});
	psys.registerPlugin(new GMEPlugin {});
	psys.registerPlugin(new SC68Plugin {"data/sc68"});
	psys.registerPlugin(new UADEPlugin {});

	auto *console = bbs::Console::createLocalConsole();
	console->clear();
	console->moveCursor(0,0);
	mutex m;

	unique_ptr<ChipPlayer> player;

	AudioPlayer ap ([&](int16_t *ptr, int size) {
		lock_guard<mutex> guard(m);
		if(player)
			player->getSamples(ptr, size);
	});

	string modlandPath = "/nas/DATA/MUSIC/MODLAND";
	vector<string> songs;
	while(true) {
		auto line = console->getLine(">");
		makeLower(line);
		auto parts = split(line, " ");
		if(parts[0] == "find") {
			songs.clear();

			auto q = parts.size() > 2 ? 
				  db.query<string, string, string>("SELECT path,title,composer FROM song WHERE title LIKE ? COLLATE NOCASE AND composer LIKE ? COLLATE NOCASE", "%" + parts[1] + "%", "%" + parts[2] + "%")
				: db.query<string, string, string>("SELECT path,title,composer FROM song WHERE title LIKE ? COLLATE NOCASE", "%" + parts[1] + "%");

			int i = 0;
			while(q.step()) {
				auto t = q.get_tuple();
				songs.push_back(format("%s/%s", modlandPath, get<0>(t)));
				console->write(format("[%02d] %s / %s\n", i++, get<1>(t), get<2>(t)));
			}
		} else if(parts[0] == "play") {
			int n = stol(parts[1]);
			File file { songs[n] };
			console->write(format("Song:%s\n", songs[n]));
			{
				lock_guard<mutex> guard(m);
				player = nullptr;
				player = unique_ptr<ChipPlayer>(psys.fromFile(file));
			}
		}

	}
/*
	auto q = db.query<string, string>("SELECT path,name FROM song WHERE name LIKE ?", string(argv[1]) + "%");
	while(q.step()) {
		auto t = q.get_tuple();
		songs.push_back(format("%s/%s/%s", modlandPath, get<0>(t), get<1>(t)));
		print_fmt("Found %s\n", get<1>(t));
	}
*/
	//for(int i=1; i<argc; i++) {
	for(auto song : songs) { 


		File file { song };

		print_fmt("Song:%s\n", song);

		m.lock();
		player = nullptr;
		player = unique_ptr<ChipPlayer>(psys.fromFile(file));

		if(player) {

			player->onMeta([&](const vector<string> &meta, ChipPlayer *player) {
				//for(const auto &m : meta)
				//	console->write(format("%s:%s\n", m, player->getMeta(m)));
			});
			m.unlock();

			int song = 0;
			bool next_song = false;
			while(!next_song) {
				{ lock_guard<mutex> guard(m);
					auto k = console->getKey(0);
					switch(k) {
					case Console::KEY_RIGHT:
						player->seekTo(++song);
						break;
					case Console::KEY_LEFT:
						player->seekTo(--song);
						break;
					case ' ':
						next_song = true;
						break;
					}
				}
				sleepms(100);
			}

		} else {
			m.unlock();
			print_fmt("%s FAILED\n", song);
		}
	}
	return 0;
}