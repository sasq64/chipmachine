#include "SongFileIdentifier.h"

#include <string>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/log.h>
#include <archive/archive.h>

#define WITH_MPG123

#ifdef WITH_MPG123
#include <mpg123.h>
#endif
using namespace utils;
using namespace std;

static string get_string(uint8_t *ptr, int size) {

	auto end = ptr;
	while(*end && end-ptr < size) end++;
	return string((const char*)ptr, end-ptr);
}

bool parseSid(SongInfo &info) {
	static vector<uint8_t> buffer(0xd8);
	File f { info.path };
	info.format = "Commodore 64";
	f.read(&buffer[0], buffer.size());
	info.title = get_string(&buffer[0x16], 0x20);
	info.composer = get_string(&buffer[0x36], 0x20);
	//auto copyright = string((const char*)&buffer[0x56], 0x20);
	f.close();
	return true;
}

vector<string> getLines(const std::string &text) {

	vector<string> lines;

	char tmp[256];
	char *t = tmp;
	const char *ptr = text.c_str();
	bool eol = false;
	while(*ptr) {
		if(t - tmp >= 255) break;

		while(*ptr == 10 || *ptr == 13) {
			ptr++;
			eol = true;
		}
		if(eol) {
			*t = 0;
			t = tmp;
			lines.push_back(tmp);
			eol = false;
		}
		*t++ = *ptr++;
	}

	*t = 0;
	t = tmp;
	if(strlen(t) > 0)
		lines.push_back(tmp);

	return lines;
}

bool parseSap(SongInfo &info) {
	File f { info.path };

	auto data = f.readAll();

	auto end_of_header = search_n(data.begin(), data.end(), 2, 0xff);
	auto header = string(data.begin(), end_of_header);
	auto lines = getLines(header); 

	if(lines.size() == 0 || lines[0] != "SAP")
		return false;

	for(const auto &l : lines) {
		if(startsWith(l, "AUTHOR"))
			info.composer = lrstrip(l.substr(7), '\"');
		else
		if(startsWith(l, "NAME"))
			info.title = lrstrip(l.substr(5), '\"');
	}

	info.format = "Atari 8Bit";

	return true;

}

extern "C" {
int unice68_depacker(void * dest, const void * src);
int unice68_get_depacked_size(const void * buffer, int * p_csize);
}

bool parseSndh(SongInfo &info) {

	File f { info.path };
	LOGD("SNDH >%s", info.path);
	uint8_t *unpackptr = nullptr;
	//uint8_t *ptr = f.getPtr();
	//int size = f.getSize();
	auto data = f.readAll();
	auto *ptr = &data[0];
	int size = data.size();
	string head = get_string(ptr, 4);
	if(head == "ICE!") {
		int dsize = unice68_get_depacked_size(ptr, NULL);
		LOGD("Unicing %d bytes to %d bytes", size, dsize);
		unpackptr = new uint8_t [ dsize ];
		int res = unice68_depacker(unpackptr, ptr);
		if(res == 0) {
			ptr = unpackptr;
			size = dsize;
		}
	}

	auto id = get_string(ptr + 12, 4);

	if(id == "SNDH") {

		info.format = "Atari ST";

		//LOGD("SNDH FILE");
		int count = 10;
		int got = 0;
		ptr += 16;
		string arg;
		string tag = get_string(ptr, 4);
		//LOGD("TAG %s", tag);
		while(tag != "HDNS") {
			if(count-- == 0)
				break;
			uint8_t *p = ptr;
			if(tag == "#!SN" || tag == "TIME") {
				ptr += 12;
			} else {
				while(*ptr)
					ptr++;
				while(!(*ptr))
					ptr++;
			}
			if(tag == "TITL") {
				got |= 1;
				info.title = get_string(p+4, 256);
			} else if(tag == "COMM") {
				got |= 2;
				info.composer = get_string(p+4, 256);
			}
			if(got == 3)
				break;
			tag = get_string(ptr, 4);
			//LOGD("TAG %s", tag);
		}
		LOGD("%s - %s", info.title, info.composer);
		if(unpackptr)
			delete [] unpackptr;
		return true;
	}

	if(unpackptr)
		delete [] unpackptr;
	return false;
}

bool parseSnes(SongInfo &info) {

	static vector<uint8_t> buffer(0xd8);

	info.format = "Super Nintendo";

	auto *a = Archive::open(info.path, ".rsntemp", Archive::TYPE_RAR);
	//LOGD("ARCHIVE %p", a);
	bool done = false;
	for(auto s : *a) {
		//LOGD("FILE %s", s);
		if(done) continue;
		if(path_extension(s) == "spc") {
			a->extract(s);
			File f { ".rsntemp/" + s };
			f.read(&buffer[0], buffer.size());
			f.close();
			if(buffer[0x23] == 0x1a) {
				//auto title = string((const char*)&buffer[0x2e], 0x20);
				auto ptr = (const char*)&buffer[0x4e];
				auto end = ptr;
				while(*end) end++;
				auto game = get_string(&buffer[0x4e], 0x20);
				auto composer = get_string(&buffer[0xb1], 0x20);

				f.seek(0x10200);
				int rc = f.read(&buffer[0], buffer.size());
				if(rc > 12) {
					auto id = string((const char*)&buffer[0], 4);
					if(id == "xid6") {
						//int i = 0;
						if(buffer[8] == 0x2) {
							int l = buffer[10];
							game = string((const char*)&buffer[12], l);
						} else if(buffer[8] == 0x3) {
							int l = buffer[10];
							composer = string((const char*)&buffer[12], l);
						}
					}
				}

				info.composer = composer;
				info.game = game;
				info.title = "";
				done = true;
			}
		}
	}
	delete a;
	return done;
}

bool parseMp3(SongInfo &info) {
#ifdef WITH_MPG123
	int err = mpg123_init();
	mpg123_handle *mp3 = mpg123_new(NULL, &err);

	if(mpg123_open(mp3, info.path.c_str()) != MPG123_OK)
		return false;

	mpg123_format_none(mp3);

	mpg123_scan(mp3);
	int meta = mpg123_meta_check(mp3);
	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;
	if(meta & MPG123_ID3 && mpg123_id3(mp3, &v1, &v2) == MPG123_OK) {
		if(v2) {
			info.title = htmldecode(v2->title->p);
			info.composer = htmldecode(v2->artist->p);
		} else
		if(v1) {
			info.title = htmldecode((char*)v2->title);
			info.composer = htmldecode((char*)v2->artist);
		}
	}

	info.format = "MP3";

	if(mp3) {
		mpg123_close(mp3);
		mpg123_delete(mp3);
	}
	mpg123_exit();
	return true;
#else
	return false;
#endif
}

bool parsePList(SongInfo &info) {

	File f { info.path };

	info.title = path_basename(info.path);
	info.composer = "";
	info.format = "Playlist";

	for(auto l : f.getLines()) {
		if(l.length() > 0 && l[0] == ';') {
			auto parts = split(l.substr(1), "\t");
			info.title = parts[0];
			if(parts.size() >= 2) {
				info.composer = parts[1];
				info.format = "C64 Demo";
			} else
				info.format = "C64 Event";
		}
	}
	return true;
}

bool identify_song(SongInfo &info, string ext) {

	if(ext == "")
		ext = path_extension(info.path);

	if(ext == "plist")
		return parsePList(info);
	if(ext == "rsn")
		return parseSnes(info);
	if(ext == "sid")
		return parseSid(info);
	if(ext == "sndh")
		return parseSndh(info);
	if(ext == "sap")
		return parseSap(info);
	if(ext == "mp3")
		return parseMp3(info);
	return false;
}
