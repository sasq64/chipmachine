#include "URLPlayer.h"
#include "utils.h"
#include "Archive.h"
#include "log.h"

using namespace std;
using namespace utils;
using namespace logging;

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

void URLPlayer::seekTo(int song, int seconds) {
	if(currentPlayer)
		return currentPlayer->seekTo(song, seconds);
}

std::string URLPlayer::getMetaData(const std::string &what) {
	if(currentPlayer)
		return currentPlayer->getMetaData(what);
	return "";
}

int URLPlayer::getSamples(int16_t *target, int noSamples) {

	if(!currentPlayer) {
		if(urlJob) {
			if(urlJob->isDone()) {

				string target = urlJob->getFile();
				File file(target);

				if(Archive::canHandle(target)) {
					Archive *a = Archive::open(target, "_cache");
					for(const string &name : *a) {
						LOGD("%s\n", name.c_str());
						if(playerFactory->canHandle(name)) {
							LOGD("We can handle %s\n", name);
							file = a->extract(name);
							break;
						}
					}
				}
				
				if(file.exists()) {
					if(playerFactory->canHandle(file.getName())) {
						LOGD("Trying %s\n", file.getName());
						currentPlayer = playerFactory->fromFile(file); //new ModPlayer {file.getPtr(), file.getSize()};
					} else
						LOGD("Can not handle %s\n", file.getName());
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
