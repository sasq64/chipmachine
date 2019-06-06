#pragma once

#include "MusicDatabase.h"
#include "MusicPlayerList.h"

#include <coreutils/utils.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

class RemoteLoader;

namespace chipmachine {

class ChipInterface
{
public:
    ChipInterface(const utils::path& wd, RemoteLoader& rl, MusicPlayerList& mpl,
                  MusicDatabase& mdb)
        : workDir(wd), remoteLoader(rl), player(mpl), mdb(mdb)
    {
        mdb.initFromLua(wd);
    }

    std::shared_ptr<IncrementalQuery> createQuery()
    {
        std::lock_guard<std::mutex> lg(m);
        return mdb.createQuery();
    }

    SongInfo getSongInfo(int i) { return mdb.getSongInfo(i); }

    int play(const SongInfo& song)
    {
        player.playSong(song);
        return 0;
    }

    void addSong(const SongInfo& song) { player.addSong(song); }
    void nextSong() { player.nextSong(); }
    void clearSongs();

    void setTune(int t) { player.seek(t); }

    bool playing() { return player.isPlaying(); }
    void pause(bool p) { return player.pause(p); }

    void update()
    {
        playerState = player.getState();
        if (playerState == MusicPlayerList::Playstarted) {
            info = player.getInfo();
            for (auto& cb : meta_callbacks)
                (*cb)(info);
        }
    }

    using MetaCallback = std::function<void(const SongInfo&)>;
    using MetaHolder = std::shared_ptr<std::function<void(std::nullptr_t)>>;

    MetaHolder onMeta(const MetaCallback& callback)
    {
        std::lock_guard<std::mutex> lg(m);
        meta_callbacks.push_back(std::make_shared<MetaCallback>(callback));
        auto mc = meta_callbacks.back();
        (*mc)(info);
        return MetaHolder(nullptr, [=](std::nullptr_t) {
            std::lock_guard<std::mutex> lg(m);
            meta_callbacks.erase(
                std::remove(meta_callbacks.begin(), meta_callbacks.end(), mc),
                meta_callbacks.end());
        });
    }

    [[nodiscard]] int seconds() const { return player.getPosition(); }

    RemoteLoader& getRemoteLoader() { return remoteLoader; }

private:
    RemoteLoader& remoteLoader;
    utils::path workDir;
    std::mutex m;
    MusicDatabase& mdb;
    SongInfo info;
    MusicPlayerList& player;
    MusicPlayerList::State playerState{ MusicPlayerList::State::Stopped };
    std::vector<std::shared_ptr<MetaCallback>> meta_callbacks;
    void setupRules();
    void updateKeys();
};

} // namespace chipmachine
