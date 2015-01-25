
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- local_dir : If exists, will be checked first for files before downloading. Also used to create song list
-- id : unique id for database. Also sort order
-- If song_list or or source can not be found, database will not be added


DB = {
{
	type = "modland",
	id = 1,
	source = "ftp://ftp.modland.com/pub/modules/",
	song_list = "data/allmods.txt",
	exclude_formats = "RealSID;PlaySID;Nintendo SPC;SNDH;Slight Atari Player",
	color = 0xfffff
},
{
	type = "hvsc",
	id = 2,
	-- source = "http://www.prg.dtu.dk/HVSC/C64Music/",
	source = "http://hvsc.etv.cx/C64Music/",
	song_list = "data/hvsc.txt",
	local_dir = "/opt/Music/C64Music",
	color = 0xfffff
},
{
	type = "rsn",
	id = 3,
	source = "http://snesmusic.org/v2/download.php?spcNow=",
	song_list = "data/rsn.txt",
	local_dir = "/opt/Music/spcsets",
	color = 0xfffff
},
{
	type = "sndh",
	id = 4,
	source = "http://sndh.atari.org/sndh/sndh_lf/",
	song_list = "data/sndh.txt",
	local_dir = "/opt/Music/sndh_lf",
	color = 0xfffff
},
{
	type = "asma",
	id = 5,
	source = "http://asma.atari.org/asma/",
	song_list = "data/asma.txt",
	local_dir = "/opt/Music/asma",
	color = 0xfffff
},
{
	type = "rko",
	id = 6,
	source = "http://remix.kwed.org/download.php/",
	song_list = "data/rko.txt",
	local_dir = "/opt/Music/rko",
	color = 0xfffff
},
{
	type = "amigaremix",
	id = 7,
	source = "http://amigaremix.com/listen/",
	song_list = "data/amiremix.txt",
	local_dir = "/opt/Music/amiremix",
	color = 0xfffff
},
{
	type = "scenesat",
	id = 8,
	source = "",
	song_list = "data/scenesat.txt",
	local_dir = "/opt/Music/scenesat",
	color = 0xfffff
}
};
