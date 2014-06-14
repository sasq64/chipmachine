

DB.modland = {
	type = "modland",
	url = "ftp://ftp.modland.com/pub/modules/",
	id = 1,
	source = "data/allmods.txt"
};

DB.hvsc = {
	type = "hvsc",
	id = 2,
	source = "C64Music"
}

DB.NSFE = {

	source = "C64Music"
	source_lines = { "MUSICIANS/*/{AUTHOR}/{TITLE}.sid"
	"GAMES/*/{TITLE}.sid", AUTHOR = "Unknown" }
}