#ifndef REMOTE_LISTS_H
#define REMOTE_LISTS_H

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
class RemoteLists {
public:
	RemoteLists();
	//~RemoteLists();
	void songPlayed(const std::string &fileName);
	void sendList(const std::vector<SongInfo> &songs, const std::string &name, const std::function<void()> done);
	void getLists(std::function<void(std::vector<std::string>)> f);
	void login(const std::string &name, std::function<void(int)> f);
	void getList(const std::string &name, std::function<void(const std::string&, const std::vector<std::string>&)> f);
	static RemoteLists& getInstance() {
		static RemoteLists tracker;
		return tracker;
	}

	void onError(const std::function<void(int, const std::string)> callback) { onErrorCallback = callback; }

	enum {
		INVALID_VERSION = -1,
		NO_SUCH_PLAYLIST = -3,
		NAME_ALREADY_TAKEN = -4,

		JSON_INVALID = -99
	};

private:
	bool parse(const std::string &data, JSon &result);
	bool checkError(JSon &json);

	uint64_t trackid;
	WebRPC rpc;
	std::atomic<bool> done;
	std::function<void(int, const std::string)> onErrorCallback;
};

}

#endif // REMOTE_LISTS_H
