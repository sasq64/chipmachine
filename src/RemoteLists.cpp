#include "RemoteLists.h"

namespace chipmachine {

using namespace std;
using namespace utils;

#define API_VERSION 1

#define HOST_NAME "http://wired-height-596.appspot.com/"
//#define HOST_NAME "http://localhost:8080/"

RemoteLists::RemoteLists() : rpc(HOST_NAME), done(true) {
	utils::File f { File::getConfigDir() + "userid" };
	if(!f.exists()) {
		random_device rd;
	    default_random_engine re(rd());
	    uniform_int_distribution<uint64_t> dis;
	    trackid = dis(re);
	    f.write(trackid);
	} else {
		trackid = f.read<uint64_t>();
	}
    f.close();
    LOGD("#### TRACKID %016x", trackid);
}

//~RemoteLists();
void RemoteLists::song_played(const string &fileName) {

	if(!done) {
		LOGD("Tracking still in progress!");
		return;
	}

	done = false;

	JSon json;
	json.add("uid", to_string(trackid));
	json.add("path", fileName);
	json.add("version", API_VERSION);

	rpc.post("song_played", json.to_string(), [=](const string &result) {
		JSon json = JSon::parse(result);
		if(!checkError(json)) {
			LOGD("trackDone:" + result);
		}
		done = true;
	});
}

void RemoteLists::sendList(const vector<SongInfo> &songs, const string &name, const function<void()> done) {
	JSon json;
	json.add("uid", to_string(trackid));
	json.add("name", name);
	json.add("version", API_VERSION);
	auto sl = json.add_array("songs");
	for(const SongInfo &info : songs) {
		LOGD("SENDING %s", info.path);
		sl.add(info.path);//fn + ":" + collection.name);
	}
	rpc.post("set_list", json.to_string(), [=](const string &result) {
		JSon json = JSon::parse(result);
		if(!checkError(json)) {
			LOGD("set_list done:" + result);
			done();
		}
	});
}

bool RemoteLists::checkError(JSon &json) {

	if(!json.valid()) {
		onErrorCallback(JSON_INVALID, "Invalid response");
		return false;
	}
	try {
		int rc = json["rc"];
		string msg = json["msg"];
		if(rc != 0) {
			onErrorCallback(rc, msg);
			return false;
		}
	} catch(json_exception &e) {
		onErrorCallback(JSON_INVALID, "Invalid response");
		return false;
	}
	return true;
}

void RemoteLists::getLists(function<void(vector<string>)> f) {
	JSon json;
	json.add("version", API_VERSION);

	rpc.post("get_lists", json.to_string(), [=](const string &result) {
		vector<string> lists;
		LOGD("GET LISTS:%s", result);
		JSon json = JSon::parse(result);
		if(!checkError(json))
			return;
		for(auto pl : json["lists"]) {
			string user = pl["user"];
			string name = pl["name"];
			lists.emplace_back(name + ":" + user);
		}
		f(lists);
	});
}

void RemoteLists::login(const string &name, function<void(int)> f) {
	JSon json;
	json.add("uid", to_string(trackid));
	json.add("name", name);
	json.add("version", API_VERSION);

	rpc.post("login", json.to_string(), [=](const string &result) {
		JSon json = JSon::parse(result);
		if(!checkError(json))
			return;
		LOGD("LOGIN: " + result);
		f(0);
	});
}

void RemoteLists::getList(const string &name, function<void(const string&, const vector<string>&)> f) {

	auto parts = utils::split(name, ":");

	JSon json;
	json.add("username", parts[1]);
	json.add("name", parts[0]);
	json.add("version", API_VERSION);

	rpc.post("get_list", json.to_string(), [=](const string &result) {
		LOGD("GET LIST: %s", result);
		auto jres =  JSon::parse(result);
		if(!checkError(jres))
			return;
		string name = jres["name"];
		vector<string> songs = jres["songs"];
		for(auto &s : songs) {
			LOGD("GETTING %s", s);
		}
		f(name + ":" + parts[1], songs);
	});
}

} // namespace chipmachine