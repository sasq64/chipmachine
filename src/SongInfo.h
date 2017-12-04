#ifndef SONGINFO_H
#define SONGINFO_H

#include <coreutils/log.h>
#include <string>
#include <unordered_map>

struct SongInfo {
	SongInfo(const std::string &path = "", const std::string &game = "",
	         const std::string &title = "", const std::string &composer = "",
	         const std::string &format = "", const std::string &info = "")
	    : path(path), game(game), title(title), composer(composer), format(format), metadata { info, "" } {
		auto pos = path.find_last_of(';');
		if(pos != std::string::npos) {
			auto s = path.substr(pos+1);
			if(s.size() < 3) {
				starttune = stol(s);
				this->path = path.substr(0, pos);
			}
		}
	}
	
	enum { INFO, SCREENSHOT };

	bool operator==(const SongInfo &other) {
		return path == other.path && starttune == other.starttune; 
	}

	std::string path;
	std::string game;
	std::string title;
	std::string composer;
    std::string format;
	std::vector<std::string> metadata;
	
	int numtunes = 0;
	int starttune = -1;
};

#endif // SONGINFO_H
