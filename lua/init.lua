
songs = {}

function cmd_status ()
	print "STATUS"
end

function cmd_find (query)
	songs = find(query)
	cmd_print_result()
end

function cmd_print_result ()
	for i,song in ipairs(songs) do
		print(string.format("[%02d] %s - %s", i, song.composer, song.title))
	end
end

function cmd_play (no)
	play_file(songs[no].path)
end


toast("New telnet connection", 0);

-- add_command('status', 'cmd_status');
-- add_command('find', 'cmd_find');
