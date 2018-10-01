#ifndef MUSIC_DATABASE_H
#define MUSIC_DATABASE_H

#include "SearchIndex.h"
#include "SongInfo.h"

#include <coreutils/environment.h>
#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>

#include <coreutils/thread.h>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

//#include <experimental/filesystem>
//namespace fs = std::experimental::filesystem;

namespace chipmachine {

class not_found_exception : public std::exception
{
public:
    virtual char const* what() const throw() { return "Not found exception"; }
};

// console -- sid -- tracker -- amiga
enum Formats
{

    NOT_SET,

    UNKNOWN_FORMAT,
    NO_FORMAT,
    PLAYLIST,

    CONSOLE,

    HES,

    NINTENDO,

    GAMEBOY,
    NES,
    SNES,
    NINTENDO64,
    GBA,
    NDS,

    SEGA,

    SEGAMS,
    MEGADRIVE,
    DREAMCAST,

    SONY,

    PLAYSTATION,
    PLAYSTATION2,

    COMPUTER,
    C64,
    SID,

    SPECTRUM,

    ATARI,

    MP3,

    M3U,
    PLS,

    OGG,

    YOUTUBE,

    PC,

    ADPLUG,
    TRACKER = 0x30,
    SCREAMTRACKER,
    IMPULSETRACKER,
    FASTTRACKER,

    AMIGA,
    PROTRACKER,
    SOUNDTRACKER,

    UADE,

    PRODUCT = 0x40

};

struct Product
{
    std::string title;
    std::string creator;
    std::string type;
    std::string screenshots;
    std::vector<std::string> songs;
};

class MusicDatabase : public SearchProvider
{
public:
    using Variables = std::map<std::string, std::string>;

    MusicDatabase()
        : db((Environment::getCacheDir() / "music.db").string()), reindexNeeded(false)
    {
        createTables();
    }

    bool initFromLua(utils::path const& workDir);
    void initFromLuaAsync(utils::path const& workDir);

    int search(std::string const& query, std::vector<int>& result,
               unsigned int searchLimit) override;
    // Lookup internal string for index
    std::string getString(int index) const override
    {
        // std::lock_guard lock{dbMutex};
        return utils::format("%s %s", getTitle(index), getComposer(index));
    }

    std::string getFullString(int index) const override
    {
        // std::lock_guard lock{dbMutex};
        int f;
        if (index >= PLAYLIST_INDEX)
            f = PLAYLIST;
        else
            f = formats[index];
        return utils::format("%s\t%s\t%d\t%d", getTitle(index),
                             getComposer(index), index, f);
    }
    // Get full data, may require SQL query
    SongInfo getSongInfo(int index) const;

    std::string getTitle(int index) const
    {
        std::lock_guard lock{ dbMutex };
        if (index >= PLAYLIST_INDEX)
            return playLists[index - PLAYLIST_INDEX].name;
        return titleIndex.getString(index);
    }

    std::string getComposer(int index) const
    {
        std::lock_guard lock{ dbMutex };
        if (index >= PLAYLIST_INDEX) return "";
        return composerIndex.getString(titleToComposer[index]);
    }

    std::shared_ptr<IncrementalQuery> createQuery()
    {
        std::lock_guard lock{ dbMutex };
        return std::make_shared<IncrementalQuery>(this);
    }

    int getSongs(std::vector<SongInfo>& target, SongInfo const& match,
                 int limit, bool random);

    bool busy()
    {
        std::lock_guard lock{ chkMutex };
        if (initFuture.valid()) {
            if (initFuture.wait_for(std::chrono::milliseconds(1)) ==
                std::future_status::ready) {
                initFuture.get();
                return false;
            }
            return true;
        }

        if (dbMutex.try_lock()) {
            dbMutex.unlock();
            return false;
        }
        return true;
    }

    SongInfo& lookup(SongInfo& song);

    std::vector<SongInfo> getProductSongs(uint32_t id);

private:
    std::string getProductScreenshots(uint32_t id);
    std::string getScreenshotURL(std::string const& collection);

public:
    std::string getSongScreenshots(SongInfo& s);

    static MusicDatabase& getInstance()
    {
        static MusicDatabase mdb;
        return mdb;
    }

    struct Playlist
    {
        Playlist(utils::path f) : fileName(f.string())
        {
            if (fs::exists(f)) {
                for (auto const& l : apone::File{ f }.lines()) {
                    if (l != "") songs.emplace_back(l);
                }
            }
            name = f.filename().string();
        }
        std::string name;
        std::string fileName;
        std::vector<SongInfo> songs;
        void save()
        {
            apone::File f{ fileName, apone::File::Write };
            LOGD("Writing to %s", fileName);
            for (auto const& s : songs) {
                if (s.starttune >= 0)
                    f.writeln(utils::format("%s;%d", s.path, s.starttune));
                else
                    f.writeln(s.path);
            }
        }
    };

    void addToPlaylist(std::string const& plist, SongInfo const& song);
    void removeFromPlaylist(std::string const& plist, SongInfo const& toRemove);
    std::vector<SongInfo>& getPlaylist(std::string const& plist);

    void setFilter(std::string const& filter, int type = 0);

private:
    void initDatabase(utils::path const& workDir, Variables& vars);
    void generateIndex();

    struct Collection
    {
        Collection(int id = -1, std::string const& name = "",
                   std::string const& url = "", utils::path const& local_dir = utils::path(""))
            : id(id), name(name), url(url), local_dir(local_dir)
        {}
        int id;
        std::string name;
        std::string url;
        utils::path local_dir;
    };

    template <typename T> using Callback = std::function<void(T const&)>;

    typedef bool (MusicDatabase::*ParseSongFun)(Variables&, std::string const&,
                                                Callback<SongInfo> const&);
    typedef bool (MusicDatabase::*ParseProdFun)(Variables&, std::string const&,
                                                Callback<Product> const&);

    bool parseCsdb(Variables& vars, std::string const& listFile,
                   Callback<Product> const& callback);
    bool parseBitworld(Variables& vars, std::string const& listFile,
                       Callback<Product> const& callback);
    bool parseGamebase(Variables& vars, std::string const& listFile,
                       Callback<Product> const& callback);
    bool parsePouet(Variables& vars, std::string const& listFile,
                    Callback<SongInfo> const& callback);
    bool parseRss(Variables& vars, std::string const& listFile,
                  Callback<SongInfo> const& callback);
    bool parseModland(Variables& vars, std::string const& listFile,
                      Callback<SongInfo> const& callback);
    bool parseAmp(Variables& vars, std::string const& listFile,
                  Callback<SongInfo> const& callback);
    bool parseStandard(Variables& vars, std::string const& listFile,
                       Callback<SongInfo> const& callback);

    void writeIndex(apone::File&& f);
    void readIndex(apone::File&& f);

    void createTables();

    static constexpr int PLAYLIST_INDEX = 0x10000000;

    SearchIndex composerIndex;
    SearchIndex titleIndex;

    std::vector<uint32_t> titleToComposer;
    std::vector<uint32_t> composerToTitle;
    std::vector<uint32_t> composerTitleStart;
    std::vector<uint16_t> formats;

    mutable std::mutex chkMutex;
    mutable std::mutex dbMutex;
    sqlite3db::Database db;
    bool reindexNeeded;

    uint16_t dbVersion;
    uint16_t indexVersion;

    int collectionFilter = -1;

    std::future<void> initFuture;
    std::atomic<bool> indexing;

    std::vector<Playlist> playLists;
    std::unordered_map<uint64_t, uint32_t> pathMap;
    uint32_t productStartIndex;
    std::vector<uint8_t> dontIndex;
};
} // namespace chipmachine

#endif // MUSIC_DATABASE_H
