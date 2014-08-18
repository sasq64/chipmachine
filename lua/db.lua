
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- id : unique id for database. Also sort order
-- If song_list or or source can not be found, database will not be added


DB = {
{
	type = "modland",
	id = 1,
	source = "ftp://ftp.modland.com/pub/modules/",
	song_list = "data/allmods.txt",
	exclude_formats = "RealSID;PlaySID;Nintendo SPC;SNDH;Slight Atari Player",
},
{
	type = "hvsc",
	id = 2,
	source = "http://hvsc.etv.cx/C64Music/",
	song_list = "data/hvsc.txt",
	local_dir = "/opt/Music/C64Music"
},
{
	type = "rsn",
	id = 3,
	source = "http://snesmusic.org/v2/download.php?spcNow=",
	song_list = "data/rsn.txt",
	local_dir = "/opt/Music/spcsets"
},
{
	type = "sndh",
	id = 4,
	-- source = "http://",
	song_list = "data/sndh.txt",
	local_dir = "/opt/Music/sndh_lf"
},
{
	type = "asma",
	id = 5,
	-- source = "http://",
	song_list = "data/sap.txt",
	local_dir = "/opt/Music/asma"
}
};
