#include "MusicPlayer.h"
#include "GZPlugin.h"
#include "modutils.h"

#include <archive/archive.h>
#include <audioplayer/audioplayer.h>
#include <coreutils/format.h>
#include <coreutils/utils.h>
#include <musicplayer/plugins/plugins.h>
#include <psf/PSFFile.h>

#include <algorithm>
#include <set>

namespace chipmachine {

MusicPlayer::MusicPlayer(AudioPlayer& ap)
    : fifo(32786 * 4),
      stream_fifo(std::make_shared<utils::Fifo<uint8_t>>(32768 * 8)),
      audio_player(ap)
{
    audio_player.set_volume(80);
    volume = 0.8;

    musix::ChipPlugin::addPlugin(
        std::make_shared<GZPlugin>(musix::ChipPlugin::getPlugins()), true);

    audio_player.play([this](int16_t* ptr, int size) mutable {
        if (dont_play) {
            memset(ptr, 0, size * 2);
            return;
        }

        if (fifo.filled() >= size) {
            fifo.get(ptr, size);
            play_pos += size / 2;
            if (audio_callback) audio_callback(ptr, size);
        } else
            memset(ptr, 0, size * 2);
    });
}

// Make sure the fifo is filled
void MusicPlayer::update()
{
    static std::vector<int16_t> temp_buf(fifo.size());

    if (!paused && player) {

        sub_title = player->getMeta("sub_title");
        length = player->getMetaInt("length");
        message = player->getMeta("message");
        silent_frames = check_silence ? fifo.getSilence() : 0;

        while (true) {

            int space_left = fifo.left();

            if (space_left < 4096) break;

            int samples_generated =
                player->getSamples(&temp_buf[0], space_left - 1024);

            if (samples_generated <= 0) {
                play_ended = samples_generated < 0;
                break;
            }
            if (fadeout_pos != 0 && fadeout_pos >= play_pos) {
                fifo.setVolume((fadeout_pos - play_pos) / (float)fade_length);
            }

            fifo.put(&temp_buf[0], samples_generated);
            if (fifo.filled() >= fifo.size() / 2) {
                break;
            }
        }
    }
}

MusicPlayer::~MusicPlayer()
{
    stream_fifo->quit();
}

void MusicPlayer::seek(int song, int seconds)
{
    if (!player) return;
    if (player->seekTo(song, seconds)) {
        if (seconds < 0)
            play_pos = 0;
        else
            play_pos = seconds * 44100;
        fifo.clear();
        // length = player->getMetaInt("length");
        updatePlayingInfo();
        currentTune = song;
    }
}

int MusicPlayer::getSilence() const
{
    return silent_frames;
}

// fadeOutPos music
void MusicPlayer::fadeOut(float secs)
{
    fade_length = secs * 44100;
    fadeout_pos = play_pos + fade_length;
}

void MusicPlayer::putStream(const uint8_t* ptr, int size)
{
    // LOGD("Writing %d bytes to stream", size);
    stream_fifo->put(ptr, size);
}

void MusicPlayer::setParameter(const std::string& what, int v)
{
    if (player) player->setParameter(what, v);
}

bool MusicPlayer::streamFile(const std::string& fileName)
{
    dont_play = true;
    silent_frames = 0;

    playing_info = SongInfo();
    std::string name = fileName;
    player = nullptr;

    utils::makeLower(name);
    check_silence = true;
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            LOGD("Playing with %s\n", plugin->name());
            auto newPlayer = std::shared_ptr<musix::ChipPlayer>(
                plugin->fromStream(stream_fifo));
            if (newPlayer) player = newPlayer;
            check_silence = plugin->checkSilence();
            break;
        }
    }

    dont_play = false;
    play_ended = false;

    if (player) {

        clearStreamFifo();
        fifo.clear();
        fadeout_pos = 0;
        pause(false);
        play_pos = 0;
        message = "";
        length = 0;
        sub_title = "";
        currentTune = playing_info.starttune;
        return true;
    }
    return false;
}

bool MusicPlayer::playFile(const std::string& fileName)
{

    dont_play = true;
    silent_frames = 0;
    playing_info = SongInfo();
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

    player = nullptr;
    player = fromFile(name);

    dont_play = false;
    play_ended = false;

    if (player) {

        fifo.clear();
        fadeout_pos = 0;
        pause(false);
        play_pos = 0;
        updatePlayingInfo();
        currentTune = playing_info.starttune;
        return true;
    }
    return false;
}

void MusicPlayer::updatePlayingInfo()
{
    SongInfo info;
    auto game = player->getMeta("game");
    info.title = player->getMeta("title");
    if (!game.empty()) {
        if (!info.title.empty()) {
            info.title = utils::format("%s (%s)", game, info.title);
        } else
            info.title = game;
    }

    info.composer = player->getMeta("composer");
    info.format = player->getMeta("format");
    info.numtunes = player->getMetaInt("songs");
    info.starttune = player->getMetaInt("startSong");
    if (info.starttune == -1) info.starttune = 0;

    length = player->getMetaInt("length");
    message = player->getMeta("message");
    sub_title = player->getMeta("sub_title");
    playing_info = info;
}

void MusicPlayer::pause(bool do_pause)
{
    if (do_pause)
        audio_player.pause();
    else
        audio_player.resume();
    paused = do_pause;
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
    audio_player.set_volume(volume * 100);
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
            std::array tag_names = { "_lib", "_lib2", "_lib3", "_lib4" };
            std::vector<std::string> lib_files;
            for (auto const& tag : tag_names) {
                auto lib = f.tags()[tag];
                if (lib != "") {
                    utils::makeLower(lib);
                    lib_files.push_back(lib);
                }
            }
            return lib_files;
        }

        for (auto& plugin : musix::ChipPlugin::getPlugins()) {
            if (plugin->canHandle(name)) {
                return plugin->getSecondaryFiles(file);
            }
        }
    }
    return {};
}

// PRIVATE

std::shared_ptr<musix::ChipPlayer>
MusicPlayer::fromFile(const std::string& file_name)
{
    auto name = file_name;
    utils::makeLower(name);
    check_silence = true;
    LOGD("Finding plugin for '%s' (%s)", file_name, name);
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            LOGD("Playing with %s\n", plugin->name());
            auto player =
                std::shared_ptr<musix::ChipPlayer>(plugin->fromFile(file_name));
            if (!player) continue;
            check_silence = plugin->checkSilence();
            return player;
        }
    }
    return nullptr;
}

} // namespace chipmachine
