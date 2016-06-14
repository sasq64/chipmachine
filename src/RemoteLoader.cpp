#include "RemoteLoader.h"

#include <coreutils/log.h>

using namespace std;
using namespace utils;
//using namespace webutils;

RemoteLoader::RemoteLoader() { //: webgetter(std::make_unique<webutils::Web>(utils::File::getCacheDir() / "_webfiles")) {
	cacheDir = utils::File::getCacheDir() / "_webfiles";
	utils::makedirs(cacheDir);
	// webgetter->setErrorCallback([](int code, const string &msg) {
	//	LOGD("Error %d %s", code, msg);
	//});
}

RemoteLoader::~RemoteLoader() {
}

void RemoteLoader::registerSource(const std::string &name, const std::string url,
                                  const std::string local_dir) {
	Source s(url, local_dir);
	if(s.local_dir != "" && !endsWith(s.local_dir, "/"))
		s.local_dir += "/";
	sources[name] = s;
}

void RemoteLoader::cancel() {
	if(lastStream)
		lastStream.cancel();
}

void RemoteLoader::update() { webutils::poll(); }

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
		url = url.substr(0, url.length() - 4);
	}
	
	File targetFile = cacheDir / utils::urlencode(url, ":/\\?;");

	return targetFile.exists();

}

bool RemoteLoader::isOffline(const std::string &p) {

	Source source;
	string path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1) {
		source = sources[parts[0]];
		path = parts[1];
	}

	string local_path = source.local_dir + path;
	return inCache(p) || File::exists(local_path);
}

bool RemoteLoader::load(const std::string &p, const function<void(File f)> &done_cb) {

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
		done_cb(File(local_path));
		return true;
	}

	string url = source.url + path;

	if(url.find("snesmusic.org") != string::npos) {
		url = url.substr(0, url.length() - 4);
	}

	File targetFile = cacheDir / utils::urlencode(url, ":/\\?;");
	if(targetFile.exists()) {
		done_cb(targetFile);
		return true;
	}

	lastFile = webutils::get<File>(url, targetFile).onDone([=](File &f) {
		//LOGD("CODE %d", job.code());
		//auto f = job.file();
		string fileName = f.getName();
		if(fileName.find("snesmusic.org") != string::npos) {
			auto newFile = fileName + ".rsn";
			rename(fileName.c_str(), newFile.c_str());
			f = File{newFile};
		}
		done_cb(f);
	});
	return true;
}

void RemoteLoader::preCache(const std::string &path) {}

struct Stream {

};

//std::shared_ptr<webutils::WebJob>
bool
RemoteLoader::stream(const std::string &p,
                     std::function<bool(int what, const uint8_t *data, int size)> data_cb) {

	Source source;
	string path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1) {
		source = sources[parts[0]];
		path = parts[1];
	}

	string local_path = source.local_dir + path;
	// if(File::exists(local_path)) {
	//	done_cb(File(local_path));
	//	return true;
	//}

	string url = source.url + path;
	bool doneHeaders = false;

	lastStream = webutils::get<std::function<bool(uint8_t *, size_t)>>(url, 
			[=](uint8_t *ptr, size_t size) mutable -> bool {
				if(!doneHeaders) {
					string s = lastStream.getHeader("icy-metaint");
					if(s != "") {
						data_cb(PARAMETER, (uint8_t*)"icy-interval", stol(s));
					}
			    	if(lastStream.contentLength() > 0)
				    	data_cb(PARAMETER, (uint8_t *)"size", lastStream.contentLength());
					doneHeaders = true;
				}
				data_cb(0, ptr, size);
				return true;
			});
	return false;
}
