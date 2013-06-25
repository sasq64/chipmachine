#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <stdint.h>

class AudioPlayer {
public:
	virtual void writeAudio(int16_t *samples, int sampleCount) = 0;
};

#endif // AUDIOPLAYER_H