#ifndef DS_FIFO_H
#define DS_FIFO_H

class Fifo {

public:
	Fifo(int size);
	~Fifo();
	void putBytes(char *src, int bytelen);
	void putShorts(short *src, int shortlen) {
		putBytes((char*)src, shortlen*2);
	}
	int getBytes(char *dest, int bytelen);
	int getShorts(short *dest, int shortlen) {
		return getBytes((char*)dest, shortlen*2) / 2;
	}

	void processBytes(char *src, int byteLen);

	int filled() { return bufPtr - buffer; }

	int getSilence() { return position - lastSoundPos; }

private:
	int size;
	int lastSoundPos;
	int position;
	char *buffer;
	char *bufPtr;

};

Fifo::Fifo(int size) {
	buffer = NULL;
	if(size > 0) {
		buffer = (char *)malloc(size);
	}
	bufPtr = buffer;
	position = 0;
}

Fifo::~Fifo() {
	if(buffer)
		free(buffer);
}



void Fifo::processBytes(char *src, int bytelen) {

	int soundPos = -1;
	short *samples = (short*)src;
	for(int i=0; i<bytelen/2; i++) {
		short s = samples[i];
		if(s > 256 || s < -256)
			soundPos = i;
	}

	if(soundPos >= 0)
		lastSoundPos = position + soundPos;

	position += (bytelen/2);

}

void Fifo::putBytes(char *src, int bytelen) {
	memcpy(bufPtr, src, bytelen);
	processBytes(src, bytelen);
	bufPtr += bytelen;
}

int Fifo::getBytes(char *dest, int bytelen) {

	int filled = bufPtr - buffer;
	if(bytelen > filled)
		bytelen = filled;

	memcpy(dest, buffer, bytelen);
	if(filled > bytelen)
		memmove(buffer, &buffer[bytelen], filled - bytelen);
	bufPtr = &buffer[filled - bytelen];

	return bytelen;
}

#endif
