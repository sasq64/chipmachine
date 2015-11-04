
## ISSUES

* Deal with no Internet
* Deal with 404
* UADE SONG END does not go to next?
* Loop detecton, max play or remember skip position?
* Limit stream speed
* _lib2 and _lib3 support for PSF
* Common resampler in player, not in plugins

* Repeating tweens sometimes die (title scrolling stops, selection hilight gone)
* webgetter renaming fails when downloading the same song in parallell.
* Format colors wrong
* Parse collection color from db.lua ?
* sid titles wrong encoding 
* Mouse click select and scroll
* Search hits shown twice when term is in both composer and title
* Preload next song (RemoteLoader.assureCached() ?)
* Indicate error if lua fails

* Pause tween not stopped when new song starts


## TASKS / FEATURES

* Seeking in MP3s
* Seeking in MP3 streams
* Shuffle current play queue

* Print more info (KB size, source)

### Deal with different encodings

### Deal with multi file songs

Problem: Some SNG files does not require INS files.

Problem: We want to treat some multi song files as one song with subtunes

### Playlist support

Multiple playlists
Online playlist
Smart playlists

Separate Playlist / Playqueue screen with edit capabilities


======================================================================================


*DO:*

When parsing modland, look for multi file songs.
Create paths such as
modland::some/path::file.ins::file.sng
or
modland::some/game::song01.psf::song02.psf....
The latter case should be treated as sub songs.

When loading path, load all files
Mark secondary files. If more than one primary file, treat as sub songs

### Rewrite search interface

Search string -> words
Lookup first word in all indexes



## DONE

X Better tween on song failure, and go to next after delay
X List displays old result after last
X Second word digits => wrong results
X MDX Needs secondary file download
X Tween source position for songs that are not NEXT seems wrong
X MP3 on Raspberry
X Star skip
X Hilight color fade to type color
X Key repeat
X Skip to next prev composer

X &amp; in mp3 names
X Local MP3 not mp3 format
X Fade out eq on list screen
X EQ colors from screen.lua

X Report song failed to play

X VOLUME CONTROL
X QUIT

X Slow playlist info parsing
X Remove slash on non demo lists

X DB init in thread, progess display

X Update db rules

X Remove online playlists for now

X Mods don't end (Compilications.mod)





## OLD STUFF

## STATIC PLAYLISTS

### Sounds of Scenesat

### Amiga Classics


## PLUGINS TO BE ADDED

* TEDPLAY

## FILTER

Collection & Type stored
Configure colors in lua file

Collection IS <name>
Format IS between <lower> and <higher>

## COMMAND MODE

Default TAB to Enter, ENTER to leave
Uses searchIndex for registered commands to render the main list






//

>> NETWORK RULES <<
>>>>>>>>><<<<<<<<<<




RELOGIN (NO SUCH USER)

Song is PATH in COLLECTION
FULL_PATH depends on offline/online

PATH,COLLECTION -> FULL_PATH
FULL_PATH -> PATH,COLLECTION

songLoader.addSource(name, local_dir, url)


File = SONG_LOADER(path, collection)

LOADER is a class that loads files either locally or from a remote location, and can cache the files




* Network threads stops quitting


filtering

filters.lua

NAME, set of formats

TAB to cycle filters    

PARTY MODE
----------

Tween probs: Pause and party red
release lock after song end wven if no new song
subsong++ -- when song not switched...


2 second grace only allows sub song?
only applies to songs started with ENTER



v1 SIMPLE
* Song must play for at least 60 seconds before new song can be selected, unless ALT is held


ALWAYS
* Queue remains with ENTER 





Own lock for fft 

SUBSONG does not work after ending in
USF64

Silence at start should have higher threshhold




DC and DS needs to rename libnames to lowercase




* Subtitle in USF64 (and dreamcast?)
* Old info returns if song cant be played
* position next info to right edge
* larger sizes on larger screen

* subsong grouping... RSN vs USF64 for instance.. search on subsongs?





----------------

BUGS

* PI cant always keep up with fft audio
* PI jerky scroll, high cpu when more than ~15 texts on search screen?

* Dont index secondary files
* Dont duplicate fonts
* openmpt type name too long
* Freeze when starting mods?
* Speed up inc search? Remember last set?

FX

* Scroll screens (& stars) left right
* Scroll stars up/down with playlist scrolling


GUI

* Edit playlist screen
* KEY brings up EDITOR of CURRENT list
* Can save to any list name and replaces old
* Find show playlist hits first

TABLE playlist
 NAME INDEX

TABLE plmapping
 PLINDEX IDX SONGINDEX




LUA

* Return/Input VECTOR = Table
*


pl = find('stardust memories')
play(pl)


InfoScreen.add('Hello')

function InfoScreen.add(text, x, y, scale, color)
	infoscreen_add(text, x, y, scale_color);
end


playlist.mt = {}

function Playlist.new()
	local pl = {}
	setmetatable(pl, Playlist.mt)
	return pl
end





struct SongData {
	string title;
	string composer;
	string format;
	string source;
	string url;
	uint64_t uniqueid;

};


class SongSequence {
	int count();
	SongData operator[](int index);
};

class SongList {


class InfoScreen {

	struct InfoText {
		std::string text;
		int x;
		int y;
		uint32_t color;
		float alpha;
		float size;
	}

	shared_ptr<InfoText> add_text(const std::string &text, int x, int y, Font &font, uint32_t color, float size)

	std::vector<std::shared_ptr<InfoText>> texts;

};


auto t = screen.add_text();

tween(t.x




class CommandParser {
public:
	typedef std::function<void(const std::vector<std::string> &args)> Function;

	struct Command {
		std::string name;
		Function callback;
	};

	CommandParser() {
		defineType('i', [](std::string &arg) -> bool {
			stoi(arg);
			return true;
		});
	}

	void defineType(char c, std::function<bool(std::string &arg)> verifier);

	void add(const std::string &name, const std::string &desc, Function f);

	vector<Command> matchCommands(const std::string &line);

	void applyCommand(const Command &cmd, const std::vector<std::string> &args);


	void parse(const std::string &line) {
/*
		auto parts = split(line);
		if(parts.size() < 1)
			return true;

		vector<Command*> foundCommands;

		int l = parts[0].length();
		for(auto &c : commands) {
			if(c.name.substr(l) == parts[0]) {
				foundCommands.push_back(&c);
			}
		}

		if(foundCommands.size() == 1) {
			for(int i=1; i<parts.size(); i++) {
				char c = command.args[i-1].type;
				auto verifier = types[c].verifier;
				verifier(parts[i]);
			}
			foundCommands[0].callback(parts);
		} else {
			for(auto &c : foundCommands) {
			}
		}*/
	}

};
