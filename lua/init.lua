
-- Playlist

Playlist = {}
Playlist.mt = {}

function Playlist.new()
	local pl = {}
	setmetatable(pl, Playlist.mt)
	return pl
end

function Playlist.concat(a, b)
	local res = Set.new{}
	for _,v in ipairs(a) do res[#t+1] = v end
	for _,v in ipairs(b) do res[#t+1] = v end
	return res
end

Console = {}
currentPlayList = {}

function Console.find(...)
	s = ''
	for i = 1, #arg do
		if i > 1 then s = s..' ' end
		s = s..arg[i]
	end
	result = db_find(s)
end
