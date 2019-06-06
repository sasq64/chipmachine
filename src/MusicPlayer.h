#pragma once

#include "SongInfo.h"

#include <coreutils/fifo.h>

#include <atomic>
#include <memory>
#include <vector>

namespace musix {
class ChipPlugin;
class ChipPlayer;
} // namespace musix

class AudioPlayer;

namespace chipmachine {

class MusicPlayer
{
public:
    explicit MusicPlayer(AudioPlayer& ap);
    MusicPlayer(MusicPlayer const& other) = delete;
    ~MusicPlayer();
    bool playFile(const std::string& fileName);
    bool streamFile(const std::string& fileName);
    [[nodiscard]] bool playing() const { return !playEnded && player != nullptr; }
    void stop() { player = nullptr; }
    [[nodiscard]] uint32_t getPosition() const { return pos / 44100; };
    [[nodiscard]] uint32_t getLength() const { return length; }

    void putStream(const uint8_t* ptr, int size);
    void clearStreamFifo()
    {
        LOGD("Clearing stream fifo");
        streamFifo->clear();
    }

    void setParameter(const std::string& what, int v);

    // Asks the plugin if the given file requires secondary files.
    // Can be called several times, normally first with non-existing
    // file, and later with the loaded file
    std::vector<std::string> getSecondaryFiles(const std::string& name);

    void pause(bool dopause = true);

    [[nodiscard]] bool isPaused() const { return paused; }

    void seek(int song, int seconds = -1);

    [[nodiscard]] int getTune() const { return currentTune; }

    [[nodiscard]] SongInfo getPlayingInfo() const { return playingInfo; }

    std::string getMeta(const std::string& what);

    // Returns silence (from now) in seconds
    [[nodiscard]] int getSilence() const;

    void setVolume(float v);
    [[nodiscard]] float getVolume() const;

    // Fadeout music
    void fadeOut(float secs);
    float getFadeVolume() { return fifo.getVolume(); }

    void update();

    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        audioCb = cb;
    }

private:
    std::shared_ptr<musix::ChipPlayer> fromFile(const std::string& fileName);
    // std::shared_ptr<musix::ChipPlayer> fromStream(const std::string
    // &fileName);
    void updatePlayingInfo();

    utils::AudioFifo<int16_t> fifo;
    SongInfo playingInfo;
    // Fifo fifo;
    std::function<void(int16_t*, int)> audioCb;

    std::atomic<bool> paused{ false };

    std::shared_ptr<musix::ChipPlayer> player;
    std::string message;
    std::string sub_title;
    std::atomic<int> pos{ 0 };
    std::atomic<int> length{ 0 };
    int fadeLength = 0;
    int fadeOutPos = 0;
    int silentFrames = 0;
    int currentTune = 0;
    std::atomic<float> volume = 1.0F;

    std::atomic<bool> dontPlay{ false };
    std::atomic<bool> playEnded{ false };
    bool checkSilence = true;

    std::shared_ptr<utils::Fifo<uint8_t>> streamFifo;

    AudioPlayer& audioPlayer;
};
} // namespace chipmachine
