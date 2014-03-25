
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
#include <webutils/webgetter.h>

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
	WebGetter webgetter { "_files" };

	webgetter.setBaseURL("ftp://ftp.modland.com/pub/modules/");

	if(!db.has_table("song"))
		index_db(db);

	setvbuf(stdout, NULL, _IONBF, 0);
	logging::setLevel(logging::WARNING);

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

	string modlandPath = "/nas/DATA/MUSIC/MODLAND/";
	vector<string> songs;
	while(true) {
		auto line = console->getLine(">");
		makeLower(line);
		auto parts = split(line, " ");
		auto cmd = parts[0];
		parts.erase(parts.begin());
		if(cmd == "find") {
			songs.clear();

			string qs = "SELECT path,title,composer,format FROM song WHERE ";
			for(int i=0; i<parts.size(); i++) {
				if(i > 0)
					qs.append(" AND ");
				qs.append("path LIKE ?");
				parts[i] = format("%%%s%%",parts[i]);
			}
			qs.append(" COLLATE NOCASE");
			auto q = db.query<string, string, string,string>(qs, parts);
			int i = 0;
			while(q.step()) {
				auto t = q.get_tuple();
				songs.push_back(get<0>(t));
				console->write(format("[%02d] %s / %s (%s)\n", i++, get<1>(t), get<2>(t), get<3>(t)));
			}
			console->write(qs + "\n");
		} else if(cmd == "play") {
			int n = stol(parts[0]);

			File file { modlandPath + songs[n] };
			console->write(format("Song:%s\n", songs[n]));
			if(!webgetter.inCache(songs[n]))
				console->write("Downloading from modland...\n");
			webgetter.getURL(songs[n], [&](const WebGetter::Job &job) mutable {
				lock_guard<mutex> guard(m);
				player = nullptr;
				File f = File { job.getFile() };
				player = unique_ptr<ChipPlayer>(psys.fromFile(f));				
			});
		}

	}
	return 0;
}