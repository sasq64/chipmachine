#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <unordered_map>

class ChipPlayer {
public:

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

protected:

	virtual void setMetaData(const std::string &meta, const std::string &value) {
		metaData[meta] = value;
	};

	virtual void setMetaData(const std::string &meta, int value) {
		metaData[meta] = std::to_string(value);
	};

private:
	std::unordered_map<std::string, std::string> metaData;

};

#endif // CHIP_PLAYER_H