#ifndef PLAY_TRACKER_H
#define PLAY_TRACKER_H

#include "MusicDatabase.h"
#include <coreutils/format.h>
#include <coreutils/log.h>
#include <webutils/webrpc.h>
#include <grappix/grappix.h>
#include <json/json.h>
#include <random>
#include <set>
#include <vector>
namespace chipmachine {
//wired-height-596.appspot.com
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

		LOGD("TRACK %s %s", collection.name, fn);
		done = false;

		JSon json;
		json.add("uid", std::to_string(trackid));
		json.add("path", fn);
		json.add("collection", collection.name);
		//json("path") = fn;
		//json("collection") = collection;
		rpc.post("song_played", json.to_string(), [=](const std::string &result) {
			//rpc.post("song_played", utils::format("{ \"id\" : \"%x\", \"path\" : \"%s\", \"collection\" : \"%s\" }", trackid, fn, collection), [=](const std::string &result) {
			LOGD("trackDone:" + result);
			done = true;
		});
	}

	void sendList(const std::vector<SongInfo> &songs, const std::string &name) {
		JSon json;
		json.add("uid", std::to_string(trackid));
		json.add("name", name);
		auto sl = json.add_array("songs");
		for(const SongInfo &info : songs) {
			auto fn = info.path;
			auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);
			sl.add(fn + ":" + collection.name);
		}
		rpc.post("set_list", json.to_string(), [=](const std::string &result) {
			LOGD("set_list done:" + result);
		});
	}

	void getLists(std::function<void(std::vector<std::string>)> f) {
		rpc.call("get_lists",  [=](const std::string &result) {
			std::vector<std::string> lists;
			LOGD("GET LISTS:%s", result);
			JSon json = JSon::parse(result);
			for(auto pl : json("lists")) {
				std::string user = pl("user");
				std::string name = pl("name");
				lists.emplace_back((std::string)pl("name") + ":" + user);
			}
			f(lists);
		});
	}

	void login(const std::string &name, std::function<void(int)> f) {
		JSon json;
		json.add("uid", std::to_string(trackid));
		json.add("name", name);
		rpc.post("login", json.to_string(), [=](const std::string &result) {
			LOGD("LOGIN: " + result);
			f(0);
		});
	}

	void getList(const std::string &name, std::function<void(const std::string&, const std::vector<std::string>&)> f) {

		auto parts = utils::split(name, ":");

		JSon json;
		json.add("username", parts[1]);
		json.add("name", parts[0]);

		rpc.post("get_list", json.to_string(), [=](const std::string &result) {
			LOGD("GET LIST: %s", result);
			auto jres =  JSon::parse(result);
			std::string name = jres("name");
			std::vector<std::string> songs = jres("songs");
			f(name + ":" + parts[1], songs);
		});
	}

	void updateList(const std::vector<SongInfo> &songs) {
		std::set<std::string> lastset;
		std::set<std::string> currset;
		utils::File f { ".last_favorites" };
		for(const SongInfo &info : songs) {
			currset.insert(info.path);
		}
		for(const auto &line : f.getLines()) {
			lastset.insert(line);
			if(currset.count(line) == 0)
				favorite(line, false);
		}
		for(const SongInfo &info : songs) {
			if(lastset.count(info.path) == 0)
				favorite(info.path, true);
		}
		f.clear();
		for(const SongInfo &info : songs) {
			f.writeln(info.path);
		}
	}

	void favorite(const std::string &fileName, bool add) {
		auto fn = fileName;
		auto collection = MusicDatabase::getInstance().stripCollectionPath(fn);

		LOGD("FAV %s %s", collection.name, fn);
		done = false;
		rpc.post(add ? "add_favorite" : "del_favorite", utils::format("{ \"id\" : \"%x\", \"path\" : \"%s\", \"collection\" : \"%s\" }", trackid, fn, collection.name), [=](const std::string &result) {
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
