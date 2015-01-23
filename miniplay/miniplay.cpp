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
#include <mutex>

using namespace chipmachine;
using namespace std;
using namespace utils;

int main(int argc, char* argv[]) {

	logging::setLevel(logging::WARNING);

	if(argc < 2) {
		printf("%s [musicfile]\n", argv[0]);
		return 0;
	}

	vector<shared_ptr<ChipPlugin>> plugins;
	ChipPlugin::createPlugins("data", plugins);

	File file { argv[1] };
	ChipPlayer *player = nullptr;

	string name = file.getName();
	makeLower(name);
	for(auto plugin : plugins) {
		if(plugin->canHandle(name)) {
			printf("Playing with %s\n", plugin->name().c_str());
			player = plugin->fromFile(file.getName());
			break;
		}
	}

	if(player) {

		AudioFifo<int16_t> fifo(32768);

		fifo.setVolume(0.1);

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