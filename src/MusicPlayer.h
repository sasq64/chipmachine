#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "SongInfo.h"

#include <coreutils/fifo.h>

#include <fft/spectrum.h>

#include <atomic>
#include <memory>
#include <vector>
#include <coreutils/thread.h>

//#define LOCK_GUARD(x) if(x.try_lock()) x.unlock(); else LOGE("WAITING FOR LOCK"); \
//std::lock_guard<std::mutex> guard(x)
#define LOCK_GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace chipmachine {

class ChipPlugin;
class ChipPlayer;

template <typename T> class SafePointer {
public:
	struct LockHolder {
		LockHolder(T *ptr, std::unique_lock<std::mutex> &&g) : ptr(ptr), guard(std::move(g)) {}

		LockHolder(LockHolder &&l) : guard(std::move(l.guard)), ptr(std::move(l.ptr)) {}

		T *operator->() const { return ptr; }

		T *get() const { return ptr; }

		explicit operator bool() { return ptr != nullptr; }

		void unlock() { guard.unlock(); }

		std::unique_lock<std::mutex> guard;
		T *ptr;
	};

	LockHolder operator->() const {
		std::unique_lock<std::mutex> guard(*m);
		return LockHolder(ptr.get(), std::move(guard));
	}

	LockHolder aquire() const {
		std::unique_lock<std::mutex> guard(*m);
		return LockHolder(ptr.get(), std::move(guard));
	}

	SafePointer() : m(std::make_shared<std::mutex>()) {}
	SafePointer(std::shared_ptr<T> ptr) : ptr(ptr), m(std::make_shared<std::mutex>()) {}
	SafePointer(T *ptr) : ptr(std::shared_ptr<T>(ptr)), m(std::make_shared<std::mutex>()) {}

	template <typename P> SafePointer<T> operator=(P t) {
		std::unique_lock<std::mutex> guard(*m);
		ptr = t;
		return *this;
	}

	SafePointer<T> operator=(SafePointer<T> p) {
		auto m2 = m;
		std::unique_lock<std::mutex> guard(*m);
		// TODO: Lock other mutex?
		ptr = p.ptr;
		m = p.m;
		return *this;
	}

	SafePointer<T> operator=(std::shared_ptr<T> p) {
		std::unique_lock<std::mutex> guard(*m);
		ptr = p;
		return *this;
	}

	bool operator==(T *t) const {
		std::unique_lock<std::mutex> guard(*m);
		return ptr.get() == t;
	}

	bool operator!=(T *t) const {
		std::unique_lock<std::mutex> guard(*m);
		return ptr.get() != t;
	}

	explicit operator bool() const {
		std::unique_lock<std::mutex> guard(*m);
		return ptr.get() != nullptr;
	}

private:
	std::shared_ptr<T> ptr;
	std::shared_ptr<std::mutex> m;
};

template <typename T> SafePointer<T> make_safepointer(std::shared_ptr<T> ptr) {
	return SafePointer<T>(ptr);
}

class Streamer {
public:
	Streamer(std::shared_ptr<std::mutex> m, std::shared_ptr<ChipPlayer> pl)
	    : playerMutex(m), player(pl) {}
	void put(const uint8_t *ptr, int size);

private:
	std::shared_ptr<std::mutex> playerMutex;
	std::shared_ptr<ChipPlayer> player;
};

// MUST BE THREAD SAFE
class MusicPlayer {
public:
	MusicPlayer(const std::string &workDir);
	~MusicPlayer();
	bool playFile(const std::string &fileName);
	SafePointer<ChipPlayer> streamFile(const std::string &fileName);
	bool playing() { return !playEnded && player != nullptr; }
	void stop() { player = nullptr; }
	uint32_t getPosition() { return pos / 44100; };
	uint32_t getLength() { return length; }

	// void addStreamData(uint8_t *ptr, int size);

	// Asks the plugin if the given file requires secondary files.
	// Can be called several times, normally first with non-existing
	// file, and later with the loaded file
	std::vector<std::string> getSecondaryFiles(const std::string &name);

	void pause(bool dopause = true);

	bool isPaused() { return paused; }

	void seek(int song, int seconds = -1);

	int getTune() { return currentTune; }

	SongInfo getPlayingInfo() {
		LOCK_GUARD(infoMutex);
		return playingInfo;
	}

	uint16_t *getSpectrum();

	std::string getMeta(const std::string &what);

	int spectrumSize() {
		LOCK_GUARD(fftMutex);
		return fft.eq_slots;
	}

	// Returns silence (from now) in seconds
	int getSilence();

	void setVolume(float v);
	float getVolume() const;

	// Fadeout music
	void fadeOut(float secs);
	float getFadeVolume() { return fifo.getVolume(); }

	void update();

private:
	std::shared_ptr<ChipPlayer> fromFile(const std::string &fileName);
	std::shared_ptr<ChipPlayer> fromStream(const std::string &fileName);
	void updatePlayingInfo();

	AudioFifo<int16_t> fifo;
	SongInfo playingInfo;
	// Fifo fifo;
	SpectrumAnalyzer fft;
	std::vector<std::shared_ptr<ChipPlugin>> plugins;
	bool paused = false;
	std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;

	// Should be held when accessing FFT data
	std::mutex fftMutex;
	std::mutex infoMutex;
	SafePointer<ChipPlayer> player;
	std::string message;
	std::string sub_title;
    int pos = 0;
    int length = 0;
    int fadeLength = 0;
    int fadeOutPos = 0;
    int silentFrames = 0;
    int currentTune = 0;
    float volume = 1.0f;
	// bool changedSong = false;
	std::atomic<bool> dontPlay;
	std::atomic<bool> playEnded;
};
}

#endif // MUSIC_PLAYER_H
