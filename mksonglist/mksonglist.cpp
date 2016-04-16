#include "../src/SongFileIdentifier.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>

void makeList(const std::string &local_dir, const std::string &list_file) {

	using namespace std;
	using namespace utils;

    File listFile{ list_file };

    for(auto &rf : File(local_dir).listRecursive()) {
        auto name = rf.getName();
        SongInfo songInfo(name);
        name = name.substr(local_dir.length());
        if(name[0] == '/')
            name = name.substr(1);
        songInfo.metadata = name;
        if(identify_song(songInfo)) {
            listFile.writeln(join("\t", songInfo.title, songInfo.game, songInfo.composer,
                                  songInfo.format, name));
        }
    }
	listFile.close();
}

int main(int argc, char **argv) {
	if(argc < 3)
		return 0;
	makeList(argv[1], argv[2]);
	return 0;
}
