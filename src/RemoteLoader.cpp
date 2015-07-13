#include "RemoteLoader.h"

#include <coreutils/log.h>

using namespace std;
using namespace utils;

void RemoteLoader::registerSource(const std::string &name, const std::string url, const std::string local_dir) {
	Source s(url, local_dir);

	//LOGD("REGISTER %s", name);

	//if(s.url[s.url.length()-1] != '/')
	//	s.url += '/';
	if(s.local_dir[s.local_dir.length()-1] != '/')
		s.local_dir += '/';
	sources[name] = s;
}


//bool RemoteLoader::load(const std::vector<std::string> &paths, function<void(File f[])> done_cb) {
//	return false;
//}

// modland:Protracker/X/Y.mod"
bool RemoteLoader::inCache(const std::string &p) const {
	Source source;
	string path = p;
	const auto parts = split(path, "::");
	if(parts.size() > 1) {
		source = sources.at(parts[0]);
		path = parts[1];
	}

	string local_path = source.local_dir + path;
	if(File::exists(local_path))
		return true;

	string url = source.url + path;

	if(url.find("snesmusic.org") != string::npos) {
		url = url.substr(0, url.length()-4);
	}

	return webgetter.inCache(url);
}


bool RemoteLoader::load(const std::string &p, function<void(File f)> done_cb) {

	Source source;
	string path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1) {
		source = sources[parts[0]];
		path = parts[1];
	}

	string local_path = source.local_dir + path;
	LOGD("Local path: %s", local_path);
	if(File::exists(local_path)) {
		schedule_callback([=]() { done_cb(File(local_path)); });
		return true;
	}

	string url = source.url + path;

	if(url.find("snesmusic.org") != string::npos) {
		url = url.substr(0, url.length()-4);
	}

	lastSession = webgetter.getFile(url, [=](File f) {
		string fileName = f.getName();
		if(fileName.find("snesmusic.org") != string::npos) {
			auto newFile = fileName + ".rsn";
			rename(fileName.c_str(), newFile.c_str());
			f = File { newFile };
		}
		done_cb(f);
	});
	return true;
}

void RemoteLoader::preCache(const std::string &path) {

}


bool RemoteLoader::stream(const std::string &p, std::function<bool(const uint8_t *data, int size)> data_cb) {

	Source source;
	string path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1) {
		source = sources[parts[0]];
		path = parts[1];
	}

	string local_path = source.local_dir + path;
	//if(File::exists(local_path)) {
	//	done_cb(File(local_path));
	//	return true;
	//}

	string url = source.url + path;

	lastSession = webgetter.streamData(url, data_cb);
	return true;
}


