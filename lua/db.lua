
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- local_dir : If exists, will be checked first for files before downloading.
-- If song_list or or source can not be found, database will not be added

VERSION = 11;

DB = {
{
	name = "Playlists",
	id =  "pl",
	local_dir = "data/playlists",
	color = 0xfffff
},
{
	name = "CSDb",
	id =  "csdb",
	local_dir = "",
	song_list = "data/csdb.xml",
	color = 0xfffff
},
{
	name = "Modland",
	id =  "modland",
	source = "http://ftp.modland.com/pub/modules/",
	song_list = "data/allmods.txt",
	local_dir = "/opt/Music/MODLAND",
	exclude_formats = "RealSID;PlaySID;Nintendo SPC;SNDH;Slight Atari Player;Super Nintendo Sound Format",
	color = 0xfffff
},
{
	name = "HVSC",
	id =  "hvsc",
	-- source = "http://www.prg.dtu.dk/HVSC/C64Music/",
	-- source = "http://hvsc.etv.cx/C64Music/",
	source = "https://www.sannic.nl/hvsc/C64Music/",
	song_list = "data/hvsc.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/hvsc.txt",
	local_dir = "/opt/Music/C64Music",
	color = 0xfffff
},
{
	name = "HVTC",
	id =  "hvtc",
	source = "http://plus4world.powweb.com/feat/tedsound/player/",
	song_list = "data/hvtc.txt",
	local_dir = "/opt/Music/hvtc",
	color = 0xfffff
 },
 {
	name = "snesmusic.org",
	id =  "rsn",
	source = "http://snesmusic.org/v2/download.php?spcNow=",
	make_source = snes,
	song_list = "data/rsn.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rsn.txt",
	local_dir = "/opt/Music/spcsets",
	color = 0xfffff
},
{
	name = "sndh",
	id =  "sndh",
	source = "http://sndh.atari.org/sndh/sndh_lf/",
	song_list = "data/sndh.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/sndh.txt",
	local_dir = "/opt/Music/sndh_lf",
	color = 0xfffff
},
{
	name = "asma",
	id =  "asma",
	source = "http://asma.atari.org/asma/",
	song_list = "data/asma.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/asma.txt",
	local_dir = "/opt/Music/asma",
	color = 0xfffff
},
{
	name = "remix.kwed.org",
	id =  "rko",
	source = "http://remix.kwed.org/download.php/",
	song_list = "data/rko.txt",
	utf8 = "no",
	song_template = "path sidname sidsong title composer rating",
	format = "MP3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rko.txt",
	local_dir = "/opt/Music/rko",
	color = 0xfffff
},
{
	name = "amigaremix",
	id =  "amigaremix",
	source = "http://amigaremix.com/listen/",
	song_list = "data/amiremix.txt",
	song_template = "no path title composer",
	format = "MP3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/amiremix.txt",
	local_dir = "/opt/Music/amiremix",
	color = 0xfffff
},
{
	name = "scenesat",
	id =  "scenesat",
	source = "http://sos.scenesat.com/play/",
	song_list = "data/scenesat.txt",
	song_template = "composer game title format path",
	local_dir = "/opt/Music/scenesat",
	color = 0xfffff
},
{
	name = "Bitjam",
	id =  "bitjam",
	type = "podcast",
	source = "http://malus.exotica.org.uk/pub/",
	remote_list = "http://www.bitfellas.org/podcast/podcast.xml",
	color = 0xfffff
},
{
	name = "Demovibes",
	id =  "demovibes",
	source = "http://www.demovibes.org/downloads/",
	song_list = "data/demovibes.txt",
	color = 0xfffff
},
{
	name = "Amigavibes",
	id =  "amigavibes",
	source = "http://www.amigavibes.org/index.php/download/category/2-podcast-musicaux?download=",
	song_list = "data/amigavibes.txt",
	color = 0xfffff
},
{
	name = "Radio",
	id =  "radio",
	source = "",
	song_list = "data/radio.txt",
	color = 0xfffff
},
{
	name = "Bitar till Kaffet",
	id = "bitar",
	type = "podcast",
	source = "",
	song_list = "data/bitar.xml",
	remote_list = "http://www.bitartillkaffet.se/?feed=podcast",
	color = 0xfffff
},
{
	name = "Bitar till Kaffet",
	id =  "bitar2",
	source = "http://www.bitartillkaffet.se/media/",
	song_list = "data/bitar.txt",
	color = 0xfffff
},
{
	name = "This Week in Chiptune",
	id = "weekchip",
	type =  "podcast",
	source = "",
	presenter = "Dj CUTMAN",
	song_list = "http://thisweekinchiptune.libsyn.com/rss",
	color = 0xfffff
},
{
	name = "Gamewave Podcast",
	id = "gamewave",
	type = "podcast",
	source = "",
	song_list = "http://gamewave.yays.co/rss.xml",
	color = 0xfffff
},
{
	name = "Syntax Error",
	id =  "syntax",
	source = "http://se-ksd-01.files.syntaxerror.nu/mp3/",
	song_list = "data/syntax.txt",
	song_template = "path title",
	format = "MP3",
	-- presenter = "Sol"
	color = 0xfffff
},
-- {
-- 	name = "NSFE",
-- 	id = "nsfe",
-- 	source = "",
-- 	song_list = "data/nsfe.txt",
-- 	local_dir = "/opt/M/nsfe",
-- 	color = 0xfffff
-- },
{
	name = "C64 Take-away",
	id = "takeaway",
	type =  "podcast",
	source = "",
	song_list = "data/c64takeaway.xml",
	color = 0xfffff
},


--,{
--	name = "Pouet/Youtube",
--	id =  "pouet",
--	source = "",
--	song_list = "data/pouet_youtube.xml",
--	color = 0xfffff
--}
};
