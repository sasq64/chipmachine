#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>

struct SongInfo {
	SongInfo(const std::string &path = "", const std::string &title = "", const std::string &composer = "", const std::string &format = "") :
		path(path), title(title), composer(composer), format(format) {}
	SongInfo(const std::string &path, const std::string &title, const std::string &subtitle, const std::string &composer, const std::string &format) :
		path(path), composer(composer), format(format) {
			if(title != "") {
				if(subtitle != "")
					this->title = title + "(" + subtitle + ")" ;
				else
					this->title = title;
			} else if(subtitle != "")
				this->title = subtitle;
		}
	std::string path;
	std::string title;
	std::string composer;
	std::string format;
};


#endif // SONGINFO_H
