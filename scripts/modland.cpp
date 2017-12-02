// g++ -I../apone/mods -std=c++11 ../apone/mods/coreutils/*.cpp modland.cpp
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/log.h>
#include <unordered_map>
#include <tuple>
#include <map>

int main(int argc, char **argv) {
	using namespace utils;
	using namespace std;

	unordered_map<string, int> songs;

	std::vector<string> toRemove;
	std::vector<std::pair<string, string>> toRename;

	string modlandPath = "/opt/Music/MODLAND";

	File mods{"data/allmods.txt"};
	long total = 0;
	int count = 0;
	for(const auto &l : mods.getLines()) {
		auto parts = split(l, "\t");
		int sz = stol(parts[0]);
		songs[parts[1]] = sz;
		total += sz;
		count++;
	}

	printf("%d files in %ld bytes\n", count, total);
	int pathLen = modlandPath.size();
	function<void(const File &)> checkDir;

	int counter = 0;
	int report = count / 10;

	checkDir = [&](const File &root) {
		for(auto &rf : root.listFiles()) {
			if(rf.isDir()) {
				checkDir(rf);
			} else {
				if((++counter % report) == 0)
					LOGD("Parsed %d/%d files", counter, count);
				auto name = rf.getName();
				const char *ptr = &name.c_str()[pathLen + 1];
				auto p = songs.find(ptr);
				// Is this file in allmods.txt ?
				if(p == songs.end()) { // NO
					// Should be deleted
					toRemove.push_back(name);
				} else {
					auto sz = File(name).getSize();
					if(sz != p->second) {
						// File differs, should be downloaded
						LOGD("### DIFFERS %s %d vs %s %d", name, sz, p->first, p->second);
					} else // Dont do anything
						songs.erase(p);
				}
			}
		}
	};
	checkDir(File(modlandPath));

	long download = 0;
	int dlcount = 0;

	File out {"modland.work"};
	out.writeln("# DOWNLOAD");
	for(const auto &e : songs) {
		auto x = modlandPath + "/" + e.first;
		if(File::exists(x)) {
			auto y = File::resolvePath(x);
			if(x != y) {
				LOGD("Need to rename %s = > %s", y, x);
				auto px = path_directory(x);
				auto py = path_directory(y);
				if(px != py) {
					toRename.emplace_back(py, px);
					// File(py).rename(px);
				} else {
					toRename.emplace_back(y, x);
					// File(y).rename(x);
				}
			}
		} else {
			out.write(e.first + "\n");
			download += e.second;
			dlcount++;
		}
	}

	out.writeln("# RENAME");
	for(const auto &d : toRename) {
		out.write(d.first + " to " + d.second + "\n");
	}
	out.writeln("# REMOVE");
	for(const auto &d : toRemove) {
		out.write(d + "\n");
	}

	LOGD("Need to download %d bytes in %d files", download, dlcount);

	return 0;
}
