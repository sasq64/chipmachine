#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>

struct SongInfo {
	SongInfo(const std::string &path = "", const std::string &game = "", const std::string &title = "", const std::string &composer = "", const std::string &format = "") :
		path(path), game(game), title(title), composer(composer), format(format) {}

	bool operator==(const SongInfo &other) { return path == other.path; }

	std::string path;
	std::string game;
	std::string title;
	std::string composer;
	std::string format;
	//uint32_t length = 0;
	uint32_t numtunes = 0;
	uint32_t starttune = 0;
};


#endif // SONGINFO_H
