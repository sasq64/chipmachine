#ifndef REMOTE_LOADER_H
#define REMOTE_LOADER_H

#include <net/webgetter.h>
#include <coreutils/file.h>

#include <string>
#include <functional>
#include <unordered_map>

class RemoteLoader {
public:
	RemoteLoader();

	void registerSource(const std::string &name, const std::string url,
	                    const std::string local_dir);

	bool load(const std::string &path, std::function<void(utils::File)> done_cb);

	bool stream(const std::string &path,
	            std::function<bool(const uint8_t *data, int size)> data_cb);

	void preCache(const std::string &path);

	bool inCache(const std::string &path) const;

	static RemoteLoader &getInstance() {
		static RemoteLoader loader;
		return loader;
	}

	void cancel() {
		if(lastSession)
			lastSession->stop();
		lastSession = nullptr;
	}

private:
	struct Source {
		Source() {}
		Source(const std::string &url, const std::string &ld) : url(url), local_dir(ld) {}
		std::string url;
		std::string local_dir;
	};

	std::unordered_map<std::string, Source> sources;

	net::WebGetter webgetter;
	std::shared_ptr<net::HttpSession> lastSession;
};

#endif // REMOTE_LOADER_H
