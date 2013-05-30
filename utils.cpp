#include <sys/stat.h>

#include "utils.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <iostream>
#include <sstream>

namespace utils {

using namespace std;

File::File() : size(-1), loaded(false), writeFP(nullptr), readFP(nullptr) {}

File::File(const string &name) : fileName(name), size(-1), loaded(false), writeFP(nullptr), readFP(nullptr) {
};

void File::read() {		
	FILE *fp = fopen(fileName.c_str(), "rb");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data.reserve(size);
	fread(&data[0], 1, size, fp);
	fclose(fp);
	loaded = true;
}

vector<string> File::getLines() {
	vector<string> lines;
	close();
	if(!loaded)
		read();
	string source { reinterpret_cast<char*>(&data[0]), size };
	stringstream ss(source);
	string to;

	while(getline(ss, to)) {
		lines.push_back(to);
    }
    return lines;
}

void File::write(const uint8_t *data, int size) {
	if(!writeFP) {
		makedirs(fileName);
		writeFP = fopen(fileName.c_str(), "wb");	
	}
	fwrite(data, 1, size, writeFP);
}

void File::close() {
	if(writeFP)
		fclose(writeFP);
	writeFP = nullptr;
}

bool File::exists() {
	struct stat ss;
	return (stat(fileName.c_str(), &ss) == 0);
}

uint8_t *File::getPtr() {
	close();
	if(!loaded)
		read();
	return &data[0];
}

StringTokenizer::StringTokenizer(const string &s, const string &delim) {
	char *temp = new char [ s.length()+1 ];
	char *ptr = temp;
	strcpy(temp, s.c_str());
	int pos = 0;
	while(true) {
		char *arg = strtok(ptr, delim.c_str());			
		if(arg) {
			args.push_back(string(arg));
			delims.push_back(ptr ? 0 : s[pos-1]);//ptr[-1]);
			pos += (strlen(arg)+1);
		} else
			break;
		ptr = nullptr;
	}
	delete [] temp;
}


string urlencode(const string &s, const string &chars) {
	char *target = new char [s.length() * 3 + 1];
	char *ptr = target;
	for(uint i=0; i<s.length(); i++) {
		char c = s[i];
		if(chars.find(c) != string::npos) {
			sprintf(ptr, "%%%02x", c);
			ptr += 3;
		} else
			*ptr++ = c;
	}
	*ptr = 0;
	return string(target);
}

string urldecode(const string &s, const string &chars) {
	char *target = new char [s.length() + 1];
	char *ptr = target;
	for(uint i=0; i<s.length(); i++) {
		char c = s[i];
		if(c == '%') {
			*ptr++ = strtol(s.substr(i+1,2).c_str(), nullptr, 16);
			i += 2;
		} else
			*ptr++ = c;
	}
	*ptr = 0;
	return string(target);
}

void sleepms(uint ms) {
#ifdef WIN32
	Sleep(ms);
#else
	usleep(ms*1000);
#endif
}

void makedir(const std::string &name) {
	printf("Makedir '%s'\n", name.c_str());
#ifdef WIN32
	mkdir(name.c_str());
#else
	mkdir(name.c_str(), 07777);
#endif
}

void makedirs(const std::string &path) {
	int start = 0;
	while(true) {
		int pos = path.find("/", start);
		if(pos != string::npos) {
			makedir(path.substr(0, pos));
			start = pos+1;
		} else
			break;
	}
}

bool endsWith(const string &name, const string &ext) {
	size_t pos = name.rfind(ext);
	return (pos == name.length() - ext.length());
}

void makeLower(string &s) {
	for(uint i=0; i<s.length(); i++)
		s[i] = tolower(s[i]);
}



}