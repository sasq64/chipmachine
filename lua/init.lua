
-- Given the link to a youtube URL, return an URL to an audio stream
function on_parse_youtube (url)
	name = os.tmpname()
	os.execute(string.format('youtube-dl --skip-download -g "%s" > %s', url, name))
	for l in io.lines(name) do
		if string.find(l, 'mime=audio',1 , true) then
			return l
		end
	end
	for l in io.lines(name) do
		if string.find(l, 'audio',1 , true) then
			return l
		end
	end
	return nil
end

-- Called when screen needs layout
function on_layout (width, height, ppi)
	print("LUA LAYOUT");
end


function on_select_plugin (filename, plugins)
	if string.find(filename, '.mod', 1, true) then
		for p in plugins do
			if p == 'uade' then
				return p;
			end
		end
	end
	return nil
end

