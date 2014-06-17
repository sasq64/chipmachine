
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- id : unique id for database
-- If song_list or or source can not be found, database will not be added


DB.modland = {
	source = "/home/sasq/Modland",
	--"ftp://ftp.modland.com/pub/modules/",
	id = 1,
	song_list = "/home/sasq/Modland/allmods.txt"
	--"data/allmods.txt"
};

DB.hvsc = {
	type = "hvsc",
	id = 2,
	source = "C64Music"
};

DB.rsn = {
	type = "rsn",
	id = 3,
	source = "spcsets"
};

-- DB.NSFE = {
-- 	source = "NSFE"
-- 	source_lines = { mst"MUSICIANS/*/{AUTHOR}/{TITLE}.sid"
-- 	"GAMES/*/{TITLE}.sid", AUTHOR = "Unknown" }
-- }