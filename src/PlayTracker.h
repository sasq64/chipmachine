#ifndef PLAY_TRACKER_H
#define PLAY_TRACKER_H

#include "MusicDatabase.h"
#include <coreutils/format.h>
#include <coreutils/log.h>
#include <webutils/webrpc.h>

namespace chipmachine {

class PlayTracker {
public:
	PlayTracker() : rpc("http://wired-height-596.appspot.com/") {}
	//~PlayTracker();
	void play(const std::string &fileName) {

		auto fn = fileName;
		auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);

		LOGD("TRACK %s %s", collection, fn);

		rpc.post("song_played", utils::format("{ \"path\" : \"%s\", \"collection\" : \"%s\" }", fn, collection));
	}

	static PlayTracker& getInstance() {
		static PlayTracker tracker;
		return tracker;
	}

private:
	WebRPC rpc;
};

}

#endif // PLAY_TRACKER_H
