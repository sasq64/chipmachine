#ifndef PLAY_TRACKER_H
#define PLAY_TRACKER_H

#include "MusicDatabase.h"
#include <coreutils/format.h>
#include <coreutils/log.h>
#include <webutils/webrpc.h>

namespace chipmachine {

class PlayTracker {
public:
	PlayTracker() : rpc("http://wired-height-596.appspot.com/"), done(true) {}
	//~PlayTracker();
	void play(const std::string &fileName) {

		if(!done) {
			LOGD("Tracking still in progress!");
			return;
		}
		auto fn = fileName;
		auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);

		LOGD("TRACK %s %s", collection, fn);
		done = false;
		rpc.post("song_played", utils::format("{ \"path\" : \"%s\", \"collection\" : \"%s\" }", fn, collection), [=](const std::string &result) {
			LOGD("trackDone");
			done = true;
		});
	}

	void favorite(const std::string &fileName, bool add) {
		auto fn = fileName;
		auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);

		LOGD("FAV %s %s", collection, fn);
		done = false;
		rpc.post("add_favorite", utils::format("{ \"uid\" : \"%s\", \"path\" : \"%s\", \"collection\" : \"%s\" }", userid, fn, collection), [=](const std::string &result) {
			LOGD("trackDone");
			done = true;
		});
	}

	static PlayTracker& getInstance() {
		static PlayTracker tracker;
		return tracker;
	}

private:
	WebRPC rpc;
	std::atomic<bool> done;
	std::string userid = "DUMMY";
};

}

#endif // PLAY_TRACKER_H
