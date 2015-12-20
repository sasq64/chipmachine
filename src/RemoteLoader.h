#ifndef REMOTE_LOADER_H
#define REMOTE_LOADER_H

#include <webutils/web.h>
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

	std::shared_ptr<webutils::Web::Job>
	stream(const std::string &path,
	       std::function<bool(int what, const uint8_t *data, int size)> data_cb);

	void preCache(const std::string &path);

	bool inCache(const std::string &path) const;

	bool isOffline(const std::string &p);

	static RemoteLoader &getInstance() {
		static RemoteLoader loader;
		return loader;
	}

	void cancel() {
		if(lastSession)
			lastSession->stop();
		lastSession = nullptr;
	}

	void update() { webgetter.poll(); }

	static constexpr int DATA = 0;
	static constexpr int PARAMETER = 1;
	static constexpr int END = 2;

private:
	struct Source {
		Source() {}
		Source(const std::string &url, const std::string &ld) : url(url), local_dir(ld) {}
		std::string url;
		std::string local_dir;
	};

	std::unordered_map<std::string, Source> sources;

	webutils::Web webgetter;
	std::shared_ptr<webutils::Web::Job> lastSession;
};

#endif // REMOTE_LOADER_H
