#include "MusicDatabase.h"
#include "RemoteLoader.h"
#include "SongFileIdentifier.h"

#include <luainterpreter/luainterpreter.h>
#include <coreutils/utils.h>
#include <crypto/md5.h>
#include <webutils/web.h>
#include <archive/archive.h>
#include <xml/xml.h>

#include <set>
#include <chrono>
#include <algorithm>
#include <sqlite3/database.h>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicDatabase::MusicDatabase() : db_ptr(std::make_unique<sqlite3db::Database>(utils::File::getCacheDir() / "music.db")), db(*db_ptr), reindexNeeded(false) {
	createTables();
}

MusicDatabase::~MusicDatabase() {
};

void MusicDatabase::createTables() {
	db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, localdir STRING, "
	        "description STRING, id UNIQUE, version INTEGER)");
	db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, "
	        "format STRING, path STRING, collection INTEGER, metadata STRING)");
	db.exec("CREATE TABLE IF NOT EXISTS product (title STRING, creator STRING, type STRING, "
	        "screenshots STRING, collection INTEGER, metadata STRING)");
	db.exec("CREATE TABLE IF NOT EXISTS prod2song (songid INTEGER, prodid INTEGER)");
}

bool MusicDatabase::parseBitworld(const std::string &listFile,
                                  std::function<void(const Product &)> callback) {

	File f{listFile};
	Product prod;
	std::string id, songs;

	for(const auto &s : f.getLines()) {
		tie(id, prod.title, prod.creator, prod.type, songs, prod.screenshots) = tusplit<6>(s, "\t");
		prod.songs.clear();
		for(const auto &s : split(songs, ";")) {
			if(endsWith(s, ".smpl"))
				continue;
			if(s[0] == 'M')
				prod.songs.push_back(utils::urldecode(s.substr(2)));
			else
				prod.songs.push_back(s.substr(2));
		}
		callback(prod);
	}
	return true;
}

bool MusicDatabase::parseCsdb(const std::string &listFile,
                              std::function<void(const Product &)> callback) {

	auto doc = xmldoc::fromFile(listFile);
	for(const auto &i : doc["ReleasesWithHVSC"].all("Release")) {
		Product prod;
		prod.title = htmldecode(utf8_encode(i["Name"].text()));
		prod.type = i["ReleaseType"].text();
		auto rating = i["CSDbRating"];
		float rt = rating.valid() ? stof(rating.text()) : 0.f;
		// LOGD("Found %s (%s %d)", name, type, rt);
		string group = "";
		auto rb = i["ReleasedBy"];
		if(rb.valid()) {
			for(const auto &g : rb.all("Group")) {
				auto gn = utf8_encode(g["Group"].text());
				if(group != "")
					group += "+";
				group += gn;
			}
		}
		auto shot = i["Screenshot"];
		if(shot.valid()) {
			prod.screenshots = shot.text();
			LOGD("Screenshot %s", prod.screenshots);
		}
		prod.creator = group;
		if((endsWith(prod.type, "Music Collection") || endsWith(prod.type, "Diskmag") ||
		    endsWith(prod.type, "Demo")) &&
		   rt > 0) {
			for(const auto &s : i["Sids"].all("HVSCPath")) {
				prod.songs.push_back(s.text().substr(1));
			}
			callback(prod);
		}
	}
	return true;
}

bool MusicDatabase::parsePouet(Variables &vars, const std::string &listFile,
                               std::function<void(const SongInfo &)> callback) {
	auto doc = xmldoc::fromFile(listFile);
	for(const auto &i : doc["feed"].all("prod")) {
		auto title = i["name"].text();
		auto g = i["group1"];
		auto group = g.valid() ? g.text() : "";
		auto youtube = i["youtube"].text();
		callback(SongInfo(youtube, "", title, group, "Youtube"));
	}
	return true;
}

bool MusicDatabase::parseAmp(Variables &vars, const std::string &listFile,
                             std::function<void(const SongInfo &)> callback) {
	File f{listFile};

	for(const auto &s : f.getLines()) {
		SongInfo song;
		auto path = urldecode(s, "");
		auto parts = split(path, "/");
		if(parts.size() < 3) {
			LOGD("%s (%s) broken", s, path);
			continue;
		}
		int l = parts.size();
		auto titleParts = split(parts[l - 1], ".");
		if(titleParts.size() < 2) {
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

bool MusicDatabase::parseRss(Variables &vars, const std::string &listFile,
                             std::function<void(const SongInfo &)> callback) {

	auto doc = xmldoc::fromFile(listFile);
	auto rssNode = doc["rss"];
	if(!rssNode.valid()) {
		LOGE("Could not find rss node in xml");
		return false;
	}
	auto channelNode = rssNode["channel"];
	for(const auto &i : channelNode.all("item")) {
		auto title = i["title"].text();
		auto e = i["enclosure"];
		if(!e.valid())
			continue;
		auto enclosure = e.attr("url");
		// LOGD("Title %s", title);
		string description = "";
		auto summary = i["itunes:summary"];
		auto sub_title = i["itunes:subtitle"];
		auto desc = i["description"];
		if(summary.valid())
			description = summary.text();
		else if(sub_title.valid())
			description = sub_title.text();
		else
			description = desc.text();

		description = htmldecode(description);

		string composer = "";

		auto c = i["dc:creator"];
		if(c.valid())
			composer = c.text();
		/*if(composer == "") {
		    auto dash = title.rfind(" - ");
		    if(dash != string::npos) {
		        composer = title.substr(dash + 2);
		        title = title.substr(0, dash);
		    }
		}*/

		auto pos = enclosure.find("file=");
		if(pos != string::npos)
			enclosure = enclosure.substr(pos + 5);

		callback(SongInfo(enclosure, "", title, composer, "MP3", description));
	}
	return true;
}

bool MusicDatabase::parseModland(Variables &vars, const std::string &listFile,
                                 std::function<void(const SongInfo &)> callback) {

	static const std::set<std::string> secondary = {"smpl", "sam", "ins", "smp", "pdx", "nt", "as"};
	static const unordered_set<string> hasSubFormats = {"Ad Lib", "Video Game Music"};

	auto parts = split(vars["exclude_formats"], ";");
	set<string> exclude(parts.begin(), parts.end());

	SongInfo lastSong;

	File f{listFile};

	for(const auto &s : f.getLines()) {
		auto parts = split(s, "\t");
		if(parts.size() >= 2) {

			SongInfo song(parts[1]);

			string ext = path_extension(song.path);
			if((secondary.count(ext) > 0) || endsWith(ext, "sflib")) {
				continue;
			}

			auto parts = split(song.path, "/");
			int l = parts.size();
			if(l < 3) {
				LOGD("%s", song.path);
				continue;
			}

			int i = 0;
			song.format = parts[i++];
			if(hasSubFormats.count(song.format) > 0)
				song.format = parts[i++];

			song.composer = parts[i++];

			if(song.format == "MDX") {
				i--;
				song.composer = "?";
			}

			if(song.composer == "- unknown")
				song.composer = "?";

			if(parts[i].substr(0, 5) == "coop-")
				song.composer = song.composer + "+" + parts[i++].substr(5);

			// string game;
			if(l - i >= 2)
				song.game = parts[i++];

			if(i == l) {
				LOGD("Bad file %s", song.path);
				continue;
			}

			if(endsWith(parts[i], ".rar"))
				parts[i] = parts[i].substr(0, parts[i].length() - 4);

			song.title = path_basename(parts[i++]);
			if(exclude.count(song.format) > 0)
				continue;
			if(song.game != "" && song.game == lastSong.game &&
			   song.composer == lastSong.composer) {
				// Keep adding songs of the same game to lastSong
				if(!startsWith(lastSong.path, "MULTI:")) {
					lastSong.path = string("MULTI:") + lastSong.path;
					lastSong.title = "";
				}
				lastSong.path = lastSong.path + "\t" + song.path;
				continue;
			} else {
				// song is not the same as lastSong, commit lastSong
				if(lastSong.path != "")
					callback(lastSong);
				lastSong = song;
			}
		}
	}
	if(lastSong.path != "")
		callback(lastSong);
	return true;
}

bool MusicDatabase::parseStandard(Variables &vars, const std::string &listFile,
                                  std::function<void(const SongInfo &)> callback) {

	const char *metadata = nullptr;
	int pi = 4, gi = 1, ti = 0, ci = 2, fi = 3, mi = 5;
	auto templ = vars["song_template"];
	auto format = vars["format"];
	auto composer = vars["composer"];
	int columns = 2;
	if(templ != "") {
		fi = gi = ci = -1;
		int i = 0;
		for(const auto &p : split(templ)) {
			if(p == "title")
				ti = i;
			else if(p == "composer")
				ci = i;
			else if(p == "path")
				pi = i;
			else if(p == "format")
				fi = i;
			else if(p == "game")
				gi = i;
			i++;
		}
		columns = i;
		LOGD("TEMPLATE '%s' -> %d %d %d", templ, pi, ti, ci);
	}

	bool isUtf8 = vars["utf8"] == "no" ? false : true;
	auto source = vars["source"];

	File f{listFile};

	for(const auto &s : f.getLines()) {
		auto parts = isUtf8 ? split(s, "\t") : split(utf8_encode(s), "\t");
		if(parts.size() >= columns) {

			SongInfo song;
			string metadata;

			// Strip sorce from path if necessary
			if(source != "" && parts[pi].find(source) == 0)
				parts[pi] = parts[pi].substr(source.length());
			if(ti > 0)
				parts[ti] = htmldecode(parts[ti]);
			if(gi > 0)
				parts[gi] = htmldecode(parts[gi]);
			if(ci > 0)
				parts[ci] = htmldecode(parts[ci]);

			if(parts.size() > mi)
				metadata = parts[mi];

			song = SongInfo(parts[pi], gi >= 0 ? parts[gi] : "", parts[ti],
			                ci >= 0 ? parts[ci] : composer, fi <= 0 ? format : parts[fi], metadata);
			callback(song);
		}
	}
	return true;
}

void MusicDatabase::initDatabase(const std::string &workDir, Variables &vars) {

	auto id = vars["id"];
	auto type = vars["type"];
	if(type == "")
		type = id;
	auto name = vars["name"];
	auto source = vars["source"];
	auto local_dir = vars["local_dir"];
	auto song_list = vars["song_list"];
	auto prod_list = vars["prod_list"];
	auto remote_list = vars["remote_list"];
	auto description = vars["description"];

	LOGD("Checking %s", name);

	// Return if this collection has already been indexed in this version
	auto cq = db.query<uint64_t>("SELECT ROWID FROM collection WHERE id = ?", id);
	if(cq.step()) {
		return;
	}
	cq.finalize();

	reindexNeeded = true;

	if(local_dir != "") {
		if(endsWith(local_dir, "/"))
			local_dir += "/";
		if(local_dir[0] != '/')
			local_dir = workDir + "/" + local_dir;
	}

	print_fmt("Creating '%s' database\n", name);

	db.exec("BEGIN TRANSACTION");
	db.exec("INSERT INTO collection (name, id, url, localdir, description) VALUES (?, ?, ?, ?, ?)",
	        name, id, source, local_dir, description);
	auto collection_id = db.last_rowid();
	dontIndex.resize(collection_id + 1);
	dontIndex[collection_id] = 0;

	if(vars["index"] == "no") {
		LOGD("Not indexing %s/%d", id, collection_id);
		dontIndex[collection_id] = 1;
	}

	LOGD("Workdir:%s", workDir);
	File listFile;
	bool writeListFile = false;
	webutils::Web web{File::getCacheDir() / "_webfiles"};

	bool prodCollection = false;

	if(prod_list != "") {
		song_list = prod_list;
		prodCollection = true;
	}

	if(song_list == "")
		song_list = remote_list;

	if(startsWith(song_list, "http://")) {
		listFile = web.getFileBlocking(song_list);
	} else if(song_list != "") {
		listFile = File(workDir, song_list);
		writeListFile = !listFile.exists();
	}

	if(prodCollection) {

		auto query = db.query("INSERT INTO product (title, creator, type, screenshots, collection) "
		                      "VALUES (?, ?, ?, ?, ?)");

		auto query2 = db.query("INSERT INTO prod2song (prodid, songid) "
		                       "VALUES (?, ?)");

		unordered_map<string, ParseProdFun> parsers = {
		    {"bitworld", &MusicDatabase::parseBitworld}, {"csdb", &MusicDatabase::parseCsdb},
		};

		auto parser = parsers[type];
		// if(!parser)
		// parser = &MusicDatabase::parseStandard;

		(this->*parser)(listFile, [&](const Product &prod) {
			query.bind(prod.title, prod.creator, prod.type, prod.screenshots, collection_id).step();
			auto prodrow = db.last_rowid();
			for(string path : prod.songs) {
				// TODO: Move to CORRECTIONS.LUA or something
				auto pos = path.find("Zombie (FI)");
				if(pos != string::npos)
					path = path.substr(0, pos) + "Naksahtaja" + path.substr(pos + 11);
				uint64_t hash = MD5::hash(path);
				auto it = pathMap.find(hash);
				if(it == pathMap.end()) {
					LOGD("PATH '%s' not found", path);
				} else {
					auto songrow = it->second;
					query2.bind(prodrow, songrow).step();
				}
			}
		});

	} else {
		auto query =
		    db.query("INSERT INTO song (title, game, composer, format, path, collection, metadata) "
		             "VALUES (?, ?, ?, ?, ?, ?, ?)");

		if(File::exists(listFile)) {

			unordered_map<string, ParseSongFun> parsers = {
			    {"pouet", &MusicDatabase::parseStandard},    {"amp", &MusicDatabase::parseAmp},
			    {"modland", &MusicDatabase::parseModland},   {"podcast", &MusicDatabase::parseRss},
			    {"standard", &MusicDatabase::parseStandard},
			};

			auto parser = parsers[type];
			if(!parser)
				parser = &MusicDatabase::parseStandard;

			(this->*parser)(vars, listFile, [&](const SongInfo &song) {
				query.bind(song.title, song.game, song.composer, song.format, song.path,
				           collection_id, song.metadata[SongInfo::INFO] != ""
				                              ? song.metadata[SongInfo::INFO].c_str()
				                              : nullptr)
				    .step();
				auto last = db.last_rowid();
				auto hash = MD5::hash(song.path);
				pathMap[hash] = last;

			});

		} else if(File::exists(local_dir)) {

			File root{local_dir};
			LOGD("Checking local dir '%s'", root.getName());
			for(auto &rf : root.listRecursive()) {
				auto name = rf.getName();
				LOGD("Parsing %s", name);
				SongInfo songInfo(name);
				if(identify_song(songInfo)) {

					auto pos = name.find(local_dir);
					if(pos == 0) {
						name = name.substr(local_dir.length());
					}

					query.bind(songInfo.title, songInfo.game, songInfo.composer, songInfo.format,
					           name, collection_id, (char *)nullptr)
					    .step();
					if(writeListFile)
						listFile.writeln(join("\t", songInfo.title, songInfo.game,
						                      songInfo.composer, songInfo.format, name));
				}
			}
		}
	}

	listFile.close();
	db.exec("COMMIT");
}

void MusicDatabase::setFilter(const std::string &collection, int type) {

	if(collection == "") {
		titleIndex.setFilter();
		collectionFilter = -1;
	} else {
		LOGD("FILTER: '%s'", collection);
		auto cq = db.query<int>("SELECT ROWID FROM collection WHERE id = ?", collection);
		if(cq.step()) {
			collectionFilter = cq.get();
			LOGD("ID %d from %s", collectionFilter, collection);
			// collectionFilter = 2;
			titleIndex.setFilter([=](int index) {
				auto f = formats[index];
				if(type == 1 && (f & 0xff) == PRODUCT)
					return false;
				return ((formats[index] >> 8) != collectionFilter);
			});
		}
	}
}

int MusicDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

	lock_guard<mutex>{dbMutex};

	result.resize(0);

	string title_query = query;
	string composer_query = query;

	auto p = split(query, "/");
	if(p.size() > 1) {
		title_query = p[0];
		composer_query = p[1];
	}

	// For empty query, return all playlists
	if(query == "") {
		for(int i = 0; i < playLists.size(); i++) {
			result.push_back(PLAYLIST_INDEX + i);
		}
		return result.size();
	}

	// Push back all matching playlists
	for(int i = 0; i < playLists.size(); i++) {
		if(toLower(playLists[i].name).find(query) != string::npos)
			result.push_back(PLAYLIST_INDEX + i);
	}

	titleIndex.search(title_query, result, searchLimit);

	if(result.size() >= searchLimit)
		return searchLimit;

	searchLimit -= result.size();

	vector<int> cresult;
	composerIndex.search(composer_query, cresult, searchLimit);
	for(int index : cresult) {
		int offset = composerTitleStart[index];
		while(composerToTitle[offset] != -1) {
			if(result.size() >= searchLimit)
				break;
			int songindex = composerToTitle[offset++];

			if(collectionFilter == -1 || (formats[songindex] >> 8) == collectionFilter)
				result.push_back(songindex);
		}
		if(result.size() >= searchLimit)
			break;
	}

	return result.size();
}

// Lookup the given path in the database
SongInfo &MusicDatabase::lookup(SongInfo &song) {

	lock_guard<mutex>{dbMutex};
	auto path = song.path;

	auto parts = split(path, "::");
	if(parts.size() > 1) {
		path = parts[1];
		if(parts[0] == "index") {
			int index = stol(path);
			SongInfo song = getSongInfo(index);
			path = song.path;
			parts = split(path, "::");
			if(parts.size() > 1) {
				path = parts[1];
			}
		}
		LOGD("INDEX %s %s", parts[0], path);
	}

	auto q = db.query<string, string, string, string, string, string, string>(
	    "SELECT path, title, game, composer, format, collection.id, metadata FROM song, collection "
	    "WHERE song.collection = collection.ROWID AND song.path = ?",
	    path);

	if(q.step()) {
		string coll;
		tie(song.path, song.title, song.game, song.composer, song.format, coll,
		    song.metadata[SongInfo::INFO]) = q.get_tuple();
		song.path = coll + "::" + song.path;
		LOGD("LOOKUP '%s' became '%s'", path, song.path);
	} else {
		LOGD("TODO: Check products");
	}

	return song;
}

SongInfo MusicDatabase::getSongInfo(int id) const {

	if(id >= PLAYLIST_INDEX) {
		string p = playLists[id - PLAYLIST_INDEX].name;
		File path = File::getConfigDir() / "playlists" / p;
		return SongInfo("playlist::" + path.getName(), "", p, "", "Local playlist");
	}

	id++;
	LOGD("ID %d vs PROD %d", id, productStartIndex);
	if(id >= productStartIndex) {
		id -= productStartIndex;
		auto q = db.query<string, string, string, string, string>(
		    "SELECT title, creator, type, collection.id, metadata "
		    "FROM  product, collection "
		    "WHERE product.ROWID = ? AND product.collection = collection.ROWID",
		    id);
		if(q.step()) {
			SongInfo song;
			string collection;
			tie(song.title, song.composer, song.format, collection, song.metadata[SongInfo::INFO]) =
			    q.get_tuple();
			song.path = "product::" + to_string(id);
			return song;
		}

	} else {

		auto q = db.query<string, string, string, string, string, string, string>(
		    "SELECT title, game, composer, format, song.path, collection.id, metadata "
		    "FROM song, collection "
		    "WHERE song.ROWID = ? AND song.collection = collection.ROWID",
		    id);
		if(q.step()) {
			SongInfo song;
			string collection;
			tie(song.title, song.game, song.composer, song.format, song.path, collection,
			    song.metadata[SongInfo::INFO]) = q.get_tuple();
			if(collection == "pouet") {
				song.metadata[SongInfo::SCREENSHOT] =
				    string("http://content.pouet.net/files/screenshots/") +
				    song.metadata[SongInfo::INFO];
				song.metadata[SongInfo::INFO] = "";
			}
			song.path = collection + "::" + song.path;
			return song;
		}
	}
	throw not_found_exception();
}
std::string MusicDatabase::getSongScreenshots(SongInfo &s) {

	lookup(s);
	auto parts = split(s.path, "::");
	if(parts.size() < 2)
		return "";
	string shot;
	LOGD("Collection '%s'", parts[0]);
	if(s.metadata[SongInfo::SCREENSHOT] != "") {
		shot = s.metadata[SongInfo::SCREENSHOT];
	} else {
		auto q = db.query<string, string, string>(
		    "SELECT product.screenshots, product.type, collection.id "
		    "FROM product, prod2song, song, collection "
		    "WHERE product.rowid = prod2song.prodid AND prod2song.songid = song.ROWID AND "
		    "product.collection = collection.ROWID AND song.path = ?",
		    parts[1]);
		string collection, format;
		while(q.step()) {
			tie(shot, format, collection) = q.get_tuple();
			LOGD("Collection %s Format %s", collection, format);
			if(format.find("Demo") != string::npos || format.find("Trackmo") != string::npos)
				break;
		}
		if(shot != "") {
			string prefix = "";
			if(collection == "pouet")
				prefix = "http://content.pouet.net/files/screenshots/";
			else if(collection == "bitworld")
				prefix = "http://kestra.exotica.org.uk/files/screenies/";
			auto parts = split(shot, ";");
			for(auto &p : parts) {
				p = prefix + p;
			}
			shot = join(parts, ";");
		}
	}
	return shot;
}

std::string MusicDatabase::getProductScreenshots(uint32_t id) {
	std::vector<std::string> shots;
	auto q = db.query<string, string>(
	    "SELECT collection.id,screenshots "
	    "FROM product, collection "
	    "WHERE product.rowid = ? AND collection.ROWID = product.collection",
	    id);

	string screenshot;
	string collection;

	if(q.step()) {
		tie(collection, screenshot) = q.get_tuple();
		string prefix = "";
		if(collection == "pouet")
			prefix = "http://content.pouet.net/files/screenshots/";
		else if(collection == "bitworld")
			prefix = "http://kestra.exotica.org.uk/files/screenies/";
		auto parts = split(screenshot, ";");
		for(auto &p : parts) {
			p = prefix + p;
		}
		return join(parts, ";");
	}
	return "";
}

std::vector<SongInfo> MusicDatabase::getProductSongs(uint32_t id) {
	std::vector<SongInfo> songs;
	auto screenshot = getProductScreenshots(id);
	auto q = db.query<string, string, string, string, string, string, string>(
	    "SELECT title, game, composer, format, song.path, collection.id, metadata "
	    "FROM song, prod2song, collection "
	    "WHERE prodid = ? AND songid = song.ROWID AND song.collection = collection.ROWID",
	    id);

	while(q.step()) {
		SongInfo song;
		string collection;
		tie(song.title, song.game, song.composer, song.format, song.path, collection,
		    song.metadata[SongInfo::INFO]) = q.get_tuple();
		song.path = collection + "::" + song.path;
		song.metadata[SongInfo::SCREENSHOT] = screenshot;
		songs.push_back(song);
	}
	return songs;
}

#include "formats.h"

static std::unordered_map<std::string, uint8_t> format_map;

void initFormats() {
	for(const char *f : uade_formats) {
		format_map[f] = UADE;
	}
	for(const char *f : adlib_formats) {
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

static uint8_t formatToByte(const std::string &fmt, const std::string &path, int coll) {

	static bool init = false;
	if(!init) {
		initFormats();
		init = true;
	}

	string f = toLower(fmt);
	uint8_t l = format_map[f];
	if(l == 0) {

		l = UNKNOWN_FORMAT;

		if((path.find("youtube.com/") != string::npos) ||
		   (path.find("youtu.be/") != string::npos)) {
			return YOUTUBE;
		}

		if(endsWith(f, "tracker"))
			l = TRACKER;
		if(startsWith(f, "protracker"))
			l = PROTRACKER;
		else if(startsWith(f, "fasttracker"))
			l = FASTTRACKER;
		else if(startsWith(f, "impulsetracker"))
			l = IMPULSETRACKER;
		else if(startsWith(f, "screamtracker"))
			l = SCREAMTRACKER;
		else if(startsWith(f, "atari"))
			l = ATARI;
		else if(startsWith(f, "ay ") || startsWith(f, "spectrum "))
			l = SPECTRUM;
		else if(startsWith(f, "gameboy"))
			l = GAMEBOY;
		if(f.find("megadrive") != string::npos)
			l = MEGADRIVE;
		if(l != UNKNOWN_FORMAT)
			format_map[f] = l;
		// fprintf(stderr, "%s\n", f.c_str());
	}
	return l;
}

template <typename T> static void readVector(std::vector<T> &v, File &f) {
	auto sz = f.read<uint32_t>();
	v.resize(sz);
	f.read((uint8_t *)&v[0], v.size() * sizeof(T));
}

template <typename T> static void writeVector(std::vector<T> &v, File &f) {
	f.write<uint32_t>(v.size());
	f.write((uint8_t *)&v[0], v.size() * sizeof(T));
}

void MusicDatabase::readIndex(File &f) {

	indexVersion = 0;
	auto marker = f.read<uint16_t>();
	if(marker == 0xFEDC)
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

void MusicDatabase::writeIndex(File &f) {
	f.write<uint16_t>(0xFEDC);
	f.write<uint16_t>(dbVersion);
	f.write<uint32_t>(productStartIndex);
	writeVector(titleToComposer, f);
	writeVector(composerToTitle, f);
	writeVector(composerTitleStart, f);
	writeVector(formats, f);

	titleIndex.dump(f);
	composerIndex.dump(f);
}

void MusicDatabase::generateIndex() {

	lock_guard<mutex>{dbMutex};

	RemoteLoader &loader = RemoteLoader::getInstance();
	auto q = db.query<int, string, string, string>("SELECT ROWID,id,url,localdir FROM collection");
	while(q.step()) {
		auto c = q.get<Collection>();
		// NOTE c.name is really c.id
		loader.registerSource(c.name, c.url, c.local_dir);
	}

	File f{File::getCacheDir() / "index.dat"};

	if(!reindexNeeded && f.exists()) {
		readIndex(f);
		f.close();
		return;
	}

	print_fmt("Creating Search Index...\n");

	string oldComposer;
	auto query = db.query<string, string, string, string, string, int>(
	    "SELECT title, game, format, composer, path, collection FROM song");

	int count = 0;
	// int maxTotal = 3;
	int cindex = 0;

	const int approxSongs = 340000;

	titleToComposer.reserve(approxSongs);
	composerToTitle.reserve(37000);
	titleIndex.reserve(approxSongs);
	composerIndex.reserve(37000);
	formats.reserve(approxSongs);

	int step = approxSongs / 20;

	unordered_map<string, vector<uint32_t>> composers;

	string title, game, fmt, composer, path;
	int collection;

	while(count < 1000000) {
		count++;
		if(!query.step())
			break;

		if(count % step == 0) {
			LOGD("%d songs indexed", count);
		}

		tie(title, game, fmt, composer, path, collection) = query.get_tuple();

		uint8_t b = formatToByte(fmt, path, collection);
		formats.push_back(b | (collection << 8));

		if(game != "") {
			if(title != "")
				title = format("%s [%s]", game, title);
			else
				title = game;
		}

		if(dontIndex[collection]) {
			title = "";
			composer = "";
		}

		// The title index maps one-to-one with the database
		int tindex = titleIndex.add(title);

		auto &v = composers[composer];
		if(v.size() == 0) {
			cindex = composerIndex.add(composer);
			composers[composer].push_back(cindex);
		} else
			cindex = composers[composer][0];

		composers[composer].push_back(tindex);

		// We also need to find the composer for a give title
		titleToComposer.push_back(cindex);
	}

	productStartIndex = titleIndex.size();

	auto prodQuery = db.query<string, string, string, int>(
	    "SELECT title, type, creator, collection FROM product");
	while(count < 1000000) {
		count++;
		if(!prodQuery.step())
			break;

		if(count % step == 0) {
			LOGD("%d songs indexed", count);
		}

		tie(title, fmt, composer, collection) = prodQuery.get_tuple();

		uint8_t b = PRODUCT;
		formats.push_back(b | (collection << 8));

		// The title index maps one-to-one with the database
		int tindex = titleIndex.add(title);

		auto &v = composers[composer];
		if(v.size() == 0) {
			cindex = composerIndex.add(composer);
			composers[composer].push_back(cindex);
		} else
			cindex = composers[composer][0];

		composers[composer].push_back(tindex);

		// We also need to find the composer for a give title
		titleToComposer.push_back(cindex);
	}

	// composers[name] -> vector of titleindexes for each composer.

	LOGD("Found %d composers and %d titles", composers.size(), titleToComposer.size());

	composerTitleStart.resize(composers.size());
	for(const auto &p : composers) {
		// p,first == composer, p.second == vector
		auto cindex = p.second[0];
		composerTitleStart[cindex] = composerToTitle.size();
		for(int i = 1; i < (int)p.second.size(); i++)
			composerToTitle.push_back(p.second[i]);
		composerToTitle.push_back(-1);
	}

	writeIndex(f);
	f.close();

	reindexNeeded = false;
}

void MusicDatabase::initFromLuaAsync(const File &workDir) {
	indexing = true;
	initFuture = std::async(std::launch::async, [=]() {
		std::lock_guard<std::mutex>{dbMutex};
		if(!initFromLua(workDir)) {
		}
		std::lock_guard<std::mutex>{chkMutex};
		indexing = false;
	});
}

bool MusicDatabase::initFromLua(const File &workDir) {

	File playlistDir{File::getConfigDir() / "playlists"};
	makedir(playlistDir);
	bool favFound = false;
	for(auto f : playlistDir.listFiles()) {
		playLists.emplace_back(f);
		if(playLists.back().name == "Favorites")
			favFound = true;
	}
	if(!favFound) {
		playLists.emplace_back(playlistDir / "Favorites");
		playLists.back().save();
	}

	reindexNeeded = false;

	File fi{File::getCacheDir() / "index.dat"};

	indexVersion = 0;
	if(fi.exists()) {
		auto marker = fi.read<uint16_t>();
		if(marker == 0xFEDC)
			indexVersion = fi.read<uint16_t>();
	}

	LuaInterpreter lua;

	lua.registerFunction("set_db_var", [&](string name, string val) {
		static unordered_map<string, string> dbmap;
		if(val == "start") {
		} else if(val == "end") {
			initDatabase(workDir, dbmap);
			dbmap.clear();
		} else {
			dbmap[name] = val;
		}
	});

	File f = File::findFile(workDir, "lua/db.lua");

	if(!lua.loadFile(f.getName())) {
		LOGE("Could not load db.lua");
		return false;
	}

	dbVersion = lua.getGlobal<int>("VERSION");
	LOGD("DBVERSION %d INDEXVERSION %d", dbVersion, indexVersion);
	if(dbVersion != indexVersion) {
		db.exec("DROP TABLE IF EXISTS collection");
		db.exec("DROP TABLE IF EXISTS song");
		db.exec("DROP TABLE IF EXISTS product");
		db.exec("DROP TABLE IF EXISTS prod2song");
		createTables();
		reindexNeeded = true;
	}

	lua.load(R"(
		for a,b in pairs(DB) do
			if type(b) == 'table' then
				set_db_var(a, 'start')
				for a1,b1 in pairs(b) do
					set_db_var(a1, b1)
				end
				set_db_var(a, 'end')
			end
		end
	)");
	generateIndex();
	return true;
}

int MusicDatabase::getSongs(std::vector<SongInfo> &target, const SongInfo &match, int limit,
                            bool random) {

	lock_guard<mutex>{dbMutex};
	string txt = "SELECT path, game, title, composer, format, collection.id FROM song, collection "
	             "WHERE song.collection = collection.ROWID";

	string collection = "";
	if(match.path != "") {
		auto parts = split(match.path, "::");
		if(parts.size() >= 2)
			collection = parts[0];
	}

	if(match.format != "")
		txt += " AND format=?";
	if(match.composer != "")
		txt += " AND composer=?";
	if(collection != "")
		txt += " AND collection.id=?";
	if(random)
		txt += " ORDER BY RANDOM()";
	if(limit > 0)
		txt += format(" LIMIT %d", limit);

	LOGD("SQL:%s", txt);

	auto q = db.query<string, string, string, string, string, string>(txt);
	int index = 1;
	if(match.format != "")
		q.bind(index++, match.format);
	if(match.composer != "")
		q.bind(index++, match.composer);
	if(collection != "")
		q.bind(index++, collection);

	while(q.step()) {
		string collection;
		SongInfo song;
		tie(song.path, song.game, song.title, song.composer, song.format, collection) =
		    q.get_tuple();
		song.path = collection + "::" + song.path;
		if(song.game != "")
			song.title = utils::format("%s [%s]", song.game, song.title);
		target.push_back(song);
	}
	return 0;
}

void MusicDatabase::addToPlaylist(const std::string &plist, const SongInfo &song) {
	for(auto &pl : playLists) {
		if(pl.name == plist) {
			pl.songs.push_back(song);
			pl.save();
			break;
		}
	}
}

void MusicDatabase::removeFromPlaylist(const std::string &plist, const SongInfo &toRemove) {
	for(auto &pl : playLists) {
		if(pl.name == plist) {
			pl.songs.erase(
			    std::remove_if(pl.songs.begin(), pl.songs.end(), [&](const SongInfo &song) -> bool {
				    return song.path == toRemove.path &&
				           (song.starttune == -1 || song.starttune == toRemove.starttune);
				}), pl.songs.end());
			pl.save();
			break;
		}
	}
}

std::vector<SongInfo> &MusicDatabase::getPlaylist(const std::string &plist) {
	static std::vector<SongInfo> empty;
	for(auto &pl : playLists) {
		if(pl.name == plist)
			return pl.songs;
	}
	return empty;
}
}
