
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>

namespace utils {

typedef unsigned int uint;

class File {
public:
	File(const std::string &name);
	void read();
	uint8_t *getPtr();
	int getSize() { return size; }
private:
	std::string fileName;
	std::vector<uint8_t> data;
	int size;
	bool loaded;
};

class StringTokenizer {
public:
	StringTokenizer(const std::string &s, const std::string &delim);
	int noParts() { return args.size(); }
	const std::string &getString(int no) { return args[no]; }
	const char getDelim(int no) { return delims[no]; }
private:
	std::vector<std::string> args;
	std::vector<char> delims;
};

std::string urlencode(const std::string &s, const std::string &chars);
std::string urldecode(const std::string &s, const std::string &chars);

};