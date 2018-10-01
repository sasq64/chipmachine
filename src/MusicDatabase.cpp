#include "MusicDatabase.h"
#include "RemoteLoader.h"
#include "SongFileIdentifier.h"
#include "modutils.h"

#include <archive/archive.h>
#include <coreutils/environment.h>
#include <coreutils/searchpath.h>
#include <coreutils/utils.h>
#include <crypto/md5.h>
#include <webutils/web.h>
#include <xml/xml.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <set>

#include "../sol2/sol.hpp"

#include "csv.h"

using namespace utils;

// From Rosetta stone
// Compute Levenshtein Distance
// Martin Ettl, 2012-10-05
size_t levenshteinDistance(std::string const& s1, std::string const& s2)
{
    auto m = s1.length();
    auto n = s2.length();

    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<size_t> costs(n + 1);

    size_t i = 0;
    for (auto it1 = s1.begin(); it1 != s1.end(); ++it1, ++i) {
        costs[0] = i + 1;
        size_t corner = i;

        size_t j = 0;
        for (auto it2 = s2.begin(); it2 != s2.end(); ++it2, ++j) {
            size_t upper = costs[j + 1];
            if (*it1 == *it2) {
                costs[j + 1] = corner;
            } else {
                size_t t(upper < corner ? upper : corner);
                costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
            }

            corner = upper;
        }
    }

    size_t result = costs[n];

    return result;
}

namespace chipmachine {

void MusicDatabase::createTables()
{
    db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, "
            "localdir STRING, "
            "description STRING, id UNIQUE, version INTEGER)");
    db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, "
            "composer STRING, "
            "format STRING, path STRING, collection INTEGER, metadata STRING)");
    db.exec("CREATE TABLE IF NOT EXISTS product (title STRING, creator STRING, "
            "type STRING, "
            "screenshots STRING, collection INTEGER, metadata STRING)");
    db.exec("CREATE TABLE IF NOT EXISTS prod2song (songid INTEGER, prodid "
            "INTEGER)");
}

bool MusicDatabase::parseBitworld(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{
    for (auto const& s : apone::File{ listFile }.lines()) {
        auto parts = split(s, "\t");
        Product prod;
        // LOGD("ID: %s", parts[0]);
        prod.title = parts[1];
        prod.creator = parts[2];
        prod.type = "Amiga " + parts[3];
        prod.screenshots = parts[5];
        for (auto const& s : split(parts[4], ";")) {
            if (endsWith(s, ".smpl")) continue;
            if (s[0] == 'M')
                prod.songs.push_back(utils::urldecode(s.substr(2), ""));
            else
                prod.songs.push_back(s.substr(2));
        }
        callback(prod);
    }
    return true;
}

bool MusicDatabase::parseGamebase(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{

    using namespace io;
    CSVReader<3, trim_chars<' '>, double_quote_escape<',', '\"'>> in(listFile);
    in.read_header(io::ignore_extra_column, "Name", "ScrnshotFilename",
                   "SidFilename");
    std::string name, screenshot, sid;
    while (in.read_row(name, screenshot, sid)) {
        // do stuff with the data
        replace(screenshot.begin(), screenshot.end(), '\\', '/');
        replace(sid.begin(), sid.end(), '\\', '/');
        if (sid != "") {
            Product prod;
            prod.title = name;
            prod.type = "C64 Game";
            prod.screenshots = screenshot;
            prod.songs.push_back(sid);
            callback(prod);
        }
    }
    return true;
}

bool MusicDatabase::parseCsdb(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{

    auto doc = xmldoc::fromFile(listFile);
    for (auto const& i : doc["ReleasesWithHVSC"].all("Release")) {
        Product prod;
        prod.title = htmldecode(utf8_encode(i["Name"].text()));
        prod.type = i["ReleaseType"].text();
        auto rating = i["CSDbRating"];
        float rt = rating.valid() ? stod(rating.text()) : 0.0;
        // LOGD("Found %s (%s %d)", name, type, rt);
        std::string group;
        auto rb = i["ReleasedBy"];
        if (rb.valid()) {
            for (auto const& g : rb.all("Group")) {
                auto gn = utf8_encode(g["Group"].text());
                if (group != "") group += "+";
                group += gn;
            }
        }
        auto shot = i["Screenshot"];
        if (shot.valid()) {
            prod.screenshots = shot.text();
            // LOGD("Screenshot %s", prod.screenshots);
        }
        prod.creator = group;
        if ((endsWith(prod.type, "Music Collection") ||
             endsWith(prod.type, "Diskmag") || endsWith(prod.type, "Demo")) &&
            rt >= 0) {
            for (auto const& s : i["Sids"].all("HVSCPath")) {
                prod.songs.push_back(s.text().substr(1));
            }
            callback(prod);
        }
    }
    return true;
}

bool MusicDatabase::parsePouet(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{
    auto doc = xmldoc::fromFile(listFile);
    for (auto const& i : doc["feed"].all("prod")) {
        auto title = i["name"].text();
        auto g = i["group1"];
        auto group = g.valid() ? g.text() : "";
        auto youtube = i["youtube"].text();
        callback(SongInfo(youtube, "", title, group, "Youtube"));
    }
    return true;
}

bool MusicDatabase::parseAmp(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{
    File f{ listFile };

    for (auto const& s : f.getLines()) {
        SongInfo song;
        auto path = urldecode(s, "");
        auto parts = split(path, "/");
        if (parts.size() < 3) {
            LOGD("%s (%s) broken", s, path);
            continue;
        }
        int l = parts.size();
        auto titleParts = split(parts[l - 1], ".");
        if (titleParts.size() < 2) {
            LOGD("%s broken", s);
            continue;
        }
        titleParts[1][0] = toupper(titleParts[1][0]);
        song.path = s;
        song.composer = parts[l - 2];
        song.title = titleParts[1];
        song.format = titleParts[0] == "STK" ? "Soundtracker" : "Protracker";
        callback(song);
    }
    return true;
}

bool MusicDatabase::parseRss(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    xmldoc doc;

    try {
        doc = xmldoc::fromFile(listFile);
    } catch(xml_exception e) {
        return false;
    }
    auto rssNode = doc["rss"];
    if (!rssNode.valid()) {
        LOGE("Could not find rss node in xml");
        return false;
    }
    auto channelNode = rssNode["channel"];
    for (auto const& i : channelNode.all("item")) {
        auto title = i["title"].text();
        auto e = i["enclosure"];
        if (!e.valid()) continue;
        auto enclosure = e.attr("url");
        // LOGD("Title %s", title);
        std::string description;
        auto summary = i["itunes:summary"];
        auto sub_title = i["itunes:subtitle"];
        auto desc = i["description"];
        if (summary.valid())
            description = summary.text();
        else if (sub_title.valid())
            description = sub_title.text();
        else
            description = desc.text();

        description = htmldecode(description);

        std::string composer;

        auto c = i["dc:creator"];
        if (c.valid()) composer = c.text();
        /*if(composer == "") {
            auto dash = title.rfind(" - ");
            if(dash != std::string::npos) {
                composer = title.substr(dash + 2);
                title = title.substr(0, dash);
            }
        }*/

        auto pos = enclosure.find("file=");
        if (pos != std::string::npos) enclosure = enclosure.substr(pos + 5);

        callback(SongInfo(enclosure, "", title, composer, "MP3", description));
    }
    LOGD("Done");
    return true;
}

bool MusicDatabase::parseModland(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    static const std::set<std::string> secondary = { "smpl", "sam", "ins",
                                                     "smp",  "pdx", "nt",
                                                     "as" };
    static const std::set<std::string> secondary_pref = { "smpl", "smp" };
    static const std::set<std::string> hasSubFormats = { "Spectrum", "Ad Lib",
                                                         "Video Game Music" };

    auto parts = split(vars["exclude_formats"], ";");
    std::set<std::string> exclude(parts.begin(), parts.end());

    SongInfo lastSong;

    File f{ listFile };

    for (auto const& s : f.getLines()) {
        auto parts = split(s, "\t");
        if (parts.size() >= 2) {

            SongInfo song(parts[1]);

            /* std::string base = path_basename(song.path); */
            /* std::string ext = path_extension(song.path); */

            /* if(base == "mdat" || base == "jpn") { */
            /* std::swap(base, ext); */
            /* } */
            auto [ext, base] = getTypeAndBase(song.path);

            if ((secondary.count(ext) > 0) ||
                (secondary_pref.count(base) > 0) || endsWith(ext, "sflib")) {
                continue;
            }

            auto parts = split(song.path, "/");
            int l = parts.size();
            if (l < 3) {
                LOGD("%s", song.path);
                continue;
            }

            int i = 0;
            song.format = parts[i++];
            if (hasSubFormats.count(song.format) > 0) song.format = parts[i++];

            song.composer = parts[i++];

            if (song.format == "MDX") {
                i--;
                song.composer = "?";
            }

            if (song.composer == "- unknown") song.composer = "?";

            if (parts[i].substr(0, 5) == "coop-")
                song.composer = song.composer + "+" + parts[i++].substr(5);

            // std::string game;
            if (l - i >= 2) song.game = parts[i++];

            if (i == l) {
                LOGD("Bad file %s", song.path);
                continue;
            }

            if (endsWith(parts[i], ".rar"))
                parts[i] = parts[i].substr(0, parts[i].length() - 4);

            song.title = base;
            if (exclude.count(song.format) > 0) continue;
            if (song.game != "" && song.game == lastSong.game &&
                song.composer == lastSong.composer) {
                // Keep adding songs of the same game to lastSong
                if (!startsWith(lastSong.path, "MULTI:")) {
                    lastSong.path = std::string("MULTI:") + lastSong.path;
                    lastSong.title = "";
                }
                lastSong.path = lastSong.path + "\t" + song.path;
                continue;
            } else {
                // song is not the same as lastSong, commit lastSong
                if (lastSong.path != "") callback(lastSong);
                lastSong = song;
            }
        }
    }
    if (lastSong.path != "") callback(lastSong);
    return true;
}

bool MusicDatabase::parseStandard(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    int pathIndex = 4, gameIndex = 1, titleIndex = 0, composerIndex = 2,
        formatIndex = 3, metaIndex = 5;
    auto templ = vars["song_template"];
    // if(temp == "")
    //  templ = "title game composer format path meta";
    auto format = vars["format"];
    auto composer = vars["composer"];
    int columns = 2;
    if (templ != "") {
        formatIndex = gameIndex = composerIndex = -1;
        int i = 0;
        for (auto const& p : split(templ)) {
            if (p == "title")
                titleIndex = i;
            else if (p == "composer")
                composerIndex = i;
            else if (p == "path")
                pathIndex = i;
            else if (p == "format")
                formatIndex = i;
            else if (p == "game")
                gameIndex = i;
            i++;
        }
        columns = i;
    }

    bool isUtf8 = (vars["utf8"] != "no");
    bool htmlDec = (vars["html_decode"] != "no");
    auto source = vars["source"];

    File f{ listFile };

    for (auto const& s : f.getLines()) {
        auto parts = isUtf8 ? split(s, "\t") : split(utf8_encode(s), "\t");
        if (parts.size() >= columns) {

            if (htmlDec) {
                for (auto& p : parts)
                    p = htmldecode(p);
            }

            SongInfo song;
            std::string metadata;

            // Strip sorce from path if necessary
            if (source != "" && parts[pathIndex].find(source) == 0)
                parts[pathIndex] = parts[pathIndex].substr(source.length());

            if (parts.size() > metaIndex) metadata = parts[metaIndex];

            song = SongInfo(
                parts[pathIndex], gameIndex >= 0 ? parts[gameIndex] : "",
                parts[titleIndex],
                composerIndex >= 0 ? parts[composerIndex] : composer,
                formatIndex <= 0 ? format : parts[formatIndex], metadata);
            callback(song);
        }
    }
    return true;
}

void MusicDatabase::initDatabase(fs::path const& workDir, Variables& vars)
{

    auto id = vars["id"];
    auto type = vars["type"];
    if (type == "") type = id;
    auto name = vars["name"];
    auto source = vars["source"];
    auto screen_source = vars["screen_source"];
    fs::path local_dir = vars["local_dir"];
    auto song_list = vars["song_list"];
    auto prod_list = vars["prod_list"];
    auto remote_list = vars["remote_list"];
    auto description = vars["description"];

    LOGD("Checking %s", name);

    // Return if this collection has already been indexed in this version
    auto cq =
        db.query<uint64_t>("SELECT ROWID FROM collection WHERE id = ?", id);
    if (cq.step()) {
        return;
    }
    cq.finalize();

    reindexNeeded = true;

    if (local_dir != "") {
        if (!local_dir.is_absolute()) local_dir = workDir / local_dir;
    }

    print_fmt("Creating '%s' database\n", name);

    if (source == "") source = screen_source;

    db.exec("BEGIN TRANSACTION");
    db.exec("INSERT INTO collection (name, id, url, localdir, description) "
            "VALUES (?, ?, ?, ?, ?)",
            name, id, source, local_dir.string(), description);
    auto collection_id = db.last_rowid();
    dontIndex.resize(collection_id + 1);
    dontIndex[collection_id] = 0;

    if (vars["index"] == "no") {
        LOGD("Not indexing %s/%d", id, collection_id);
        dontIndex[collection_id] = 1;
    }

    LOGD("Workdir:%s", workDir);
    File listFile;
    bool writeListFile = false;
    webutils::Web web{ (Environment::getCacheDir() / "_webfiles").string() };

    bool prodCollection = false;

    if (prod_list != "") {
        song_list = prod_list;
        prodCollection = true;
    }

    if (song_list == "") song_list = remote_list;

    if (startsWith(song_list, "http://")) {
        listFile = web.getFileBlocking(song_list);
    } else if (song_list != "") {
        listFile = File(workDir.string(), song_list);
        writeListFile = listFile.exists();
    }

    if (prodCollection) {

        auto query = db.query("INSERT INTO product (title, creator, type, "
                              "screenshots, collection) "
                              "VALUES (?, ?, ?, ?, ?)");

        auto query2 = db.query("INSERT INTO prod2song (prodid, songid) "
                               "VALUES (?, ?)");

        std::map<std::string, ParseProdFun> parsers = {
            { "csdb", &MusicDatabase::parseCsdb },
            { "gb64", &MusicDatabase::parseGamebase },
            { "bitworld", &MusicDatabase::parseBitworld },
        };

        auto parser = parsers[type];
        // if(!parser)
        // parser = &MusicDatabase::parseStandard;
        LOGD("Parsing %s from %s", type, listFile.getName());

        (this->*parser)(vars, listFile.getName(), [&](Product const& prod) {
            query
                .bind(prod.title, prod.creator, prod.type, prod.screenshots,
                      collection_id)
                .step();
            auto prodrow = db.last_rowid();
            for (std::string path : prod.songs) {
                // TODO: Move to CORRECTIONS.LUA or something
                auto pos = path.find("Zombie (FI)");
                if (pos != std::string::npos)
                    path = path.substr(0, pos) + "Naksahtaja" +
                           path.substr(pos + 11);
                uint64_t hash = MD5::hash(toLower(path));
                auto it = pathMap.find(hash);
                if (it == pathMap.end()) {
                    LOGD("PATH '%s' not found", path);
                } else {
                    auto songrow = it->second;
                    query2.bind(prodrow, songrow).step();
                }
            }
        });
    } else {
        auto query = db.query("INSERT INTO song (title, game, composer, "
                              "format, path, collection, metadata) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?)");

        if (fs::exists(listFile)) {

            std::map<std::string, ParseSongFun> parsers = {
                { "pouet", &MusicDatabase::parseStandard },
                { "amp", &MusicDatabase::parseAmp },
                { "modland", &MusicDatabase::parseModland },
                { "podcast", &MusicDatabase::parseRss },
                { "standard", &MusicDatabase::parseStandard },
            };

            auto parser = parsers[type];
            if (!parser) parser = &MusicDatabase::parseStandard;

            (this->*parser)(vars, listFile, [&](SongInfo const& song) {
                query
                    .bind(song.title, song.game, song.composer, song.format,
                          song.path, collection_id,
                          song.metadata[SongInfo::INFO] != ""
                              ? song.metadata[SongInfo::INFO].c_str()
                              : nullptr)
                    .step();
                auto last = db.last_rowid();
                if (collection_id == 6) LOGD("Inserting '%s'", song.path);
                auto hash = MD5::hash(utils::toLower(song.path));
                pathMap[hash] = last;
            });

        } else if (fs::exists(local_dir)) {

            File root{ local_dir };
            LOGD("Checking local dir '%s'", root.getName());
            for (auto& rf : root.listRecursive()) {
                auto name = rf.getName();
                SongInfo songInfo(name);
                if (identify_song(songInfo)) {

                    auto pos = name.find(local_dir.string());
                    if (pos != std::string::npos) {
                        name = name.substr(pos + local_dir.string().length());
                    }

                    query
                        .bind(songInfo.title, songInfo.game, songInfo.composer,
                              songInfo.format, name, collection_id,
                              (char*)nullptr)
                        .step();
                    if (writeListFile)
                        listFile.writeln(join("\t", songInfo.title,
                                              songInfo.game, songInfo.composer,
                                              songInfo.format, name));
                }
            }
        }
    }

    listFile.close();
    db.exec("COMMIT");
}

void MusicDatabase::setFilter(std::string const& collection, int type)
{

    if (collection == "") {
        titleIndex.setFilter();
        collectionFilter = -1;
    } else {
        LOGD("FILTER: '%s'", collection);
        auto cq = db.query<int>("SELECT ROWID FROM collection WHERE id = ?",
                                collection);
        if (cq.step()) {
            collectionFilter = cq.get();
            LOGD("ID %d from %s", collectionFilter, collection);
            // collectionFilter = 2;
            titleIndex.setFilter([=](int index) {
                auto f = formats[index];
                if (type == 1 && (f & 0xff) == PRODUCT) return false;
                return ((formats[index] >> 8) != collectionFilter);
            });
        }
    }
}

int MusicDatabase::search(std::string const& query, std::vector<int>& result,
                          unsigned int searchLimit)
{

    std::lock_guard lock{ dbMutex };

    result.resize(0);

    std::string title_query = query;
    std::string composer_query = query;

    auto p = split(query, "/");
    if (p.size() > 1) {
        title_query = p[0];
        composer_query = p[1];
    }

    // For empty query, return all playlists
    if (query == "") {
        for (int i = 0; i < playLists.size(); i++) {
            result.push_back(PLAYLIST_INDEX + i);
        }
        return result.size();
    }

    // Push back all matching playlists
    for (int i = 0; i < playLists.size(); i++) {
        if (toLower(playLists[i].name).find(query) != std::string::npos)
            result.push_back(PLAYLIST_INDEX + i);
    }

    titleIndex.search(title_query, result, searchLimit);

    if (result.size() >= searchLimit) return searchLimit;

    searchLimit -= result.size();

    std::vector<int> cresult;
    composerIndex.search(composer_query, cresult, searchLimit);
    for (int index : cresult) {
        int offset = composerTitleStart[index];
        while (composerToTitle[offset] != -1) {
            if (result.size() >= searchLimit) break;
            int songindex = composerToTitle[offset++];

            if (collectionFilter == -1 ||
                (formats[songindex] >> 8) == collectionFilter)
                result.push_back(songindex);
        }
        if (result.size() >= searchLimit) break;
    }

    return result.size();
}

// Lookup the given path in the database
SongInfo& MusicDatabase::lookup(SongInfo& song)
{

    std::lock_guard lock{ dbMutex };
    auto path = song.path;

    auto parts = split(path, "::");
    if (parts.size() > 1) {
        path = parts[1];
        if (parts[0] == "index") {
            int index = stol(path);
            SongInfo song = getSongInfo(index);
            path = song.path;
            parts = split(path, "::");
            if (parts.size() > 1) {
                path = parts[1];
            }
        }
        LOGD("INDEX %s %s", parts[0], path);
    }

    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string, std::string>(
        "SELECT path, title, game, composer, format, collection.id, metadata "
        "FROM song, collection "
        "WHERE song.collection = collection.ROWID AND song.path = ?",
        path);

    if (q.step()) {
        std::string coll;
        tie(song.path, song.title, song.game, song.composer, song.format, coll,
            song.metadata[SongInfo::INFO]) = q.get_tuple();
        song.path = coll + "::" + song.path;
        LOGD("LOOKUP '%s' became '%s'", path, song.path);
    } else {
        LOGD("TODO: Check products");
    }

    return song;
}

std::string MusicDatabase::getScreenshotURL(std::string const& collection)
{
    std::string prefix;
    auto q = db.query<std::string>("SELECT url FROM collection WHERE id = ?",
                                   collection);
    if (q.step()) prefix = q.get();
    return prefix;
}

// Get SongInfo from the search result
SongInfo MusicDatabase::getSongInfo(int index) const
{

    if (index >= PLAYLIST_INDEX) {
        std::string p = playLists[index - PLAYLIST_INDEX].name;
        auto path = Environment::getConfigDir() / "playlists" / p;
        return SongInfo("playlist::" + path.string(), "", p, "",
                        "Local playlist");
    }

    index++;
    // LOGD("ID %d vs PROD %d", index, productStartIndex);
    if (index >= productStartIndex) {
        index -= productStartIndex;
        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string>(
            "SELECT title, creator, type, collection.id, metadata "
            "FROM  product, collection "
            "WHERE product.ROWID = ? AND product.collection = collection.ROWID",
            index);
        if (q.step()) {
            SongInfo song;
            std::string collection;
            tie(song.title, song.composer, song.format, collection,
                song.metadata[SongInfo::INFO]) = q.get_tuple();
            song.path = "product::" + std::to_string(index);
            return song;
        }

    } else {

        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string, std::string, std::string>(
            "SELECT title, game, composer, format, song.path, "
            "collection.id, metadata "
            "FROM song, collection "
            "WHERE song.ROWID = ? AND song.collection = collection.ROWID",
            index);
        if (q.step()) {
            SongInfo song;
            std::string collection;
            tie(song.title, song.game, song.composer, song.format, song.path,
                collection, song.metadata[SongInfo::INFO]) = q.get_tuple();
            song.path = collection + "::" + song.path;
            return song;
        }
    }
    throw not_found_exception();
}
std::string MusicDatabase::getSongScreenshots(SongInfo& s)
{

    lookup(s);
    auto parts = split(s.path, "::");
    LOGD(s.path);
    if (parts.size() < 2) return "";
    std::string collection = parts[0];
    std::string shot;
    std::string title;
    std::string baseName = path_basename(parts[1]);
    LOGD("Get screenhots / Path %s Collection '%s'", parts[1], parts[0]);
    if (s.metadata[SongInfo::SCREENSHOT] != "") {
        shot = s.metadata[SongInfo::SCREENSHOT];
    } else if (collection == "rsn") {
        auto base = path_basename(parts[1]);
        shot = std::string("http://snesmusic.org/v2/images/screenshots/") +
               base + ".png";
        s.metadata[SongInfo::SCREENSHOT] = shot;
        LOGD("Got rsn shot %s", shot);
    } else if (collection == "pouet" || collection == "radio" ||
               collection == "demovibes") {
        shot = s.metadata[SongInfo::INFO];
        s.metadata[SongInfo::SCREENSHOT] = shot;
        s.metadata[SongInfo::INFO] = "";
        LOGD("Got pouet shot %s", shot);
    } else {
        auto q = db.query<std::string, std::string, std::string, std::string>(
            "SELECT product.title, product.screenshots, product.type, "
            "collection.id "
            "FROM product, prod2song, song, collection "
            "WHERE product.rowid = prod2song.prodid AND prod2song.songid = "
            "song.ROWID AND "
            "product.collection = collection.ROWID AND song.path = ?",
            parts[1]);
        std::string format;
        int lowestDist = 999999;
        collection = "";
        while (q.step()) {
            std::string s, c;
            tie(title, s, format, c) = q.get_tuple();
            LOGD("%s Collection %s Format %s", title, c, format);
            auto ld = levenshteinDistance(title, baseName);
            if (collection == "gb64" && c == "csdb") ld += 7;
            LOGD("%s <=> %s : %d", title, baseName, ld);
            if (ld < lowestDist) {
                shot = s;
                collection = c;
                lowestDist = ld;
            }
            // if(format.find("Game") != std::string::npos ||
            // format.find("Demo") != std::string::npos ||
            // format.find("Trackmo") != std::string::npos)     break;
        }
    }
    if (shot != "") {
        std::string prefix;
        if (!startsWith(shot, "http")) prefix = getScreenshotURL(collection);
        auto parts = split(shot, ";");
        if (collection == "gb64")
            parts.insert(parts.begin(), path_directory(parts[0]) + "/" +
                                            path_basename(parts[0]) + "_1." +
                                            path_extension(parts[0]));
        for (auto& p : parts) {
            if (p != "") p.insert(0, prefix);
        }
        shot = join(parts, ";");
    }
    return shot;
}

std::string MusicDatabase::getProductScreenshots(uint32_t id)
{
    std::vector<std::string> shots;
    auto q = db.query<std::string, std::string>(
        "SELECT collection.id,screenshots "
        "FROM product, collection "
        "WHERE product.rowid = ? AND collection.ROWID = product.collection",
        id);

    std::string screenshot;
    std::string collection;

    if (q.step()) {
        tie(collection, screenshot) = q.get_tuple();
        auto prefix = getScreenshotURL(collection);
        auto parts = split(screenshot, ";");
        if (collection == "gb64")
            parts.push_back(path_basename(parts[0]) + "_1." +
                            path_extension(parts[0]));
        for (auto& p : parts) {
            p.insert(0, prefix);
        }
        return join(parts, ";");
    }
    return "";
}

std::vector<SongInfo> MusicDatabase::getProductSongs(uint32_t id)
{
    std::vector<SongInfo> songs;
    auto screenshot = getProductScreenshots(id);
    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string, std::string>(
        "SELECT title, game, composer, format, song.path, collection.id, "
        "metadata "
        "FROM song, prod2song, collection "
        "WHERE prodid = ? AND songid = song.ROWID AND song.collection = "
        "collection.ROWID",
        id);

    while (q.step()) {
        SongInfo song;
        std::string collection;
        tie(song.title, song.game, song.composer, song.format, song.path,
            collection, song.metadata[SongInfo::INFO]) = q.get_tuple();
        song.path = collection + "::" + song.path;
        song.metadata[SongInfo::SCREENSHOT] = screenshot;
        songs.push_back(song);
    }
    return songs;
}

#include "formats.h"

static std::map<std::string, uint8_t> format_map;

void initFormats()
{
    for (char const* f : uade_formats) {
        format_map[f] = UADE;
    }
    for (char const* f : adlib_formats) {
        format_map[f] = ADPLUG;
    }

    format_map["commodore 64"] = C64;
    format_map["cyber tracker"] = C64;
    format_map["super nintendo"] = SNES;
    format_map["hes"] = HES;
    format_map["mp3"] = MP3;
    format_map["sc68"] = ATARI;
    format_map["ultra64 sound format"] = NINTENDO64;
    format_map["nintendo ds sound format"] = NDS;
    format_map["nintendo sound format"] = NES;
    format_map["sega master system"] = SEGAMS;
    format_map["sega game gear"] = SEGAMS;
    format_map["playstation sound format"] = PLAYSTATION;
    format_map["dreamcast sound format"] = DREAMCAST;
    format_map["playlist"] = PLAYLIST;
    format_map["c64 demo"] = PLAYLIST;
    format_map["c64 event"] = PLAYLIST;
    format_map["pls"] = PLS;
    format_map["m3u"] = M3U;
}

static uint8_t formatToByte(std::string const& fmt, std::string const& path,
                            int coll)
{

    static bool init = false;
    if (!init) {
        initFormats();
        init = true;
    }

    std::string f = toLower(fmt);
    uint8_t l = format_map[f];
    if (l == 0) {

        l = UNKNOWN_FORMAT;

        if ((path.find("youtube.com/") != std::string::npos) ||
            (path.find("youtu.be/") != std::string::npos)) {
            return YOUTUBE;
        }

        if (endsWith(f, "tracker")) l = TRACKER;
        if (startsWith(f, "soundtracker"))
            l = SOUNDTRACKER;
        else if (startsWith(f, "protracker"))
            l = PROTRACKER;
        else if (startsWith(f, "fasttracker"))
            l = FASTTRACKER;
        else if (startsWith(f, "impulsetracker"))
            l = IMPULSETRACKER;
        else if (startsWith(f, "screamtracker"))
            l = SCREAMTRACKER;
        else if (startsWith(f, "atari"))
            l = ATARI;
        else if (startsWith(f, "ay ") || startsWith(f, "spectrum "))
            l = SPECTRUM;
        else if (startsWith(f, "gameboy"))
            l = GAMEBOY;
        if (f.find("megadrive") != std::string::npos) l = MEGADRIVE;
        if (l != UNKNOWN_FORMAT) format_map[f] = l;
        // fprintf(stderr, "%s\n", f.c_str());
    }
    return l;
}

template <typename T> static void readVector(std::vector<T>& v, apone::File& f)
{
    auto sz = f.read<uint32_t>();
    v.resize(sz);
    f.read((uint8_t*)&v[0], v.size() * sizeof(T));
}

template <typename T> static void writeVector(std::vector<T>& v, apone::File& f)
{
    f.write<uint32_t>(v.size());
    f.write((uint8_t*)&v[0], v.size() * sizeof(T));
}

void MusicDatabase::readIndex(apone::File&& f)
{

    indexVersion = 0;
    auto marker = f.read<uint16_t>();
    if (marker == 0xFEDC)
        indexVersion = f.read<uint16_t>();
    else
        f.seek(0);
    productStartIndex = f.read<uint32_t>();
    readVector(titleToComposer, f);
    readVector(composerToTitle, f);
    readVector(composerTitleStart, f);
    readVector(formats, f);

    titleIndex.load(f);
    composerIndex.load(f);
}

void MusicDatabase::writeIndex(apone::File&& f)
{
    f.write<uint16_t>(0xFEDC);
    f.write<uint16_t>(dbVersion);
    f.write<uint32_t>(productStartIndex);
    writeVector(titleToComposer, f);
    writeVector(composerToTitle, f);
    writeVector(composerTitleStart, f);
    writeVector(formats, f);

    titleIndex.dump(f);
    composerIndex.dump(f);
    f.close();
}

void MusicDatabase::generateIndex()
{

    // std::lock_guard lock{dbMutex};

    RemoteLoader& loader = RemoteLoader::getInstance();
    auto q = db.query<int, std::string, std::string, std::string>(
        "SELECT ROWID,id,url,localdir FROM collection");
    while (q.step()) {
        auto c = q.get<Collection>();
        // NOTE c.name is really c.id
        loader.registerSource(c.name, c.url, c.local_dir.string());
    }
    auto indexPath = Environment::getCacheDir() / "index.dat";

    if (!reindexNeeded && fs::exists(indexPath)) {
        readIndex(apone::File{ indexPath });
        return;
    }

    print_fmt("Creating Search Index...\n");

    std::string oldComposer;
    auto query = db.query<std::string, std::string, std::string, std::string,
                          std::string, int>(
        "SELECT title, game, format, composer, path, collection FROM song");

    int count = 0;
    // int maxTotal = 3;
    int cindex = 0;

    titleToComposer.reserve(438000);
    composerToTitle.reserve(37000);
    titleIndex.reserve(438000);
    composerIndex.reserve(37000);
    formats.reserve(438000);

    int step = 438000 / 20;

    std::unordered_map<std::string, std::vector<uint32_t>> composers;

    std::string title, game, fmt, composer, path;
    int collection;

    while (count < 1000000) {
        count++;
        if (!query.step()) break;

        if (count % step == 0) {
            LOGD("%d songs indexed", count);
        }

        tie(title, game, fmt, composer, path, collection) = query.get_tuple();

        uint8_t b = formatToByte(fmt, path, collection);
        formats.push_back(b | (collection << 8));

        if (game != "") {
            if (title != "")
                title = format("%s [%s]", game, title);
            else
                title = game;
        }

        if (dontIndex[collection]) {
            title = "";
            composer = "";
        }

        // The title index maps one-to-one with the database
        int tindex = titleIndex.add(title);

        auto& v = composers[composer];
        if (v.empty()) {
            cindex = composerIndex.add(composer);
            composers[composer].push_back(cindex);
        } else
            cindex = composers[composer][0];

        composers[composer].push_back(tindex);

        // We also need to find the composer for a give title
        titleToComposer.push_back(cindex);
    }

    productStartIndex = titleIndex.size();

    auto prodQuery = db.query<std::string, std::string, std::string, int>(
        "SELECT title, type, creator, collection FROM product");
    while (count < 1000000) {
        count++;
        if (!prodQuery.step()) break;

        if (count % step == 0) {
            LOGD("%d songs indexed", count);
        }

        tie(title, fmt, composer, collection) = prodQuery.get_tuple();

        uint8_t b = PRODUCT;
        formats.push_back(b | (collection << 8));

        if (dontIndex[collection]) {
            title = "";
            composer = "";
        }

        // The title index maps one-to-one with the database
        int tindex = titleIndex.add(title);

        auto& v = composers[composer];
        if (v.empty()) {
            cindex = composerIndex.add(composer);
            composers[composer].push_back(cindex);
        } else
            cindex = composers[composer][0];

        composers[composer].push_back(tindex);

        // We also need to find the composer for a give title
        titleToComposer.push_back(cindex);
    }

    // composers[name] -> std::vector of titleindexes for each composer.

    LOGD("Found %d composers and %d titles", composers.size(),
         titleToComposer.size());

    composerTitleStart.resize(composers.size());
    for (auto const& p : composers) {
        // p,first == composer, p.second == std::vector
        auto cindex = p.second[0];
        composerTitleStart[cindex] = composerToTitle.size();
        for (int i = 1; i < (int)p.second.size(); i++)
            composerToTitle.push_back(p.second[i]);
        composerToTitle.push_back(-1);
    }

    writeIndex(apone::File{ indexPath, apone::File::Write });

    reindexNeeded = false;
}

void MusicDatabase::initFromLuaAsync(fs::path const& workDir)
{
    indexing = true;
    initFuture = std::async(std::launch::async, [=]() {
        std::lock_guard lock{ dbMutex };
        if (!initFromLua(workDir)) {
        }
        std::lock_guard lock2{ chkMutex };
        indexing = false;
    });
}

bool MusicDatabase::initFromLua(fs::path const& workDir)
{
    auto playlistPath = Environment::getConfigDir() / "playlists";
    fs::create_directory(playlistPath);
    bool favFound = false;
    for (auto const& f : fs::directory_iterator(playlistPath)) {
        playLists.emplace_back(f);
        if (playLists.back().name == "Favorites") favFound = true;
    }
    if (!favFound) {
        playLists.emplace_back(playlistPath / "Favorites");
        playLists.back().save();
    }

    reindexNeeded = false;
    auto indexDir = Environment::getCacheDir() / "index.dat";

    indexVersion = 0;
    if (fs::exists(indexDir)) {
        apone::File fi{ indexDir };
        auto marker = fi.read<uint16_t>();
        if (marker == 0xFEDC) indexVersion = fi.read<uint16_t>();
    }

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package);

    std::map<std::string, std::string> dbmap;
    lua["create_db"] = [&] {
        initDatabase(workDir, dbmap);
        dbmap.clear();
    };

    lua["set_db_var"] = sol::overload(
        [&](std::string const& name, std::string val) { dbmap[name] = val; },
        [&](std::string const& name, uint32_t val) {
            dbmap[name] = std::to_string(val);
        });

    if (auto f = findFile(workDir.string(), "lua/db.lua")) {
        lua.script_file(f->string());
    }

    dbVersion = lua["VERSION"];
    LOGD("DBVERSION %d INDEXVERSION %d", dbVersion, indexVersion);
    if (dbVersion != indexVersion) {
        db.exec("DROP TABLE IF EXISTS collection");
        db.exec("DROP TABLE IF EXISTS song");
        db.exec("DROP TABLE IF EXISTS product");
        db.exec("DROP TABLE IF EXISTS prod2song");
        createTables();
        reindexNeeded = true;
    }

    lua.script(R"(
        for a,b in pairs(DB) do
            if type(b) == 'table' then
                for a1,b1 in pairs(b) do
                    set_db_var(a1, b1)
                end
                create_db()
            end
        end
    )");
    generateIndex();
    return true;
}

int MusicDatabase::getSongs(std::vector<SongInfo>& target,
                            SongInfo const& match, int limit, bool random)
{

    std::lock_guard lock{ dbMutex };
    std::string txt =
        "SELECT path, game, title, composer, format, collection.id "
        "FROM song, collection "
        "WHERE song.collection = collection.ROWID";

    std::string collection;
    if (match.path != "") {
        auto parts = split(match.path, "::");
        if (parts.size() >= 2) collection = parts[0];
    }

    if (match.format != "") txt += " AND format=?";
    if (match.composer != "") txt += " AND composer=?";
    if (collection != "") txt += " AND collection.id=?";
    if (random) txt += " ORDER BY RANDOM()";
    if (limit > 0) txt += format(" LIMIT %d", limit);

    LOGD("SQL:%s", txt);

    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string>(txt);
    int index = 1;
    if (match.format != "") q.bind(index++, match.format);
    if (match.composer != "") q.bind(index++, match.composer);
    if (collection != "") q.bind(index++, collection);

    while (q.step()) {
        std::string collection;
        SongInfo song;
        tie(song.path, song.game, song.title, song.composer, song.format,
            collection) = q.get_tuple();
        song.path = collection + "::" + song.path;
        if (song.game != "")
            song.title = utils::format("%s [%s]", song.game, song.title);
        target.push_back(song);
    }
    return 0;
}

void MusicDatabase::addToPlaylist(std::string const& plist,
                                  SongInfo const& song)
{
    for (auto& pl : playLists) {
        if (pl.name == plist) {
            pl.songs.push_back(song);
            pl.save();
            break;
        }
    }
}

void MusicDatabase::removeFromPlaylist(std::string const& plist,
                                       SongInfo const& toRemove)
{
    for (auto& pl : playLists) {
        if (pl.name == plist) {
            pl.songs.erase(std::remove_if(pl.songs.begin(), pl.songs.end(),
                                          [&](SongInfo const& song) -> bool {
                                              return song.path ==
                                                         toRemove.path &&
                                                     (song.starttune == -1 ||
                                                      song.starttune ==
                                                          toRemove.starttune);
                                          }),
                           pl.songs.end());
            pl.save();
            break;
        }
    }
}

std::vector<SongInfo>& MusicDatabase::getPlaylist(std::string const& plist)
{
    static std::vector<SongInfo> empty;
    for (auto& pl : playLists) {
        if (pl.name == plist) return pl.songs;
    }
    return empty;
}
} // namespace chipmachine
