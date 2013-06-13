#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include "log.h"

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <unordered_map>
#include <functional>
#include <vector>

class ChipPlayer {
public:
	typedef std::function<void(const std::string &meta, ChipPlayer*)> Callback;

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

	virtual void onMeta(Callback callback) {
		LOGD("Setting callback in %p", this);
		callbacks.push_back(callback);
		for(auto &md : metaData) {
			LOGD("Calling callback for '%s'", md.first);
			callback(md.first, this);
		}
	}

protected:

	virtual void setMetaData(const std::string &meta, const std::string &value) {
		metaData[meta] = value;
		LOGD("Setting meta '%s' in %p", meta, this);
		for(auto cb : callbacks) {
			LOGD("Calling callback for '%s'", meta);
			cb(meta, this);
		}
	};

	virtual void setMetaData(const std::string &meta, int value) {
		setMetaData(meta, std::to_string(value));
	};

	virtual void metaDataEnd() {
		setMetaData("metaend", "");
	}

	std::unordered_map<std::string, std::string> metaData;
	std::vector<Callback> callbacks;

};

#endif // CHIP_PLAYER_H