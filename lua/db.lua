
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- local_dir : If exists, will be checked first for files before downloading.
-- If song_list or or source can not be found, database will not be added

VERSION = 5;

DB = {
{
	name = "Amiga Playlists",
	type = "amipl",
	local_dir = "data/playlists/amiga",
	color = 0xfffff
},
{
	name = "CSDb",
	type = "csdb",
	local_dir = "",
	song_list = "data/csdb.xml",
	color = 0xfffff
},
{
	name = "Sounds of Scenesat",
	type = "scensatpl",
	local_dir = "data/playlists/scenesat",
	color = 0xfffff
},
{
	name = "Modland",
	type = "modland",
	source = "http://ftp.modland.com/pub/modules/",
	song_list = "data/allmods.txt",
	local_dir = "/opt/Music/MODLAND",
	exclude_formats = "RealSID;PlaySID;Nintendo SPC;SNDH;Slight Atari Player;Super Nintendo Sound Format",
	color = 0xfffff
},
{
	name = "HVSC",
	type = "hvsc",
	-- source = "http://www.prg.dtu.dk/HVSC/C64Music/",
	-- source = "http://hvsc.etv.cx/C64Music/",
	source = "http://www.sannic.nl/hvsc/HVSC63/C64Music/",
	song_list = "data/hvsc.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/hvsc.txt",
	local_dir = "/opt/Music/C64Music",
	color = 0xfffff
 },
 {
	name = "snesmusic.org",
	type = "rsn",
	source = "http://snesmusic.org/v2/download.php?spcNow=",
	make_source = snes,
	song_list = "data/rsn.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rsn.txt",
	local_dir = "/opt/Music/spcsets",
	color = 0xfffff
},
{
	name = "sndh",
	type = "sndh",
	source = "http://sndh.atari.org/sndh/sndh_lf/",
	song_list = "data/sndh.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/sndh.txt",
	local_dir = "/opt/Music/sndh_lf",
	color = 0xfffff
},
{
	name = "asma",
	type = "asma",
	source = "http://asma.atari.org/asma/",
	song_list = "data/asma.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/asma.txt",
	local_dir = "/opt/Music/asma",
	color = 0xfffff
},
{
	name = "remix.kwed.org",
	type = "rko",
	source = "http://remix.kwed.org/download.php/",
	song_list = "data/rko.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rko.txt",
	local_dir = "/opt/Music/rko",
	color = 0xfffff
},
{
	name = "amigaremix",
	type = "amigaremix",
	source = "http://amigaremix.com/listen/",
	song_list = "data/amiremix.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/amiremix.txt",
	local_dir = "/opt/Music/amiremix",
	color = 0xfffff
},
{
	name = "scenesat",
	type = "scenesat",
	source = "http://sos.scenesat.com/play/",
	song_list = "data/scenesat.txt",
	local_dir = "/opt/Music/scenesat",
	color = 0xfffff
},
{
	name = "Bitjam",
	type = "bitjam",
	source = "http://malus.exotica.org.uk/pub/",
	remote_list = "http://www.bitfellas.org/podcast/podcast.xml",
	color = 0xfffff
},
{
	name = "Demovibes",
	type = "demovibes",
	source = "http://www.demovibes.org/downloads/",
	song_list = "data/demovibes.txt",
	color = 0xfffff
},
{
	name = "Amigavibes",
	type = "amigavibes",
	source = "http://www.amigavibes.org/index.php/download/category/2-podcast-musicaux?download=",
	song_list = "data/amigavibes.txt",
	color = 0xfffff
},
{
	name = "Radio",
	type = "radio",
	source = "",
	song_list = "data/radio.txt",
	color = 0xfffff
},
{
	name = "Bitar till Kaffet",
	type = "podcast",
	source = "",
	remote_list = "http://www.bitartillkaffet.se/?feed=podcast",
	extra_list = "data/bitar.txt",
	color = 0xfffff
}
--,{
--	name = "Pouet/Youtube",
--	type = "pouet",
--	source = "",
--	song_list = "data/pouet_youtube.xml",
--	color = 0xfffff
--}
};
