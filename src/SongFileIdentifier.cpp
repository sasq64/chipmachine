#include "SongFileIdentifier.h"
#include "modutils.h"

#include <archive/archive.h>
#include <coreutils/environment.h>
#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/split.h>
#include <coreutils/utils.h>

#ifdef WITH_MPG123
#    include <mpg123.h>
#endif

#include <algorithm>
#include <memory>
#include <string>

using apone::File;

static std::string get_string(uint8_t* ptr, int size)
{

    auto end = ptr;
    while (*end && end - ptr < size)
        end++;
    return std::string((const char*)ptr, end - ptr);
}

std::vector<std::string> getLines(std::string const& text)
{
    std::vector<std::string> lines;

    char tmp[256];
    char* t = tmp;
    const char* ptr = text.c_str();
    bool eol = false;
    while (*ptr) {
        if (t - tmp >= 255) break;

        while (*ptr == 10 || *ptr == 13) {
            ptr++;
            eol = true;
        }
        if (eol) {
            *t = 0;
            t = tmp;
            lines.emplace_back(tmp);
            eol = false;
        }
        *t++ = *ptr++;
    }

    *t = 0;
    t = tmp;
    if (strlen(t) > 0) lines.emplace_back(tmp);

    return lines;
}

bool parseSid(SongInfo& info)
{
    static std::vector<uint8_t> buffer(0xd8);
    File f{ info.path };
    info.format = "Commodore 64";
    f.read(&buffer[0], buffer.size());
    info.title = utils::utf8_encode(get_string(&buffer[0x16], 0x20));
    info.composer = utils::utf8_encode(get_string(&buffer[0x36], 0x20));
    // auto copyright = std::string((const char*)&buffer[0x56], 0x20);
    f.close();
    return true;
}

bool parseSap(SongInfo& info)
{
    File f{ info.path };

    auto data = f.readAll();

    auto end_of_header = search_n(data.begin(), data.end(), 2, 0xff);
    auto header = std::string(data.begin(), end_of_header);
    auto lines = getLines(header);

    if (lines.empty() || lines[0] != "SAP") return false;

    for (const auto& l : lines) {
        if (utils::startsWith(l, "AUTHOR"))
            info.composer = utils::lrstrip(l.substr(7), '\"');
        else if (utils::startsWith(l, "NAME"))
            info.title = utils::lrstrip(l.substr(5), '\"');
    }

    info.format = "Atari 8Bit";

    return true;
}

extern "C"
{
    int unice68_depacker(void* dest, const void* src);
    int unice68_get_depacked_size(const void* buffer, int* p_csize);
}

bool parseSndh(SongInfo& info)
{

    std::unique_ptr<uint8_t[]> unpackPtr;
    File f{ info.path };
    LOGD("SNDH >%s", info.path);
    auto data = f.readAll();
    if (data.size() < 32) return false;
    auto* ptr = &data[0];
    std::string head = get_string(ptr, 4);
    if (head == "ICE!") {
        int dsize = unice68_get_depacked_size(ptr, nullptr);
        LOGD("Unicing %d bytes to %d bytes", data.size(), dsize);
        unpackPtr = std::make_unique<uint8_t[]>(dsize);
        int res = unice68_depacker(unpackPtr.get(), ptr);
        if (res == 0) ptr = unpackPtr.get();
    }

    auto id = get_string(ptr + 12, 4);

    if (id == "SNDH") {

        info.format = "Atari ST";

        // LOGD("SNDH FILE");
        int count = 10;
        int got = 0;
        ptr += 16;
        std::string arg;
        std::string tag = get_string(ptr, 4);
        // LOGD("TAG %s", tag);
        while (tag != "HDNS") {
            if (count-- == 0) break;
            uint8_t* p = ptr;
            if (tag == "#!SN" || tag == "TIME") {
                ptr += 12;
            } else {
                while (*ptr)
                    ptr++;
                while (!(*ptr))
                    ptr++;
            }
            if (tag == "TITL") {
                got |= 1;
                info.title = get_string(p + 4, 256);
            } else if (tag == "COMM") {
                got |= 2;
                info.composer = get_string(p + 4, 256);
            }
            if (got == 3) break;
            tag = get_string(ptr, 4);
            // LOGD("TAG %s", tag);
        }
        LOGD("%s - %s", info.title, info.composer);
        return true;
    }
    return false;
}

bool parseSnes(SongInfo& info)
{
    static std::vector<uint8_t> buffer(0xd8);

    info.format = "Super Nintendo";

    utils::path outDir = Environment::getCacheDir() / ".rsntemp";
    auto* a = utils::Archive::open(info.path, outDir.string(),
                                   utils::Archive::TYPE_RAR);
    // LOGD("ARCHIVE %p", a);
    bool done = false;
    for (auto const& s : *a) {
        // LOGD("FILE %s", s);
        if (done) continue;
        if (utils::path_extension(s) == "spc") {
            a->extract(s);
            File f{ outDir / s };
            f.read(&buffer[0], buffer.size());
            if (buffer[0x23] == 0x1a) {
                // auto title = std::string((const char*)&buffer[0x2e], 0x20);
                auto ptr = (const char*)&buffer[0x4e];
                auto end = ptr;
                while (*end)
                    end++;
                auto game = get_string(&buffer[0x4e], 0x20);
                auto composer = get_string(&buffer[0xb1], 0x20);

                f.seek(0x10200);
                int rc = f.read(&buffer[0], buffer.size());
                if (rc > 12) {
                    auto id = std::string((const char*)&buffer[0], 4);
                    if (id == "xid6") {
                        // int i = 0;
                        if (buffer[8] == 0x2) {
                            int l = buffer[10];
                            game = std::string((const char*)&buffer[12], l);
                        } else if (buffer[8] == 0x3) {
                            int l = buffer[10];
                            composer = std::string((const char*)&buffer[12], l);
                        }
                    }
                }
                f.close();

                info.composer = composer;
                info.game = game;
                info.title = "";
                done = true;
            }
        }
    }
    delete a;
    return done;
}

bool parseMp3(SongInfo& info)
{
#ifdef WITH_MPG123
    int err = mpg123_init();
    mpg123_handle* mp3 = mpg123_new(NULL, &err);

    if (mpg123_open(mp3, info.path.c_str()) != MPG123_OK) return false;

    mpg123_format_none(mp3);

    mpg123_scan(mp3);
    int meta = mpg123_meta_check(mp3);
    mpg123_id3v1* v1;
    mpg123_id3v2* v2;
    if (meta & MPG123_ID3 && mpg123_id3(mp3, &v1, &v2) == MPG123_OK) {
        if (v2) {
            info.title = htmldecode(v2->title->p);
            info.composer = htmldecode(v2->artist->p);
        } else if (v1) {
            info.title = htmldecode((char*)v2->title);
            info.composer = htmldecode((char*)v2->artist);
        }
    }

    info.format = "MP3";

    if (mp3) {
        mpg123_close(mp3);
        mpg123_delete(mp3);
    }
    mpg123_exit();
    return true;
#else
    (void)info;
    return false;
#endif
}

bool parsePList(SongInfo& info)
{

    File f{ info.path };

    info.title = utils::path_basename(info.path);
    info.composer = "";
    info.format = "Playlist";

    for (auto const& l : f.lines()) {
        if (l.length() > 0 && l[0] == ';') {
            auto parts = utils::split(l.substr(1), "\t");
            info.title = parts[0];
            if (parts.size() >= 2) {
                info.composer = parts[1];
                info.format = "C64 Demo";
            } else
                info.format = "C64 Event";
        }
    }
    return true;
}

bool parseNsfe(SongInfo& song)
{
    File f{ song.path };
    if (f.readString(4) != "NSFE") return false;
    while (!f.eof()) {
        auto size = f.read<uint32_t>();
        auto tag = f.readString(4);
        auto next = f.tell() + size;

        if (tag == "auth") {
            song.game = f.readString();
            song.composer = f.readString();
            return true;
        }
        f.seek(next);
    }
    return false;
}

static void fixName(std::string& name)
{
    bool capNext = true;
    for (size_t i = 0; i < name.size(); i++) {
        auto& c = name[i];
        if (capNext) {
            c = toupper(c);
            capNext = (c == 'I' && name[i + 1] == 'i');
        }
        if (c == '_') {
            capNext = true;
            c = ' ';
        }
    }
}

bool identify_song(SongInfo& info, std::string ext)
{

    if (ext.empty()) ext = getTypeFromName(info.path);

    if (ext == "prg") {

        auto parts = utils::split(info.metadata[SongInfo::INFO], "/");
        LOGD("PARTS %s", parts);
        int l = parts.size();
        auto title = utils::path_basename(parts[l - 1]);
        fixName(title);
        std::string composer = "Unknown";
        if (strcmp(parts[0], "musicians") == 0) {
            composer = parts[1];
            std::vector<std::string> cp = utils::split(composer, "_");
            auto cpl = cp.size();
            if (cpl > 1 && cp[0] != "the" && cp[0] != "billy" &&
                cp[0] != "legion") {
                auto t = cp[0];
                cp[0] = cp[cpl - 1];
                cp[cpl - 1] = t;
            }
            for (auto& cpp : cp) {
                cpp[0] = toupper(cpp[0]);
            }

            composer = utils::join(cp.begin(), cp.end(), " "s);
        }

        info.format = "TED";
        info.title = title;
        info.composer = composer;

        return true;
    }

    if (ext == "nsfe") return parseNsfe(info);
    if (ext == "plist") return parsePList(info);
    if (ext == "rsn") return parseSnes(info);
    if (ext == "sid") return parseSid(info);
    if (ext == "sndh") return parseSndh(info);
    if (ext == "sap") return parseSap(info);
    if (ext == "mp3") return parseMp3(info);
    return false;
}
