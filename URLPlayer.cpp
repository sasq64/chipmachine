#include "URLPlayer.h"
#include "utils.h"
#include "Archive.h"

using namespace std;
using namespace utils;

URLPlayer::URLPlayer(const string &url, PlayerFactory *playerFactory) : webGetter("_cache"), currentPlayer(nullptr), urlJob(nullptr), playerFactory(playerFactory) {
	StringTokenizer st {url, ":"};
	string protocol = "";
	string path;
	int p = 0;

	if(st.noParts() > 1 && st.getString(1).substr(0,2) == "//" && st.getDelim(1) == ':') {
		protocol = st.getString(p++);
		path = st.getString(p).substr(1);
	} else {
		path = st.getString(p);
	}

	if(protocol == "http" || protocol == "ftp") {
		string musicUrl = protocol.append(":/").append(path);
		urlJob = webGetter.getURL(musicUrl);
	}
	else {
		File file(path);
		currentPlayer = playerFactory->fromFile(file);
	}
};


int URLPlayer::getSamples(int16_t *target, int noSamples) {

	printf("%p\n", currentPlayer);
	if(!currentPlayer) {
		if(urlJob) {
			if(urlJob->isDone()) {

				string target = urlJob->getFile();
				File file(target);

				if(Archive::canHandle(target)) {
					Archive *a = Archive::open(target, "_cache");
					for(const string &name : *a) {
						printf("%s\n", name.c_str());
						if(playerFactory->canHandle(name)) {
							printf("We can handle %s\n", name.c_str());
							file = a->extract(name);
							break;
						}
					}
				}
				
				if(file.exists()) {
					if(playerFactory->canHandle(file.getName())) {
						printf("Trying %s\n", file.getName().c_str());
						currentPlayer = playerFactory->fromFile(file); //new ModPlayer {file.getPtr(), file.getSize()};
						printf("%p\n", currentPlayer);
					} else
						printf("Can not handle %s\n", file.getName().c_str());
				}
				urlJob = nullptr;
			}
		}

	}
	if(currentPlayer)
		return currentPlayer->getSamples(target, noSamples);
	sleepms(100);
	return 0;
}
