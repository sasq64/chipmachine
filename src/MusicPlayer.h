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
    [[nodiscard]] bool playing() const
    {
        return !play_ended && player != nullptr;
    }
    void stop() { player = nullptr; }
    [[nodiscard]] uint32_t getPosition() const { return play_pos / 44100; };
    [[nodiscard]] uint32_t getLength() const { return length; }

    void putStream(const uint8_t* ptr, int size);
    void clearStreamFifo() { stream_fifo->clear(); }

    void setParameter(const std::string& what, int v);

    // Asks the plugin if the given file requires secondary files.
    // Can be called several times, normally first with non-existing
    // file, and later with the loaded file
    std::vector<std::string> getSecondaryFiles(const std::string& name);

    void pause(bool dopause = true);

    [[nodiscard]] bool isPaused() const { return paused; }

    void seek(int song, int seconds = -1);

    [[nodiscard]] int getTune() const { return currentTune; }

    [[nodiscard]] SongInfo getPlayingInfo() const { return playing_info; }

    std::string getMeta(const std::string& what);

    // Returns silence (from now) in seconds
    [[nodiscard]] int getSilence() const;

    void setVolume(float v);
    [[nodiscard]] float getVolume() const;

    // Fadeout music
    void fadeOut(float secs);
    [[nodiscard]] float getFadeVolume() const { return fifo.getVolume(); }

    void update();

    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        audio_callback = cb;
    }

private:
    std::shared_ptr<musix::ChipPlayer> fromFile(const std::string& fileName);
    void updatePlayingInfo();

    utils::AudioFifo<int16_t> fifo;
    SongInfo playing_info;
    // Fifo fifo;
    std::function<void(int16_t*, int)> audio_callback;

    std::atomic<bool> paused{ false };

    std::shared_ptr<musix::ChipPlayer> player;
    std::string message;
    std::string sub_title;
    std::atomic<int> play_pos{ 0 };
    std::atomic<int> length{ 0 };
    int fade_length = 0;
    int fadeout_pos = 0;
    int silent_frames = 0;
    int currentTune = 0;
    std::atomic<float> volume = 1.0F;

    // Feed silence to audio player
    std::atomic<bool> dont_play{ false };
    std::atomic<bool> play_ended{ false };
    bool check_silence = true;

    std::shared_ptr<utils::Fifo<uint8_t>> stream_fifo;

    AudioPlayer& audio_player;
};
} // namespace chipmachine
