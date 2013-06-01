#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>

namespace utils {

typedef unsigned int uint;

class File {
public:
	File();
	File(const std::string &name);
	void read();
	void write(const uint8_t *data, int size);
	void close();

	bool exists();
	uint8_t *getPtr();
	const std::string &getName() { return fileName; }
	int getSize() { return size; }
	std::vector<std::string> getLines();
private:
	std::string fileName;
	std::vector<uint8_t> data;
	uint size;
	bool loaded;
	FILE *writeFP;
	FILE *readFP;
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

void sleepms(uint ms);
void makedir(const std::string &name);
void makedirs(const std::string &name);

bool endsWith(const std::string &name, const std::string &ext);
void makeLower(std::string &s);


void percent_replace(std::string &x, size_t pos);
size_t format_replace(std::string &fmt, size_t pos, int len, const std::string &arg);
size_t format_replace(std::string &fmt, size_t pos, int len, const char * const arg);
size_t format_replace(std::string &fmt, size_t pos, int len, char * const arg);

template<class T>
size_t format_replace(std::string &fmt, size_t pos, int len, const T &arg) {
	std::string s = std::to_string((long long)arg);
	fmt.replace(pos, len, s);
	return pos + s.length();
}

template <class T>
size_t format_inplace(std::string &fmt, size_t pos, const T& arg) {
	//size_t pos = 0;
	while(pos < fmt.length()) {
		pos = fmt.find_first_of('%', pos);
		if(pos == std::string::npos)
			return -1;
		if(fmt[pos+1] != '%')
			break;
		fmt.replace(pos, 2, "%");
		pos += 1;
	}
	switch(fmt[pos+1]) {
	case 's':
		pos = format_replace(fmt, pos, 2, arg);
		return pos;
	case 'd':
		pos = format_replace(fmt, pos, 2, arg);
		return pos;
	}
	return -2;
}

template <class A, class... B>
size_t format_inplace(std::string &fmt, size_t pos, A head, B... tail)
{
	pos = format_inplace(fmt, pos, head);
	pos = format_inplace(fmt, pos, tail...);
	return pos;
}

template <class T>
std::string format(const std::string &fmt, const T& arg) {
	std::string fcopy = fmt;
	size_t pos = format_inplace(fcopy, 0, arg);
	percent_replace(fcopy, pos);
	return fcopy;  
}

std::string format(const std::string &fmt);

template <class A, class... B>
std::string format(const std::string &fmt, A head, B... tail)
{
	std::string fcopy = fmt;
	size_t pos = format_inplace(fcopy, 0, head);
	pos = format_inplace(fcopy, pos, tail...);
	percent_replace(fcopy, pos);
	return fcopy;
}


};

#endif // UTILS_H
