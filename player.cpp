
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include <vector>
#include <string>
#include <memory>

#include <sqlite3.h>

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "utils.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "PSXPlugin.h"


#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include "Archive.h"

typedef unsigned int uint;
using namespace std;
using namespace utils;



class PlayerSystem : public PlayerFactory {
public:
	virtual ChipPlayer *fromFile(File &file) override {

		string name = file.getName();
		makeLower(name);
		printf("Handling %s\n", name.c_str());

		for(auto *plugin : plugins) {
			if(plugin->canHandle(name))
				return plugin->fromFile(file);
		}
		return nullptr;
	}

	virtual bool canHandle(const std::string &name) override {

		string lname = name;
		makeLower(lname);

		for(auto *plugin : plugins) {
			if(plugin->canHandle(lname))
				return true;
		}
		return false;
	}

	void registerPlugin(ChipPlugin *p) {	
		plugins.push_back(p);
	}

	ChipPlayer *play(const string &url) {
		return new URLPlayer {url, this};
	}

private:
	vector<ChipPlugin*> plugins;
};

int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("Modplayer test\n");

	/*TelnetServer telnet { 12345 };
	telnet.addCommand("play", [&](vector<string> args) {
		ChipPlayer *player = psys.play(name);
	});*/

	sqlite3 *db = nullptr;

	int rc = sqlite3_open("hvsc.db", &db);
	if(rc == SQLITE_OK) {
		printf("DB opened\n");
		sqlite3_stmt *s;
		const char *tail;
		rc = sqlite3_prepare(db, "select * from songs;", -1, &s, &tail);
		if(rc == SQLITE_OK) {
			printf("Statement created\n");
			while(true) {
				sqlite3_step(s);
				const char *title = (const char *)sqlite3_column_text(s, 2);
				printf("title %s\n", title);
			}
		} else
			printf("%s\n", sqlite3_errmsg(db));
	} else
		printf("%s\n", sqlite3_errmsg(db));

	string name;

	if(argc > 1)
		name = argv[1];
	else
		name = "http://swimsuitboys.com/droidmusic/C64%20Demo/Amplifire.sid";



	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {});
	psys.registerPlugin(new PSFPlugin {});

	ChipPlayer *player = psys.play(name);

	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	while(true) {
		int rc = player->getSamples(&buffer[0], bufSize);
		printf("%d\n", rc);
		if(rc > 0)
			ap.writeAudio(&buffer[0], rc);
	}
	return 0;
}