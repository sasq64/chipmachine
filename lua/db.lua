
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- id : unique id for database
-- If song_list or or source can not be found, database will not be added


DB.modland = {
	source = "/home/sasq/Modland",
	-- source = "/media/sasq/fc254b57-8fff-4f96-9609-ea202d871acf/MUSIC/Modland/",
	-- source = "ftp://ftp.modland.com/pub/modules/",
	id = 1,
	-- song_list = "/media/sasq/fc254b57-8fff-4f96-9609-ea202d871acf/MUSIC/allmods.txt"
	song_list = "/home/sasq/allmods.txt",
	exclude_formats = "RealSID;PlaySID"
	-- song_list = "data/allmods.txt"
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