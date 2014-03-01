#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

//#include <coreutils/log.h>

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <functional>
#include <vector>

namespace chipmachine {

class ChipPlayer {
public:
	typedef std::function<void(const std::vector<std::string> &meta, ChipPlayer*)> Callback;

	virtual ~ChipPlayer() {}
	virtual int getSamples(int16_t *target, int size) = 0;

	virtual std::string getMeta(const std::string &what) { 
		return metaData[what];
	};

	int getMetaInt(const std::string &what) { 
		const std::string &data = getMeta(what);
		int i = atoi(data.c_str());
		return i;
	};

	void setMeta() {
		for(auto cb : callbacks) {
			//LOGD("Calling callback for '%s'", meta);
			cb(changedMeta, this);
		}
		changedMeta.clear();
	}

	template <typename... A> void setMeta(const std::string &what, int value, const A& ...args) {
		metaData[what] = std::to_string(value);
		changedMeta.push_back(what);
		setMeta(args...);
	}

	template <typename... A> void setMeta(const std::string &what, const std::string &value, const A& ...args) {
		metaData[what] = value;
		changedMeta.push_back(what);
		setMeta(args...);
	}

	//template <typename T> void setMeta(const std::string &what, T& value) {
	//	metaData[what] = "";
	//}

/*
	virtual const std::unordered_map<std::string, std::string> &getMetaData();
	
	virtual const std::string getString(std::string what) {
		return getMetaData()[what];
	}

	virtual int getInt(std::string what) {
		stoi()
		int i = atoi(getMetaData()[what]);
		return i;
	}
*/
	virtual void seekTo(int song, int seconds = -1) { printf("NOT IMPLEMENTED\n"); }
/*
	virtual void onMeta(Callback callback) {
		//LOGD("Setting callback in %p", this);
		callbacks.push_back(callback);
		for(auto &md : metaData) {
			//LOGD("Calling callback for '%s'", md.first);
			callback(md.first, this);
		}
	}
*/
protected:

	virtual void setMetaData(const std::string &meta, const std::string &value) {
		metaData[meta] = value;
		////LOGD("Setting meta '%s' in %p", meta, this);
		//for(auto cb : callbacks) {
			//LOGD("Calling callback for '%s'", meta);
		//	cb(meta, this);
		//}
	};

	virtual void setMetaData(const std::string &meta, int value) {

#ifdef ANDROID
		char tmp[32];
		sprintf(tmp, "%d", value);
		setMetaData(meta, tmp);
#else
		setMetaData(meta, std::to_string(value));
#endif
	};

	//virtual void metaDataEnd() {
	//	setMetaData("metaend", "");
//	}

	std::unordered_map<std::string, std::string> metaData;
	std::vector<Callback> callbacks;
	std::vector<std::string> changedMeta;

};

}

#endif // CHIP_PLAYER_H