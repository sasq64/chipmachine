#ifndef CHIP_PLUGIN_H
#define CHIP_PLUGIN_H

#include <string>

namespace chipmachine {

class ChipPlayer;

class ChipPlugin {
public:
	virtual ~ChipPlugin() {};

	virtual bool canHandle(const std::string &name) = 0;
	virtual ChipPlayer *fromFile(const std::string &fileName) = 0;
	virtual ChipPlayer *fromData(uint8_t *data, int size) {
		FILE *fp = fopen("tmpfile", "wb");
		fwrite(data, size, 1, fp);
		fclose(fp);
		return fromFile("tmpfile");
	}
};

}
#endif // CHIP_PLUGIN_H