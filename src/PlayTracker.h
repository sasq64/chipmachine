#ifndef PLAY_TRACKER_H
#define PLAY_TRACKER_H

#include "MusicDatabase.h"
#include <coreutils/format.h>
#include <coreutils/log.h>
#include <webutils/webrpc.h>

#include <random>

namespace chipmachine {

class PlayTracker {
public:
	PlayTracker() : rpc("http://wired-height-596.appspot.com/"), done(true) {
		utils::File f { ".trackid" };
		if(!f.exists()) {
			std::random_device rd;
		    std::default_random_engine re(rd());
		    std::uniform_int_distribution<uint64_t> dis;
		    trackid = dis(re);
		    f.write(trackid);
		} else {
			trackid = f.read<uint64_t>();
		}
	    f.close();
	    LOGD("#### TRACKID %016x", trackid);
	}
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
		rpc.post("song_played", utils::format("{ \"id\" : \"%x\", \"path\" : \"%s\", \"collection\" : \"%s\" }", trackid, fn, collection), [=](const std::string &result) {
			LOGD("trackDone");
			done = true;
		});
	}

	void favorite(const std::string &fileName, bool add) {
		auto fn = fileName;
		auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);

		LOGD("FAV %s %s", collection, fn);
		done = false;
		rpc.post(add ? "add_favorite" : "del_favorite", utils::format("{ \"id\" : \"%x\", \"path\" : \"%s\", \"collection\" : \"%s\" }", trackid, fn, collection), [=](const std::string &result) {
			LOGD("favDone");
			done = true;
		});
	}

	static PlayTracker& getInstance() {
		static PlayTracker tracker;
		return tracker;
	}

private:
	uint64_t trackid;
	WebRPC rpc;
	std::atomic<bool> done;
};

}

#endif // PLAY_TRACKER_H
