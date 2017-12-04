require 'find'

$modland = '/opt/Music/MODLAND'

def pack

	Dir.glob($modland + "/Ultra64 Sound Format/**/*.usflib") do |f|
		d = File.dirname(f)
		
		File.delete("#{d}.zip") if File.exist?("#{d}.zip")
		File.delete("#{d}.usfz") if File.exist?("#{d}.usfz")
		puts `zip -j "#{d}.usfz" "#{d}/"*`
	end

end

def update

	songs = {}
	total = 0
	count = 0
	open('data/allmods.txt').readlines.each { |l|
		x = l.rstrip.split("\t")
		sz = x[0].to_i
		total += sz
		count += 1
		songs[x[1]] = sz
	}
	puts "ALLMODS #{count} files in #{total} bytes"

	#Dir.glob($modland + '/**/*') do |f|
	#	puts f	
	#end
	Dir.chdir $modland
	Find.find(".") do |path|
		if !FileTest.directory?(path)
			path = path[2..-1]
			if !songs.has_value?("sdadsafasdFasddsf/sdf/dsfa/sd/fasd/fsd/fdsa/fsad/sa")
				puts path + " MISSING"
			end
		end
	end


end

require 'net/ftp'
require 'fileutils'

def download

	downloading = false
	deleting = false

	ftp = Net::FTP.new('ftp.modland.com')
	ftp.passive = true
	ftp.login

	open('modland.work').readlines.each { |l|
		l.rstrip!
		if l[0] == '#'
			what = l[1..-1].strip
			downloading = (what == 'DOWNLOAD')
			deleting = (what == 'REMOVE')
		elsif downloading
			puts "Downloading #{l}"
			d = File.dirname(l)
			FileUtils.makedirs(d) if !File.exist?(d)
			if !File.exist?(l)
				begin
					ftp.getbinaryfile("/pub/modules/#{l}", l, 1024)
				rescue Net::FTPError => e
					puts "FAILED " + e.message
				end
			else
				puts "Not overwriting #{l}"
			end
		elsif deleting
			puts "Deleting #{l}..."
			File.delete(l)
		else
			puts "WTF"
		end
	}

	ftp.close

end


download()
