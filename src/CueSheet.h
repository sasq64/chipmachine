#ifndef CUE_SHEET_H
#define CUE_SHEET_H

/*
 
 FORMER "BitJam"
 TITLE "BitJam Podcast #100"
 FILE "bitjam_100.mp3" MP3
   TRACK 01 AUDIO
       TITLE "BitJam jingle"
	       PERFORMER "CONS"
		       INDEX 01 00:00:00
*/
#include <coreutils/log.h>
#include <coreutils/file.h>
#include <string>

class CueSheet {
public:

	struct Track {
		std::string title;
		std::string performer;
		int index;
	};

	CueSheet(utils::File cf) {

		using namespace std;

		for(const string &line : cf.getLines()) {
			bool quotes = false;
			int start = 0;
			vector<string> parts;
			for(int i=0; i<line.length(); i++) {
				if(line[i] == '\"') {
					quotes = !quotes;
					if(!quotes)
						parts.push_back(line.substr(start, i-start));
					start = i+1;
				}
				if(quotes)
					continue;
				if(line[i] == ' ') {
					if(i>start) {
						parts.push_back(line.substr(start, i-start));
					}
					start = i+1;
				}

			}
			if(start < line.length())
				parts.push_back(line.substr(start));

			const auto &cmd = parts[0];
			LOGD("[%s]", parts);
			if(cmd == "TRACK")
				tracks.emplace_back();
			if(tracks.size() > 0) {
				auto &track = tracks.back();
				if (cmd == "TITLE")
					track.title = parts[1];
				else if (cmd == "PERFORMER")
					track.performer = parts[1];
				else if (cmd == "INDEX") {
					auto iparts = utils::split(parts[2], ":");
					track.index = stol(iparts[0]) * 60 + stol(iparts[1]);
				}
			}
		
		}
	}

	std::string getTitle(int pos) {
		const Track *lastt = nullptr;
		for(const auto &t : tracks) {
			if(pos < t.index)
				break;
			lastt = &t;
		}
		if(lastt)
			return lastt->title + " / " + lastt->performer;
		return "";
	}

	std::vector<Track> tracks;

};

#endif // CUE_SHEET_H
