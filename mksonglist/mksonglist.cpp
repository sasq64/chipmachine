#include "SongFileIdentifier.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/format.h>
#include <coreutils/log.h>

using namespace std;
using namespace utils;

void makeList(const string &local_dir, const string &list_file) {

	//makedir(File::getCacheDir() / ".rsntemp");
	File listFile{list_file};

	function<void(File &)> checkDir;
	checkDir = [&](File &root) {
		for(auto &rf : root.listFiles()) {
			if(rf.isDir()) {
				checkDir(rf);
			} else {
				auto name = rf.getName();
				SongInfo songInfo(name);
				if(identify_song(songInfo)) {

					auto pos = name.find(local_dir);
					if(pos != string::npos) {
						name = name.substr(pos + local_dir.length());
						if(name[0] == '/')
							name = name.substr(1);
					}
					listFile.writeln(format("%s\t%s\t%s\t%s\t%s", songInfo.title,
											songInfo.game, songInfo.composer,
											songInfo.format, name));
				}
			}
		}
	};

	File root{local_dir};

	LOGD("Checking local dir '%s'", root.getName());

	checkDir(root);

	listFile.close();
}


int main(int argc, char **argv) {
	makeList(argv[1], argv[2]);
	return 0;
}
