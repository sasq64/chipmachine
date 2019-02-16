#include "MusicPlayer.h"
#include "GZPlugin.h"
#include "modutils.h"

#include <archive/archive.h>
#include <audioplayer/audioplayer.h>
#include <coreutils/format.h>
#include <coreutils/utils.h>
#include <musicplayer/PSFFile.h>
#include <musicplayer/plugins/plugins.h>

#include <algorithm>
#include <set>

namespace chipmachine {

MusicPlayer::MusicPlayer(AudioPlayer& ap)
    : fifo(32786 * 4),
      streamFifo(std::make_shared<utils::Fifo<uint8_t>>(32768 * 8)),
      audioPlayer(ap)
{
    audioPlayer.set_volume(80);
    volume = 0.8;

    musix::ChipPlugin::addPlugin(
        std::make_shared<GZPlugin>(musix::ChipPlugin::getPlugins()));

    audioPlayer.play([=](int16_t* ptr, int size) mutable {
        if (dontPlay) {
            memset(ptr, 0, size * 2);
            return;
        }

        if (fifo.filled() >= size) {
            fifo.get(ptr, size);
            pos += size / 2;
            if (audioCb) audioCb(ptr, size);
        } else
            memset(ptr, 0, size * 2);
    });
}

// MusicPlayer::MusicPlayer(MusicPlayer const& other) = default;

// Make sure the fifo is filled
void MusicPlayer::update()
{
    static std::vector<int16_t> tempBuf(fifo.size());

    if (!paused && player) {

        sub_title = player->getMeta("sub_title");
        length = player->getMetaInt("length");
        message = player->getMeta("message");
        silentFrames = checkSilence ? fifo.getSilence() : 0;

        while (true) {

            int f = fifo.left();

            if (f < 4096) break;

            int rc = player->getSamples(&tempBuf[0], f - 1024);

            if (rc == 0) break;

            if (rc < 0) {
                playEnded = true;
                break;
            }

            if (fadeOutPos != 0) {
                fifo.setVolume(std::min(0, fadeOutPos - pos) / (float)fadeLength);
            }

            fifo.put(&tempBuf[0], rc);
            if (fifo.filled() >= fifo.size() / 2) {
                break;
            }
        }
    }
}

MusicPlayer::~MusicPlayer()
{
    streamFifo->quit();
}

void MusicPlayer::seek(int song, int seconds)
{
    if (!player) return;
    if (player->seekTo(song, seconds)) {
        if (seconds < 0)
            pos = 0;
        else
            pos = seconds * 44100;
        fifo.clear();
        // length = player->getMetaInt("length");
        updatePlayingInfo();
        currentTune = song;
    }
}

int MusicPlayer::getSilence() const
{
    return silentFrames;
}

// fadeOutPos music
void MusicPlayer::fadeOut(float secs)
{
    fadeLength = secs * 44100;
    fadeOutPos = pos + fadeLength;
}

void MusicPlayer::putStream(const uint8_t* ptr, int size)
{
    // LOGD("Writing %d bytes to stream", size);
    streamFifo->put(ptr, size);
}

void MusicPlayer::setParameter(const std::string& what, int v)
{
    if (player) player->setParameter(what, v);
}

bool MusicPlayer::streamFile(const std::string& fileName)
{
    dontPlay = true;
    silentFrames = 0;

    playingInfo = SongInfo();
    std::string name = fileName;
    player = nullptr;

    utils::makeLower(name);
    checkSilence = true;
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            LOGD("Playing with %s\n", plugin->name());
            auto newPlayer = std::shared_ptr<musix::ChipPlayer>(
                plugin->fromStream(streamFifo));
            if (newPlayer) player = newPlayer;
            checkSilence = plugin->checkSilence();
            break;
        }
    }

    dontPlay = false;
    playEnded = false;

    if (player) {

        clearStreamFifo();
        fifo.clear();
        fadeOutPos = 0;
        pause(false);
        pos = 0;
        // updatePlayingInfo();
        message = "";
        length = 0;
        sub_title = "";
        currentTune = playingInfo.starttune;
        return true;
    }
    return false;
}

bool MusicPlayer::playFile(const std::string& fileName)
{

    dontPlay = true;
    silentFrames = 0;
    playingInfo = SongInfo();
    std::string name = fileName;

    if (utils::endsWith(name, ".rar")) {
        try {
            auto* a = utils::Archive::open(name, "_files");
            for (const auto& s : *a) {
                a->extract(s);
                name = "_files/" + s;
                LOGD("Extracted %s", name);
                break;
            }
        } catch (utils::archive_exception& ae) {
            player = nullptr;
            return false;
        }
    }

    LOGD("Try");
    player = nullptr;
    player = fromFile(name);
    LOGD("Done");

    dontPlay = false;
    playEnded = false;

    if (player) {

        fifo.clear();
        fadeOutPos = 0;
        pause(false);
        pos = 0;
        updatePlayingInfo();
        currentTune = playingInfo.starttune;
        return true;
    }
    return false;
}

void MusicPlayer::updatePlayingInfo()
{
    SongInfo si;
    auto game = player->getMeta("game");
    si.title = player->getMeta("title");
    if (game != "") {
        if (si.title != "") {
            si.title = utils::format("%s (%s)", game, si.title);
        } else
            si.title = game;
    }

    si.composer = player->getMeta("composer");
    si.format = player->getMeta("format");
    si.numtunes = player->getMetaInt("songs");
    si.starttune = player->getMetaInt("startSong");
    if (si.starttune == -1) si.starttune = 0;

    length = player->getMetaInt("length");
    message = player->getMeta("message");
    sub_title = player->getMeta("sub_title");
    playingInfo = si;
}

void MusicPlayer::pause(bool dopause)
{
    if (dopause)
        audioPlayer.pause();
    else
        audioPlayer.resume();
    paused = dopause;
}

std::string MusicPlayer::getMeta(const std::string& what)
{
    if (what == "message") {
        return message;
    } else if (what == "sub_title") {
        return sub_title;
    }
    if (player) return player->getMeta(what);
    return "";
}

void MusicPlayer::setVolume(float v)
{
    volume = utils::clamp(v);
    audioPlayer.set_volume(volume * 100);
}

float MusicPlayer::getVolume() const
{
    return volume;
}

std::vector<std::string> MusicPlayer::getSecondaryFiles(const std::string& name)
{

    utils::File file{ name };
    if (file.exists()) {
        PSFFile f{ name };
        if (f.valid()) {
            LOGD("IS PSF");
            const std::string tagNames[] = { "_lib", "_lib2", "_lib3",
                                             "_lib4" };
            std::vector<std::string> libFiles;
            for (auto const& tag : tagNames) {
                auto lib = f.tags()[tag];
                if (lib != "") {
                    utils::makeLower(lib);
                    libFiles.push_back(lib);
                }
            }
            return libFiles;
        }

        for (auto& plugin : musix::ChipPlugin::getPlugins()) {
            if (plugin->canHandle(name)) {
                return plugin->getSecondaryFiles(file);
            }
        }
    }
    return std::vector<std::string>();
}

// PRIVATE

std::shared_ptr<musix::ChipPlayer>
MusicPlayer::fromFile(const std::string& fileName)
{
    std::shared_ptr<musix::ChipPlayer> player;
    auto name = fileName;
    utils::makeLower(name);
    checkSilence = true;
    LOGD("Finding plugin for '%s' (%s)", fileName, name);
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            LOGD("Playing with %s\n", plugin->name());
            player =
                std::shared_ptr<musix::ChipPlayer>(plugin->fromFile(fileName));
            if (!player) continue;
            checkSilence = plugin->checkSilence();
            break;
        }
    }
    return player;
}

} // namespace chipmachine
