#include "MusicDatabase.h"
#include "SongFileIdentifier.h"
#include "RemoteLoader.h"

#include <luainterpreter/luainterpreter.h>
#include <coreutils/utils.h>
#include <archive/archive.h>
#include <set>

using namespace std;
using namespace utils;

namespace chipmachine {

static const std::set<std::string> secondary = { "smpl", "sam", "ins", "smp" };

#ifdef RASPBERRYPI
static std::unordered_set<std::string> exclude = {
	"Nintendo DS Sound Format", "Gameboy Sound Format", "Dreamcast Sound Format", "Ultra64 Sound Format",
	"Saturn Sound Format" /*, "RealSID", "PlaySID" */
};
#else
static std::unordered_set<std::string> exclude = { /*"RealSID", "PlaySID",*/ "Saturn Sound Format" };
#endif


void MusicDatabase::initDatabase(unordered_map<string, string> &vars) {

	auto type = vars["type"];
	LOGD("Init db '%s'", type);
	auto source = vars["source"];
	auto local_dir = vars["local_dir"];
	auto song_list = vars["song_list"];
	auto description = vars["description"];
	auto xformats = vars["exclude_formats"];
	auto id = stol(vars["id"]);

	// Return if this collection has already been indexed
	if(db.query("SELECT 1 FROM collection WHERE name = ?", type).step())
		return;

	reindexNeeded = true;

	auto ex_copy = exclude;
	auto parts = split(xformats, ";");
	for(const auto &p : parts) {
		if(p.length())
			ex_copy.insert(p);
	}

	print_fmt("Creating '%s' database\n", type);

	db.exec("BEGIN TRANSACTION");

	if(!endsWith(local_dir, "/"))
		local_dir += "/";

	db.exec("INSERT INTO collection (name, url, localdir, description, id) VALUES (?, ?, ?, ?, ?)",
		type, source, local_dir, description, id);

	auto query = db.query("INSERT INTO song (title, game, composer, format, path, collection) VALUES (?, ?, ?, ?, ?, ?)");

	auto path = current_exe_path() + ":" + File::getAppDir();
	File listFile = File::findFile(path, song_list);
	//File listFile(song_list);

	if(listFile.exists()) {

		bool isModland = (type == "modland");
		bool isRKO = (type == "rko");
		bool isAmiRemix = (type == "amigaremix");

		for(const auto &s : listFile.getLines()) {
			auto parts = split(s, "\t");
			if(parts.size() >= 2) {
				if(isModland) {
					SongInfo song(parts[1]);
					if(!parseModlandPath(song))
						continue;
					if(ex_copy.count(song.format) > 0)
						continue;
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				} else if(isRKO) {
					parts[3] = htmldecode(parts[3]);
					parts[4] = htmldecode(parts[4]);
					SongInfo song(parts[0], "", parts[3], parts[4], "MP3");
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				} else if(isAmiRemix) {
					if(parts[0].find(source) == 0)
						parts[0] = parts[0].substr(source.length());
					SongInfo song(parts[0], "", parts[2], parts[3], "MP3");
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				} else {
					SongInfo song(parts[4], parts[1], parts[0], parts[2], parts[3]);
					query.bind(song.title, song.game, song.composer, song.format, song.path, id);
				}
				query.step();
			}
		}
	} else {

		makedir(".rsntemp");
		function<void(File &)> checkDir;
		checkDir = [&](File &root) {
			for(auto &rf : root.listFiles()) {
				if(rf.isDir()) {
					checkDir(rf);
				} else {
					auto name = rf.getName();
					SongInfo songInfo(name);
					if(identify_song(songInfo)) {

						auto pos = name.find(local_dir);
						if(pos != string::npos) {
							name = name.substr(pos + local_dir.length());
						}

						query.bind(songInfo.title, songInfo.game, songInfo.composer, songInfo.format, name, id);
						query.step();
					}
				}
			}
		};
		File root { local_dir };
		checkDir(root);
	}
	db.exec("COMMIT");
}

bool MusicDatabase::parseModlandPath(SongInfo &song) {

	static const unordered_set<string> hasSubFormats = {
		"Ad Lib",
		"Video Game Music"
	};

	string ext = path_extension(song.path);
	if(secondary.count(ext) > 0) {
		return false;
	}

	auto parts = split(song.path, "/");
	int l = parts.size();
	if(l < 3) {
		LOGD("%s", song.path);
		return false;
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

	//string game;
	if(l-i >= 2)
		song.game = parts[i++];

	if(i == l) {
		LOGD("Bad file %s", song.path);
		return false;
	}

	if(endsWith(parts[i], ".rar"))
		parts[i] = parts[i].substr(0, parts[i].length()-4);

	song.title = path_basename(parts[i++]);

	return true;
}

int MusicDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

	lock_guard<mutex>{dbMutex};

	result.resize(0);
	//if(query.size() < 3)
	//	return 0;
	//bool q3 = (q.size() <= 3);
	string title_query = query;
	string composer_query = query;

	auto p = split(query, "/");
	if(p.size() > 1) {
		title_query = p[0];
		composer_query = p[1];
	}

	//titleIndex.setFilter([&](int index) {
	//	return ((formats[index] & 0xff) != C64);
	//});

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
			//if((formats[songindex] & 0xff) != C64)
			//	continue;
			result.push_back(songindex);
		}
		if(result.size() >= searchLimit)
			break;
	}

	int sz = result.size();
	int j = sz;
	/*
	for(int i=0, j=0; i<sz; i++) {
		if(formats[result[i]] == 0x600) {
			result[j++] = result[i];
		}
	}
	result.resize(j);
	*/
	return j;
}

SongInfo MusicDatabase::lookup(const std::string &p) {
	auto path = p;

	auto parts = split(path, "::");
	if(parts.size() > 1)
		path = parts[1];
	else {
		parts = split(path, ":");
		if(parts[1] == "modland" || parts[1] == "hvsc")
			path = parts[0];
	}

	auto q = db.query<string, string, string, string, string>("SELECT title, game, composer, format, collection.name FROM song, collection WHERE song.path=? AND song.collection = collection.id", path);

	SongInfo song;
	if(q.step()) {
		string coll;
		LOGD("Found %s in %s", song.title, path);
		tie(song.title, song.game, song.composer, song.format, coll) = q.get_tuple();
		song.path = coll + "::"+ path;
	}
	return song;
}

SongInfo MusicDatabase::getSongInfo(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string, string, string>("SELECT title, game, composer, format, song.path, collection.name FROM song, collection WHERE song.ROWID = ? AND song.collection = collection.id", id);
	if(q.step()) {
		//string title, game, composer, format, path, collection;
		SongInfo song;
		string collection;
		tie(song.title, song.game, song.composer, song.format, song.path, collection) = q.get_tuple();

		song.path = collection + "::" + song.path;

		if(song.game != "")
			song.title = utils::format("%s [%s]", song.game, song.title);

		//string r = utils::format("%s\t%s\t%s\t%s", path, title, composer, format);
		//LOGD("RESULT %s", r);
		//return r;
		return song;
	}
	throw not_found_exception();
}

const char *adlib_formats[] = {
	"adl",
	"adlib tracker 2",
	"adlib tracker 2 (v9 - v11)",
	"adlib tracker",
	"amusic",
	"amusic xms",
	"apogee",
	"beni tracker",
	"bob's adlib music",
	"boom tracker 4.0",
	"creative music file",
	"defy adlib tracker",
	"digital fm",
	"dosbox",
	"drum traker",
	"edlib",
	"exotic adlib",
	"extra simple music",
	"faust music creator",
	"fm-kingtracker",
	"fm tracker",
	"herad music system agd",
	"herad music system sdb",
	"herad music system sqx",
	"hsc adlib composer",
	"jch-d00",
	"jch-d01",
	"johannes bjerregard module",
	"ken's adlib music",
	"loudness sound system",
	"lucasarts",
	"martin fernandez",
	"master tracker",
	"mk-jamz",
	"mlat adlib tracker",
	"mpu-401 trakker",
	"mus",
	"palladix",
	"pixel painters",
	"rdosplay raw",
	"reality adlib tracker",
	"shadowlands",
	"sierra",
	"sng player",
	"sound images generation 2",
	"sound interface system",
	"surprise! adlib tracker 2.0",
	"surprise! adlib tracker",
	"twin trackplayer",
	"ultima 6",
	"vibrants",
	"visual composer",
};

const char *uade_formats[] = {
	"actionamics",
	"activision pro",
	"aero studio",
	"ahx",
	"all sound tracker",
	"am composer",
	"anders oland",
	"and xsynth",
	"aprosys",
	"arkostracker",
	"art and magic",
	"art of noise",
	"asylum",
	"atari digi-mix",
	"athtune",
	"audio sculpture",
	"axs",
	"beathoven synthesizer",
	"beaver sweeper",
	"beepola",
	"ben daglish",
	"ben daglish sid",
	"berotracker",
	"boyscout",
	"bp soundmon 2",
	"bp soundmon 3",
	"buzz",
	"buzzic 1.0",
	"buzzic 1.1",
	"buzzic 2.0",
	"cba",
	"composer 669",
	"compoz",
	"core design",
	"cubic tiny xm",
	"custommade",
	"cybertracker",
	"cybertracker c64",
	"darius zendeh",
	"darkwave studio",
	"dave lowe",
	"dave lowe new",
	"david hanney",
	"david whittaker",
	"delitracker custom",
	"delta music 2",
	"delta music",
	"delta packer",
	"desire",
	"digibooster",
	"digibooster pro",
	"digital-fm music",
	"digital mugician 2",
	"digital mugician",
	"digital sonix and chrome",
	"digital sound and music interface",
	"digital sound interface kit",
	"digital sound interface kit riff",
	"digital sound studio",
	"digital tracker dtm",
	"digital tracker mod",
	"digitrakker",
	"digitrekker",
	"dirk bialluch",
	"disorder tracker 2",
	"dreamstation",
	"dynamic studio professional",
	"dynamic synthesizer",
	"earache",
	"electronic music system",
	"electronic music system v6",
	"epic megagames masi",
	"extreme tracker",
	"face the music",
	"famitracker",
	"farandole composer",
	"fashion tracker",
	"follin player ii",
	"forgotten worlds",
	"fred gray",
	"fredmon",
	"fuchstracker",
	"funktracker",
	"future composer 1.3",
	"future composer 1.4",
	"future composer bsi",
	"future player",
	"game music creator",
	"general digimusic",
	"gluemon",
	"goattracker 2",
	"goattracker",
	"goattracker stereo",
	"graoumf tracker 2",
	"graoumf tracker",
	"gt game systems",
	"hes",
	"hippel 7v",
	"hippel-atari",
	"hippel-coso",
	"hippel",
	"hippel-st",
	"hivelytracker",
	"howie davies",
	"images music system",
	"imago orpheus",
	"instereo! 2.0",
	"instereo!",
	"ixalance",
	"jamcracker",
	"janko mrsic-flogel",
	"jason brooke",
	"jason page",
	"jason page old",
	"jaytrax",
	"jeroen tel",
	"jesper olsen",
	"ken's digital music",
	"klystrack",
	"kris hatlelid",
	"kss",
	"leggless music editor",
	"lionheart",
	"liquid tracker",
	"mad tracker 2",
	"magnetic fields packer",
	"maniacs of noise",
	"maniacs of noise old",
	"mark cooksey",
	"mark cooksey old",
	"mark ii",
	"maxtrax",
	"mcmd",
	"mdx",
	"medley",
	"megastation",
	"megastation midi",
	"megatracker",
	"mike davies",
	"monotone",
	"multimedia sound",
	"music assembler",
	"music editor",
	"musicline editor",
	"musicmaker",
	"musicmaker v8",
	"mvs tracker",
	"mvx module",
	"nerdtracker 2",
	"noisetrekker 2",
	"noisetrekker",
	"novotrade packer",
	"octalyser",
	"octamed mmd0",
	"octamed mmd1",
	"octamed mmd2",
	"octamed mmd3",
	"octamed mmdc",
	"oktalyzer",
	"onyx music file",
	"organya 2",
	"organya",
	"paul robotham",
	"paul shields",
	"paul summers",
	"peter verswyvelen",
	"picatune2",
	"picatune",
	"pierre adane packer",
	"piston collage",
	"playerpro",
	"pmd",
	"pokeynoise",
	"pollytracker",
	"polytracker",
	"powertracker",
	"professional sound artists",
	"protracker",
	"protracker 3.6",
	"protrekkr 2.0",
	"protrekkr",
	"psycle",
	"pumatracker",
	"quadra composer",
	"quartet psg",
	"quartet st",
	"ramtracker",
	"real tracker",
	"renoise 1.1.1 (rns05)",
	"renoise 1.2.7 (rns15)",
	"renoise 1.2.8.1 (rns16)",
	"renoise 1.5.0 (rns17)",
	"renoise 1.5.1 (rns18)",
	"renoise 1.8 (ver04)",
	"renoise 1.9.1 (ver10)",
	"renoise 1.9 (ver09)",
	"renoise 2.0 (ver14)",
	"renoise 2.1 (ver15)",
	"renoise 2.5 (ver21)",
	"renoise 2.6 (ver22)",
	"renoise 2.7 (ver30)",
	"renoise 2.8 (ver37)",
	"richard joseph",
	"riff raff",
	"rob hubbard 2",
	"rob hubbard",
	"rob hubbard st",
	"ron klaren",
	"sam coupe cop",
	"sam coupe sng",
	"sbstudio",
	"scumm",
	"sean connolly",
	"sean conran",
	"shroom",
	"sidmon 1",
	"sidmon 2",
	"sidplayer",
	"silmarils",
	"skale tracker",
	"sonic arranger",
	"sound club 2",
	"sound club",
	"soundcontrol",
	"soundfactory",
	"soundfx 2",
	"soundfx",
	"sound images",
	"sound master ii v1",
	"sound master ii v3",
	"sound master",
	"soundplayer",
	"sound programming language",
	"soundtracker 2.6",
	"soundtracker pro ii",
	"special fx",
	"special fx st",
	"speedy a1 system",
	"speedy system",
	"spu",
	"starkos",
	"startrekker am",
	"stereo sidplayer",
	"steve barrett",
	"stonetracker",
	"suntronic",
	"sunvox",
	"svar tracker",
	"symphonie",
	"synder sng-player",
	"synder sng-player stereo",
	"synder tracker",
	"synth dream",
	"synthesis",
	"syntracker",
	"tcb tracker",
	"tfm music maker",
	"tfmx",
	"tfmx st",
	"the 0ok amazing synth tracker",
	"the holy noise",
	"the musical enlightenment",
	"thomas hermann",
	"tomy tracker",
	"tss",
	"tunefish",
	"unique development",
	"unis 669",
	"velvet studio",
	"vgm music maker",
	"vic-tracker",
	"voodoo supreme synthesizer",
	"wally beben",
	"x-tracker",
	"zoundmonitor"
};

const char *other_formats[] = {
	"fasttracker 2",
	"fasttracker",
	"impulsetracker",
	"screamtracker 3",
	"nintendo ds sound format",
	"nintendo sound format",
	"s98",
	"sc68",
	"screamtracker 2",
	"megadrive cym",
	"megadrive gym",

	"mikmod unitrk",
	"multitracker",

	"ay amadeus",
	"ay emul",
	"ay strc",

	"ultratracker",

	"v2",

	"capcom q-sound format",
	"playstation sound format",
	"spectrum asc sound master",
	"spectrum fast tracker",
	"spectrum flash tracker",
	"spectrum fuxoft ay language",
	"spectrum global tracker",
	"spectrum pro sound creator",
	"spectrum pro sound maker",
	"spectrum pro tracker 1",
	"spectrum pro tracker 2",
	"spectrum pro tracker 3",
	"spectrum sound tracker 1.1",
	"spectrum sound tracker 1.3",
	"spectrum sound tracker pro 2",
	"spectrum sound tracker pro",
	"spectrum sq tracker",
	"spectrum st song compiler",
	"spectrum vortex",
	"spectrum vortex tracker ii",
	"spectrum zxs",
	"super nintendo sound format",
	"ultra64 sound format",
	"bbc micro",
	"colecovision",
	"gameboy sound format",
	"gameboy sound system",
	"gameboy sound system gbr",
	"gameboy tracker",
	"sega 32x",
	"sega game gear",
	"sega master system",
	"sega mega cd",
	"sega megadrive",
	"sega sc-3000",
	"sega sg-1000",
	"vectrex",
	"ym",
	"ymst",
	"wonderswan",
	"commodore 64",
	"super nintendo",
	"atari st",
	"atari 8bit",
	"mp3",
	"dreamcast sound format",
};

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
}

static uint8_t formatToByte(const std::string &fmt) {

	static bool init = false;
	if(!init) {
		initFormats();
		init = true;
	}

	string f = toLower(fmt);
	uint8_t l = format_map[f];
	if(l == 0) {

		l = UNKNOWN_FORMAT;

		if(endsWith(f, "tracker"))
			l = TRACKER;
		if(startsWith(f, "protracker"))
			l = PROTRACKER;
		else
		if(startsWith(f, "fasttracker"))
			l = FASTTRACKER;
		else
		if(startsWith(f, "impulsetracker"))
			l = IMPULSETRACKER;
		else
		if(startsWith(f, "screamtracker"))
			l = SCREAMTRACKER;
		else
		if(startsWith(f, "atari"))
			l = ATARI;
		else
		if(startsWith(f, "ay ") || startsWith(f, "spectrum "))
			l = SPECTRUM;
		else
		if(startsWith(f, "gameboy"))
			l = GAMEBOY;
		if(f.find("megadrive") != string::npos)
			l = MEGADRIVE;
		format_map[f] = l;
		//fprintf(stderr, "%s\n", f.c_str());
	}
	return l;
}

/*
emul
ct cyber tracker
hes
atari digi mix
kss .. really gme?
*/

template <typename T> static void readVector(std::vector<T> &v, File &f) {
	auto sz = f.read<uint32_t>();
	v.resize(sz);
	f.read((uint8_t*)&v[0], v.size()*sizeof(T));

}

template <typename T> static void writeVector(std::vector<T> &v, File &f) {
	f.write<uint32_t>(v.size());
	f.write((uint8_t*)&v[0], v.size()*sizeof(T));
}

void MusicDatabase::readIndex(File &f) {
	readVector(titleToComposer, f);
	readVector(composerToTitle, f);
	readVector(composerTitleStart, f);
	readVector(formats, f);

	titleIndex.load(f);
	composerIndex.load(f);
}

void MusicDatabase::writeIndex(File &f) {
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
	auto q = db.query<int, string, string, string>("SELECT ROWID,name,url,localdir FROM collection");
	while(q.step()) {
		auto c = q.get<Collection>();
		loader.registerSource(c.name, c.url, c.local_dir);
	}

	File f { File::getCacheDir() + "index.dat" };

	if(!reindexNeeded && f.exists()) {
		readIndex(f);
		f.close();
		return;
	}

	print_fmt("Creating Search Index...\n");

	string oldComposer;
	auto query = db.query<string, string, string, string, string, int>("SELECT title, game, format, composer, path, collection FROM song");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;

	titleToComposer.reserve(438000);
	composerToTitle.reserve(37000);
	titleIndex.reserve(438000);
	composerIndex.reserve(37000);
	formats.reserve(438000);

	int step = 438000 / 20;


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

		uint8_t b = formatToByte(fmt);
		formats.push_back(b | (collection<<8));

		if(game != "") {
			if(title != "")
				title = format("%s [%s]", game, title);
			else title = game;
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

	// composers[name] -> vector of titleindexes for each composer.

	LOGD("Found %d composers and %d titles", composers.size(), titleToComposer.size());

	composerTitleStart.resize(composers.size());
	for(const auto &p : composers) {
		// p,first == composer, p.second == vector
		auto cindex = p.second[0];
		composerTitleStart[cindex] = composerToTitle.size();
		for(int i=1; i<(int)p.second.size(); i++)
			composerToTitle.push_back(p.second[i]);
		composerToTitle.push_back(-1);
	}

	writeIndex(f);
	f.close();

	reindexNeeded = false;
}


void MusicDatabase::initFromLua(const string &fileName) {

	reindexNeeded = false;

	LuaInterpreter lua;

	lua.registerFunction<void, string, string>("set_db_var", [&](string name, string val) {
		static unordered_map<string, string> dbmap;
		LOGD("%s %s", name, val);
		if(val == "start") {
		} else if(val == "end") {
			initDatabase(dbmap);
			dbmap.clear();
		} else {
			dbmap[name] = val;
		}
	});

	File f { fileName };

	if(fileName == "") {
		auto path = File::getUserDir() + ":" + current_exe_path() + ":" + File::getAppDir();
		f = File::findFile(path, "lua/db.lua");
	}

	lua.loadFile(f.getName());
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
}


}
