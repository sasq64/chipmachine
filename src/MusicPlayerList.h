#pragma once

#include "MusicPlayer.h"
#include "SongInfo.h"
#ifdef USE_REMOTELISTS
#    include "RemoteLists.h"
#endif
#include "CueSheet.h"
#include "MusicDatabase.h"
#include "RemoteLoader.h"

#include <coreutils/thread.h>
#include <cstdint>
#include <deque>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

struct log_guard
{
    log_guard(std::mutex& m, const char* f, int l) : m(m)
    {
        if (!m.try_lock()) {
            logging::log2(logging::xbasename(f), l, logging::Debug,
                          "Waiting for lock");
            m.lock();
        }
    }
    ~log_guard() { m.unlock(); }
    std::mutex& m;
};

#define LOCK_GUARD(x) std::lock_guard guard(x)
//#define LOCK_GUARD(x) log_guard guard(x, __FILE__, __LINE__)

#define SET_STATE(x) (LOGD("STATE: " #x), state = x)

namespace chipmachine {

class ChipMachine;

class MusicPlayerList
{
public:
    enum State
    {
        Stopped,
        Error,
        Waiting,
        Loading,
        Started,
        Playstarted,
        Playing,
        Fading,
        Playnow,
        Playmulti
    };

    MusicPlayerList(const fs::path& workDir);

    ~MusicPlayerList()
    {
        quitThread = true;
        playerThread.join();
    }

    void addSong(const SongInfo& si, bool shuffle = false);
    void playSong(const SongInfo& si);
    void clearSongs();
    void nextSong();

    SongInfo getInfo(int index = 0);
    SongInfo getDBInfo();
    int getLength();
    int getPosition();
    int listSize();

    bool isPlaying() { return playing; }

    int getTune() { return currentTune; }

    void pause(bool dopause = true)
    {
        LOCK_GUARD(plMutex);
        mp.pause(dopause);
    }

    bool isPaused() { return paused; }

    void seek(int song, int seconds = -1);

    int getBitrate() { return bitRate; }

    std::string getMeta(const std::string& what)
    {
        if (what == "sub_title") {
            std::string sub = std::string(subtitlePtr);
            return sub;
        }
        LOCK_GUARD(plMutex);
        LOGD("META %s", what);
        return mp.getMeta(what);
    }

    State getState()
    {
        // LOCK_GUARD(plMutex);
        State rc = state;
        if (rc == Playstarted) {
            SET_STATE(Playing);
        }
        return rc;
    }

    bool hasError() { return errors.size() > 0; }

    std::string getError()
    {
        LOCK_GUARD(plMutex);
        auto e = errors.front();
        errors.pop_front();
        return e;
    }

    void setReportSongs(bool on) { reportSongs = on; }

    void setVolume(float volume)
    {
        onThisThread([=] { mp.setVolume(volume); });
    }

    float getVolume()
    {
        LOCK_GUARD(plMutex);
        return mp.getVolume();
    }

    void stop()
    {
        onThisThread([=] {
            SET_STATE(Stopped);
            mp.stop();
        });
    }

    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        mp.setAudioCallback(cb);
    }

    bool wasFromQueue() const { return playedNext; }

    const std::vector<utils::File>& getSongFiles() const { return songFiles; }

    bool playlistUpdated() { return playList.wasUpdated(); }

private:
    void onThisThread(std::function<void()> f)
    {
        LOCK_GUARD(plMutex);
        funcs.push_back(f);
    }

    std::vector<std::function<void()>> funcs;

    void cancelStreaming();
    bool handlePlaylist(const std::string& fileName);
    void playCurrent();
    bool playFile(fs::path fileName);

    void update();
    void updateInfo();

    std::deque<std::string> errors;

    MusicPlayer mp;

    // Lock when accessing MusicPlayer
    std::mutex plMutex;

    struct PlayQueue
    {
        std::atomic<bool> updated;
        std::deque<SongInfo> songs;
        std::deque<SongInfo> psongs;
        std::string prodScreenshot;
        int size() { return songs.size() + psongs.size(); }
        void push_back(const SongInfo& s)
        {
            songs.push_back(s);
            updated = true;
        }
        // void push_font(const SongInfo &s) { songs.push_front(s); }
        void clear()
        {
            psongs.clear();
            songs.clear();
            updated = true;
        }
        void pop_front()
        {
            if (psongs.size() > 0)
                psongs.pop_front();
            else
                songs.pop_front();
            updated = true;
        }
        SongInfo& front()
        {
            if (psongs.size() > 0) return psongs.front();
            return songs.front();
        }
        SongInfo& getSong(int i)
        {
            if (i < psongs.size()) return psongs[i];
            return songs[i - psongs.size()];
        }
        void insertAt(int i, const SongInfo& s)
        {
            songs.insert(songs.begin() + i, s);
            updated = true;
        }
        bool wasUpdated()
        {
            bool rc = updated;
            updated = false;
            return rc;
        }
    };

    PlayQueue playList;

    std::atomic<bool> wasAllowed{true};
    std::atomic<bool> quitThread{false};

    std::atomic<int> currentTune{0};
    std::atomic<bool> playing{false};
    std::atomic<bool> paused{false};
    std::atomic<int> bitRate{0};
    std::atomic<int> playerPosition{0};
    std::atomic<int> playerLength{0};

    std::atomic<int> files{0};
    std::string loadedFile;

    std::atomic<State> state{Stopped};
    SongInfo currentInfo;
    SongInfo dbInfo;

    std::thread playerThread;

    bool changedSong = false;

    bool reportSongs = true;

    bool detectSilence = true;

    std::shared_ptr<CueSheet> cueSheet;
    std::string subtitle;
    std::atomic<const char*> subtitlePtr{nullptr};

    int multiSongNo = 0;
    std::vector<std::string> multiSongs;
    bool changedMulti = false;
    bool playedNext = false;

    std::vector<utils::File> songFiles;
};

} // namespace chipmachine

