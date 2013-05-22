#include "URLPlayer.h"
#include "utils.h"

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

	if(protocol == "http") {
		string musicUrl = protocol.append(":/").append(path);
		urlJob = webGetter.getURL(musicUrl);
	}
	else {
		printf("Loading file '%s'\n", path.c_str());
		File file(path);
		//currentPlayer = new ModPlayer{file.getPtr(), file.getSize()};
		currentPlayer = playerFactory->fromFile(file);
	}
};


int URLPlayer::getSamples(short *target, int noSamples) override {

	if(!currentPlayer) {
		if(urlJob) {
			if(urlJob->isDone()) {

				string target = urlJob->getFile();
#if 0
				if(Extractor.canHandle(target)) {
					Archive a = Extractor.openArchive(target);
					if(deepPath.length() > 0) {
						target = a.extract(deepPath, "_cache");
					} else {
						for(const string &name: a.names()) {
							if(name.rfind(".mod") != string::npos) {
								target = a.extract(entry, "_cache");
							}
						}
					}
				}
#endif
				File file(target);
				if(file.exists()) {
					currentPlayer = playerFactory->fromFile(file); //new ModPlayer {file.getPtr(), file.getSize()};
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
