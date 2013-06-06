#ifndef CHIP_PLAYER_H
#define CHIP_PLAYER_H

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <unordered_map>

class ChipPlayer {
public:

	//static const std::string TITLE = "title";
	//static const char *LENGTH = "length";
/*
	class MetaData {
	public:
		typedef enum {
			INT,
			STRING
		} Type;

		MetaData(const std::string &what, int value) : what(what), intValue(value), type(INT) {}
		MetaData(const std::string &what, const std::string &value) : what(what), strValue(value), type(STRING) {}

		int getInt() { return intValue; }
		std::string getString() { return strValue; }

	private:
		int intValue;
		std::string strValue;
		Type type;

	};*/

	virtual ~ChipPlayer() {}
	virtual int getSamples(int16_t *target, int size) = 0;

	virtual std::string getMetaData(const std::string &what) { return ""; };
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
};

#endif // CHIP_PLAYER_H