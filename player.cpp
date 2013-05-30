
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>

#include <sqlite3.h>

#include "ChipPlugin.h"
#include "ChipPlayer.h"
#include "URLPlayer.h"

#include "utils.h"

#include "ModPlugin.h"
#include "VicePlugin.h"
#include "PSXPlugin.h"

#include "TelnetServer.h"


#ifdef WIN32
#include "AudioPlayerWindows.h"
#else
#include "AudioPlayerLinux.h"
#endif

#include "Archive.h"

#include <unistd.h>

typedef unsigned int uint;
using namespace std;
using namespace utils;

// This is our base-case for the print function:
template <class T>
void formatX(std::string &fmt, const T& arg) {
	size_t pos = fmt.find_first_of('%');
	if(pos != std::string::npos) {
		fmt.replace(pos, 2, std::to_string(arg));
	}	  
}

template <>
void formatX<int>(std::string &fmt, const int& arg) {
	size_t pos = fmt.find_first_of('%');
	if(pos != std::string::npos) {
		fmt.replace(pos, 2, std::to_string(arg));
	}	  
}

template <>
void formatX<std::string>(std::string &fmt, const std::string& arg) {
	size_t pos = fmt.find_first_of('%');
	if(pos != std::string::npos) {
		fmt.replace(pos, 2, arg);
	}
}

template <class A, class... B>
void formatX(std::string &fmt, A head, B... tail)
{
	formatX(fmt, head);
	formatX(fmt, tail...);
}


template <class T>
std::string format(const std::string &fmt, const T& arg) {
	std::string fcopy = fmt;
	formatX(fcopy, arg);
	return fcopy;  
}

std::string format(const std::string &fmt) {
	return fmt;
}

// And this is the recursive case:

template <class A, class... B>
std::string format(const std::string &fmt, A head, B... tail)
{
	std::string fcopy = fmt;
	formatX(fcopy, head);
	formatX(fcopy, tail...);
	return fcopy;
}



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
	printf("Chipmachine starting\n");

	bool daemonize = false;
	queue<string> playQueue;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if((strcmp(argv[i], "--start-daemon") == 0) || (strcmp(argv[i], "-d") == 0)) {
				daemonize = true;
			}
		} else 
			playQueue.push(argv[1]);
	}
#ifndef WIN32
	if(daemonize)
		daemon(0, 0);
#endif


	mutex playMutex;

	File startSongs { "/opt/chipmachine/startsongs" };
	if(startSongs.exists()) {
		for(string s : startSongs.getLines()) {
			playQueue.push(s);
		}
		startSongs.close();
	}
/*
	FILE *fp = fopen("/opt/chipmachine/startsongs", "rb");
	if(fp) {		
		char buffer[2048];
		while(true) {
			char *line = fgets(buffer, sizeof(buffer), fp);
			if(line) {
				int len = strlen(line);
				while(len > 0 && line[len-1] == '\n' || line[len-1] == '\r')
					len--;
				line[len] = 0;
				playQueue.push(line);
			} else
				break;
		}
		fclose(fp);
	}
*/

	PlayerSystem psys;
	psys.registerPlugin(new ModPlugin {});
	psys.registerPlugin(new VicePlugin {});
	psys.registerPlugin(new PSFPlugin {});

	ChipPlayer *player = nullptr; //psys.play(name);

	int frameCount = 0;
	string songName;

	TelnetServer telnet { 12345 };
	telnet.addCommand("play", [&](TelnetServer::User &user, const vector<string> &args) {
		//printf("Play '%s'\n", args[1].c_str());
		playMutex.lock();
		playQueue.push(args[1]);
		playMutex.unlock();
	});

	telnet.addCommand("status", [&](TelnetServer::User &user, const vector<string> &args) {
		if(player) {
			char temp[2048];
			sprintf(temp, "Playing '%s' for %d seconds\n", songName.c_str(), frameCount / 44100);
			user.write(temp);
		} else
			user.write("Nothing playing\n");
	});


	telnet.setConnectCallback([&](TelnetServer::User &user) {
		user.write("Chipmachine v0.1\n");
	});

	telnet.runThread();
	songName = "yo";
	string s = format("test %s %d", songName, 4);
	printf(s.c_str());

	//else
		//name = "ftp://modland.ziphoid.com/pub/modules/Protracker/Heatbeat/cheeseburger.mod";
		//name = "http://swimsuitboys.com/droidmusic/C64%20Demo/Amplifire.sid";



	//if(name.length() > 0)
	//	player = psys.play(name);



	AudioPlayerNative ap;
	int bufSize = 4096;
	vector<int16_t> buffer(bufSize);
	while(true) {

		playMutex.lock();
		if(!playQueue.empty()) {
			if(player)
				delete player;
			songName = playQueue.front();
			player = psys.play(songName);
			playQueue.pop();
			frameCount = 0;
		}
		playMutex.unlock();

		if(player) {
			int rc = player->getSamples(&buffer[0], bufSize);
			if(rc > 0) {
				ap.writeAudio(&buffer[0], rc);
				frameCount += rc/2;
			}
		} else
			sleepms(250);
	}
	return 0;
}