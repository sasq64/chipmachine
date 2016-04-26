
-- Given the link to a youtube URL, return an URL to an audio stream
function on_parse_youtube (url)
	if WINDOWS then
	  name = os.getenv("TMP") .. "/ydlout.txt"
	else
	  name = os.tmpname()
	end
	os.remove(name)
	cm_execute(string.format('youtube-dl --skip-download -g "%s" > %s', url, name))

	url = ''

	for l in io.lines(name) do
		if string.find(l, 'mime=audio',1 , true) then
			url = l
			break
		end
		if string.find(l, 'audio',1 , true) then
			url = l
		end
	end
	
	os.remove(name)
	return url
	
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

