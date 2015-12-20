
#include "ChipInterface.h"

int main(int argc, char **argv) {
	using namespace chipmachine;
	using std::string;
	using utils::File;

	string path = File::makePath({
#ifdef __APPLE__
	    (File::getExeDir() / ".." / "Resources").resolve(),
#else
	    File::getExeDir(),
#endif
	    (File::getExeDir() / ".." / "..").resolve(), File::getAppDir()});
	string workDir = File::findFile(path, "data").getDirectory();

	LOGD("PATH:%s", workDir);

	ChipInterface ci(workDir);
	ci.init();

	ci.search("stardust");

	for(const auto &s : ci.getResult(0, ci.numHits())) {
		utils::print_fmt("%s\n", s);
	}

	auto si = ci.getSongInfo(24);
	ci.play(si);

	while(true)
		utils::sleepms(100);

	return 0;
};