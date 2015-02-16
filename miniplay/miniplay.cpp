#include <coreutils/fifo.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>

#include <audioplayer/audioplayer.h>
#include <musicplayer/plugins/plugins.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include <thread>

using namespace chipmachine;
using namespace std;
using namespace utils;

int main(int argc, char* argv[]) {

	logging::setLevel(logging::WARNING);

	if(argc < 2) {
		printf("%s [musicfile]\n", argv[0]);
		return 0;
	}

	File file { argv[1] };
	shared_ptr<ChipPlayer> player;
	string name = toLower(file.getName());

	auto plugins = ChipPlugin::createPlugins(File::getExeDir());

	for(auto plugin : plugins) {
		if(plugin->canHandle(name)) {
			printf("Playing with %s\n", plugin->name().c_str());
			player = shared_ptr<ChipPlayer>(plugin->fromFile(file.getName()));
			break;
		}
	}

	if(player) {

		AudioFifo<int16_t> fifo(32768);
		fifo.setVolume(0.5);

		std::thread playerThread([&]() mutable {
			int chunkSize = 8192;
			int16_t temp[chunkSize];
			while(true) {
				player->getSamples(&temp[0], chunkSize);
				fifo.put(temp, chunkSize);
			}
		});

		AudioPlayer ap ([&](int16_t *samples, int size) {
			fifo.get(samples, size);
		});

		playerThread.join();

	} else
		printf("'%s' FAILED\n", argv[1]);
	return 0;
}