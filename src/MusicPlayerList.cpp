#include "MusicPlayerList.h"

#include <algorithm>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <unordered_map>

#include <coreutils/environment.h>
#include <musicplayer/chipplayer.h>

using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList(const utils::path& workDir) : mp(workDir.string())
{
    playerThread = std::thread([=] {
        while (!quitThread) {
            plMutex.lock();
            if (!funcs.empty()) {
                auto q = funcs;
                funcs.clear();
                plMutex.unlock();
                for (auto& f : q)
                    f();
            } else
                plMutex.unlock();
            update();
            sleepms(50);
        }
    });
}

void MusicPlayerList::addSong(const SongInfo& si, bool shuffle)
{

    // LOCK_GUARD(plMutex);
    onThisThread([=] {
        if (shuffle) {
            playList.insertAt(rand() % (playList.size() + 1), si);
        } else {
            LOGD("PUSH %s/%s (%s)", si.title, si.composer, si.path);
            playList.push_back(si);
        }
    });
    // return true;
}

void MusicPlayerList::clearSongs()
{
    // LOCK_GUARD(plMutex);
    onThisThread([=] { playList.clear(); });
}

void MusicPlayerList::nextSong()
{
    // LOCK_GUARD(plMutex);
    onThisThread([=] {
        if (playList.size() > 0) {
            // mp.stop();
            SET_STATE(Waiting);
        }
    });
}

void MusicPlayerList::playSong(const SongInfo& si)
{
    onThisThread([=] {
        dbInfo = currentInfo = si;
        SET_STATE(Playnow);
    });
}

void MusicPlayerList::seek(int song, int seconds)
{
    onThisThread([=] {
        if (!multiSongs.empty()) {
            LOGD("CHANGED MULTI");
            state = Playmulti;
            multiSongNo = song;
            return;
        }
        mp.seek(song, seconds);
        if (song >= 0) changedSong = true;
    });
}

SongInfo MusicPlayerList::getInfo(int index)
{
    LOCK_GUARD(plMutex);
    if (index == 0) return currentInfo;
    return playList.getSong(index - 1);
}

SongInfo MusicPlayerList::getDBInfo()
{
    LOCK_GUARD(plMutex);
    return dbInfo;
}

int MusicPlayerList::getLength()
{
    return playerLength;
}

int MusicPlayerList::getPosition()
{
    return playerPosition;
}

int MusicPlayerList::listSize()
{
    LOCK_GUARD(plMutex);
    return playList.size();
}

/// PRIVATE

void MusicPlayerList::updateInfo()
{
    auto si = mp.getPlayingInfo();
    if (si.format != "") currentInfo.format = si.format;
    if (multiSongs.empty()) {
        currentInfo.numtunes = si.numtunes;
        currentInfo.starttune = si.starttune;
    }
}

bool MusicPlayerList::handlePlaylist(const std::string& fileName)
{

    playList.clear();
    File f{fileName};

    auto lines = f.getLines();

    // Remove lines with comment character
    lines.erase(
        std::remove_if(lines.begin(), lines.end(),
                       [=](const std::string& l) { return l[0] == ';'; }),
        lines.end());
    for (const std::string& s : lines) {
        playList.push_back(SongInfo(s));
    }

    if (playList.size() == 0) return false;

    MusicDatabase::getInstance().lookup(playList.front());
    if (playList.front().path == "") {
        LOGD("Could not lookup '%s'", playList.front().path);
        errors.emplace_back("Bad song in playlist");
        SET_STATE(Error);
        return false;
    }
    SET_STATE(Waiting);
    return true;
}

bool MusicPlayerList::playFile(utils::path fileName)
{
    if (fileName == "") return false;
    auto ext = toLower(fileName.extension());
    if (ext == ".pls" || currentInfo.format == "PLS") {
        File f{fileName};

        auto lines = f.getLines();
        std::vector<std::string> result;
        for (auto& l : lines) {
            if (startsWith(l, "File1=")) result.push_back(l.substr(6));
        }
        currentInfo.path = result[0];
        currentInfo.format = "MP3";
        playCurrent();
        return false;

    } else if (ext == ".m3u" || currentInfo.format == "M3U") {
        File f{fileName};

        auto lines = f.getLines();

        // Remove lines with comment character
        lines.erase(std::remove_if(lines.begin(), lines.end(),
                                   [=](const std::string& l) {
                                       return l == "" || l[0] == '#';
                                   }),
                    lines.end());
        currentInfo.path = lines[0];
        currentInfo.format = "MP3";
        playCurrent();
        return false;

    } else if (ext == ".plist") {
        handlePlaylist(fileName.string());
        return true;
    } else if (ext == ".jb") {
        // Jason Brooke fix
        auto newName = fileName;
        newName.replace_extension(".jcb");
        if (!exists(newName)) utils::copy(fileName, newName);
        fileName = newName;
    }

    if (mp.playFile(fileName.string())) {
        if (currentInfo.starttune >= 0) mp.seek(currentInfo.starttune);
        changedSong = false;
        LOGD("CHANGED MULTI:%s", changedMulti ? "YES" : "NO");
        if (!changedMulti) {
            updateInfo();
            SET_STATE(Playstarted);
        } else
            SET_STATE(Playing);

        bitRate = 0;

        changedMulti = false;
        return true;
    } else {
        errors.emplace_back("Could not play song");
        SET_STATE(Error);
    }
    return false;
}

void MusicPlayerList::cancelStreaming()
{
    RemoteLoader::getInstance().cancel();
    mp.clearStreamFifo();
}
void MusicPlayerList::update()
{

    LOCK_GUARD(plMutex);

    mp.update();

    RemoteLoader::getInstance().update();

    if (state == Playnow) {
        SET_STATE(Started);
        // LOGD("##### PLAY NOW: %s (%s)", currentInfo.path, currentInfo.title);
        multiSongs.clear();
        playedNext = false;
        playCurrent();
    }

    if (state == Playmulti) {
        SET_STATE(Started);
        currentInfo.path = multiSongs[multiSongNo];
        changedMulti = true;
        playCurrent();
    }

    if (state == Playing || state == Playstarted) {

        auto pos = mp.getPosition();
        auto length = mp.getLength();

        if (cueSheet) {
            subtitle = cueSheet->getTitle(pos);
            subtitlePtr = subtitle.c_str();
        }

        if (!changedSong && playList.size() > 0) {
            if (!mp.playing()) {
                if (playList.size() == 0)
                    SET_STATE(Stopped);
                else
                    SET_STATE(Waiting);
            } else if ((length > 0 && pos > length) && pos > 7) {
                LOGD("STATE: Song length exceeded");
                mp.fadeOut(3.0);
                SET_STATE(Fading);
            } else if (detectSilence && mp.getSilence() > 44100 * 6 &&
                       pos > 7) {
                LOGD("STATE: Silence detected");
                mp.fadeOut(0.5);
                SET_STATE(Fading);
            }
        }
    }

    if (state == Fading) {
        if (mp.getFadeVolume() <= 0.01) {
            LOGD("STATE: Music ended");
            if (playList.size() == 0)
                SET_STATE(Stopped);
            else
                SET_STATE(Waiting);
        }
    }

    if (state == Loading) {
        if (files == 0) {
            cancelStreaming();
            playFile(loadedFile);
        }
    }

    if (state == Waiting && (playList.size() > 0)) {
        SET_STATE(Started);
        playedNext = true;
        dbInfo = currentInfo = playList.front();
        playList.pop_front();

        if (playList.size() > 0) {
            // Update info for next song from
            MusicDatabase::getInstance().lookup(playList.front());
        }

        // pos = 0;
        LOGD("Next song from queue : %s (%d)", currentInfo.path,
             currentInfo.starttune);
        multiSongs.clear();
        playCurrent();
    }

    // Cache values for outside access

    playerPosition = mp.getPosition();
    playerLength = mp.getLength(); // currentInfo.length;

    if (!multiSongs.empty())
        currentTune = multiSongNo;
    else
        currentTune = mp.getTune();

    playing = mp.playing();
    paused = mp.isPaused();
    auto br = mp.getMeta("bitrate");
    if (br != "") {
        bitRate = std::stol(br);
    }

    if (!cueSheet) {
        subtitle = mp.getMeta("sub_title");
        subtitlePtr = subtitle.c_str();
    }
}

void MusicPlayerList::playCurrent()
{

    SET_STATE(Loading);

    songFiles.clear();
    // screenshot = "";

    LOGD("PLAY PATH:%s", currentInfo.path);
    std::string prefix, path;
    auto parts = split(currentInfo.path, "::", 2);
    if (parts.size() == 2) {
        prefix = parts[0];
        path = parts[1];
    } else
        path = currentInfo.path;

    if (prefix == "index") {
        int index = stol(path);
        dbInfo = currentInfo = MusicDatabase::getInstance().getSongInfo(index);
        auto parts = split(currentInfo.path, "::", 2);
        if (parts.size() == 2) {
            prefix = parts[0];
            path = parts[1];
        } else
            path = currentInfo.path;
    }

    if (prefix == "product") {
        auto id = stol(path);
        playList.psongs.clear();
        for (const auto& song :
             MusicDatabase::getInstance().getProductSongs(id)) {
            playList.psongs.push_back(song);
        }
        if (playList.psongs.empty()) {
            LOGD("No songs in product");
            errors.emplace_back("No songs in product");
            SET_STATE(Error);
            return;
        }

        // Check that the first song is working
        MusicDatabase::getInstance().lookup(playList.psongs.front());
        if (playList.psongs.front().path == "") {
            LOGD("Could not lookup '%s'", playList.psongs.front().path);
            errors.emplace_back("Bad song in product");
            SET_STATE(Error);
            return;
        }
        SET_STATE(Waiting);
        return;
    } else {
        if (currentInfo.metadata[SongInfo::SCREENSHOT] == "") {
            auto s =
                MusicDatabase::getInstance().getSongScreenshots(currentInfo);
            currentInfo.metadata[SongInfo::SCREENSHOT] = s;
        }
    }

    if (prefix == "playlist") {
        if (!handlePlaylist(path)) SET_STATE(Error);
        return;
    }

    if (startsWith(path, "MULTI:")) {
        multiSongs = split(path.substr(6), "\t");
        if (prefix != "") {
            for (std::string& m : multiSongs) {
                m = prefix + "::" + m;
            }
        }
        multiSongNo = 0;
        currentInfo.path = multiSongs[0];
        currentInfo.numtunes = multiSongs.size();
        playCurrent();
        return;
    }

    auto ext = path_extension(path);
    makeLower(ext);

    detectSilence = true;
    if (ext == "mp3") detectSilence = false;

    cueSheet = nullptr;
    subtitle = "";
    subtitlePtr = subtitle.c_str();

    playerPosition = 0;
    playerLength = 0;
    bitRate = 0;
    currentTune = 0;

    cancelStreaming();

    if (utils::exists(currentInfo.path)) {
        LOGD("PLAYING LOCAL FILE %s", currentInfo.path);
        songFiles = {File(currentInfo.path)};
        loadedFile = currentInfo.path;
        files = 0;
        return;
    }

    loadedFile = "";
    files = 0;
    RemoteLoader& loader = RemoteLoader::getInstance();

    std::string cueName = "";
    if (prefix == "bitjam")
        cueName =
            currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) +
            ".cue";
    else if (prefix == "demovibes")
        cueName = toLower(
            currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) +
            ".cue");

    if (cueName != "") {
        loader.load(cueName, [=](File cuefile) {
            if (cuefile) cueSheet = std::make_shared<CueSheet>(cuefile);
        });
    }

    if (startsWith(currentInfo.path, "pouet::")) {
        loadedFile = currentInfo.path.substr(7);
        files = 0;
        return;
    }

    if (currentInfo.format != "M3U" &&
        (ext == "mp3" || toLower(currentInfo.format) == "mp3")) {

        if (mp.streamFile("dummy.mp3")) {
            SET_STATE(Playstarted);
            LOGD("Stream start");
            std::string name = currentInfo.path;
            loader.stream(currentInfo.path,
                          [=](int what, const uint8_t* ptr, int n) -> bool {
                              if (what == RemoteLoader::PARAMETER) {
                                  mp.setParameter((char*)ptr, n);
                              } else {
                                  // LOGD("Writing to %s", name);
                                  mp.putStream(ptr, n);
                              }
                              return true;
                          });
        }
        return;
    }

    // LOGD("LOADING:%s", currentInfo.path);
    files++;
    loader.load(currentInfo.path, [=](File f0) {
        if (!f0) {
            errors.emplace_back("Could not load file");
            SET_STATE(Error);
            files--;
            return;
        }
        songFiles.push_back(f0);
        loadedFile = f0.getName();
        //auto ext = toLower(path_extension(loadedFile));
        LOGD("Loaded file '%s'", loadedFile);
        auto parentDir = File(path_directory(loadedFile));
        auto fileList = mp.getSecondaryFiles(f0);
        for (const auto& s : fileList) {
            File target = parentDir / s;
            if (!target.exists()) {
                files++;
                RemoteLoader& loader = RemoteLoader::getInstance();
                auto url = path_directory(currentInfo.path) + "/" + s;
                loader.load(url, [=](File f) {
                    if (!f) {
                        errors.emplace_back("Could not load file");
                        SET_STATE(Error);
                    } else {
                        songFiles.push_back(f);
                    }
                    files--;
                });
            } else
                songFiles.push_back(target);
        }

        files--;
    });
}

} // namespace chipmachine
