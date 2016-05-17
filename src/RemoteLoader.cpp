#include "RemoteLoader.h"

#include <coreutils/log.h>
#include <webutils/web.h>

using namespace std;
using namespace utils;

RemoteLoader::RemoteLoader() : webgetter(std::make_unique<webutils::Web>(utils::File::getCacheDir() / "_webfiles")) {
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
	if(lastSession)
		lastSession->stop();
	lastSession = nullptr;
}

void RemoteLoader::update() { webgetter->poll(); }

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

	return webgetter->inCache(url);
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
		schedule_callback([=]() { done_cb(File(local_path)); });
		return true;
	}

	string url = source.url + path;

	if(url.find("snesmusic.org") != string::npos) {
		url = url.substr(0, url.length() - 4);
	}

	lastSession = webgetter->getFile(url, [=](webutils::WebJob job) {
		LOGD("CODE %d", job.code());
		auto f = job.file();
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

std::shared_ptr<webutils::WebJob>
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
	bool headers = false;
	lastSession = webgetter->streamData(
	    url, [=](webutils::WebJob &job, uint8_t *data, int size) mutable -> bool {
		    if(!headers) {
			    string s = job.getHeader("icy-metaint");
			    if(s != "") {
				    int mi = stol(s);
				    data_cb(PARAMETER, (uint8_t *)"icy-interval", mi);
			    }
			    LOGD("CONTENT LENGTH %d", job.contentLength());
			    if(job.contentLength() > 0)
				    data_cb(PARAMETER, (uint8_t *)"size", job.contentLength());
			    headers = true;
		    }
		    if(data == nullptr)
			    return data_cb(END, nullptr, size);
		    return data_cb(DATA, data, size);
		});
	return lastSession;
}
