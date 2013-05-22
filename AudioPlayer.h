#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

class AudioPlayer {
public:
	virtual void writeAudio(short *samples, int sampleCount) = 0;
};

#endif // AUDIOPLAYER_H