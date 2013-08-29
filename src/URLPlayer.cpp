#include "URLPlayer.h"
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <coreutils/log.h>
#include "inject.h"
#include <cstring>
#include <algorithm>

using namespace std;
using namespace utils;
using namespace logging;

URLPlayer::URLPlayer(const string &url, PlayerFactory *playerFactory) : webGetter("_cache"), currentPlayer(nullptr), playerFactory(playerFactory) {
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

	LOGD("%d '%s' '%s'", st.noParts(), protocol, path);

	if(protocol == "http" || protocol == "ftp") {
		string musicUrl = protocol.append(":/").append(path);
		vector<string> urlList = { musicUrl };

		injection_point("URLPlayer.getURL", urlList);

		for(auto u : urlList) {
			urlJobs.push_back(unique_ptr<WebGetter::Job>(webGetter.getURL(u)));
		}
	}
	else {
		File file(path);
		currentPlayer = unique_ptr<ChipPlayer>(playerFactory->fromFile(file));
	}
};

void URLPlayer::seekTo(int song, int seconds) {
	if(currentPlayer)
		return currentPlayer->seekTo(song, seconds);
}

string URLPlayer::getMeta(const string &what) {
	if(currentPlayer)
		return currentPlayer->getMeta(what);
	return "";
}

int URLPlayer::getSamples(int16_t *target, int noSamples) {

	if(!currentPlayer) {
		if(urlJobs.size() > 0) {
			bool done = all_of(urlJobs.begin(), urlJobs.end(), [](unique_ptr<WebGetter::Job> &job) {
				return job->isDone();
			});

			if(done) {

				for(auto &job : urlJobs) {
					if(job->getReturnCode() != 0) {
						LOGD("Job failed, fail song");
						return -1;
					}
				}

				string target = urlJobs[0]->getFile();
				File file(target);

				if(Archive::canHandle(target)) {
					Archive *a = Archive::open(target, "_cache");
					for(const string &name : *a) {
						LOGD("%s", name.c_str());
						if(playerFactory->canHandle(name)) {
							LOGD("We can handle %s", name);
							file = a->extract(name);
							break;
						}
					}
				}
				
				if(file.exists()) {
					if(playerFactory->canHandle(file.getName())) {
						LOGD("Trying %s", file.getName());
						//rrentPlayer = playerFactory->fromFile(file); //new ModPlayer {file.getPtr(), file.getSize()};
						currentPlayer = unique_ptr<ChipPlayer>(playerFactory->fromFile(file));
						if(currentPlayer != nullptr) {
							for(auto cb : callbacks)
								currentPlayer->onMeta(cb);
						} else
							LOGD("%s failed to initialize", file.getName());

					} else
						LOGD("Can not handle %s", file.getName());
				}
				urlJobs.resize(0);
			}
		}

	}
	if(currentPlayer)
		return currentPlayer->getSamples(target, noSamples);
	else {
		memset(target, 0, noSamples * 2);
		return noSamples;
	}

	//sleepms(100);
	return 0;
}
