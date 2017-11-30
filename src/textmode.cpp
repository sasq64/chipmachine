#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/editor.h>
#include <map>
#include "ChipInterface.h"
#include "TextListView.h"

namespace chipmachine {

void runConsole(std::shared_ptr<bbs::Console> console, ChipInterface &ci) {
	using namespace bbs;
    int bgColor = Console::DARK_GREY;
	
	auto iquery = ci.createQuery();
	
	console->clear();
	console->flush();
	console->setColor(Console::WHITE);
	console->put(0,0, "#", Console::WHITE);
	console->moveCursor(1,0);
	LineEditor searchField(*console);
	int width = console->getWidth();
	int height = console->getHeight();
	int currentTune = 0;
	console->fill(bgColor, 0, 1, width, 1);
	console->moveCursor(0,2);
	console->setColor(Console::WHITE, Console::BLACK);
	TextListView listView(*console, height - 6, width);
    SongInfo info{};
	
	auto holder = ci.onMeta([&](const SongInfo &si) {
		info = si;
		currentTune = si.starttune;
		LOGD("Got new info %s %s", info.title, info.composer);
		console->fill(bgColor, 0, height-4, width, 4);
		console->put(0, height-4, utils::format(" TITLE: %s", si.title), Console::CURRENT_COLOR, bgColor);
		console->put(0, height-3, utils::format("AUTHOR: %s", si.composer), Console::CURRENT_COLOR, bgColor);
		console->put(0, height-2, utils::format("FORMAT: %s", si.format), Console::CURRENT_COLOR, bgColor);
		console->flush();
	});

	listView.setCallback([&](Console &c, int index, bool marked) {
		static const std::map<uint32_t, int> colors = {
		    {NOT_SET, Console::PURPLE},  {PLAYLIST, Console::GREY},
		    {CONSOLE, Console::RED},     {C64, Console::BROWN},
		    {ATARI, Console::YELLOW},    {MP3, Console::GREEN},
		    {M3U, Console::LIGHT_GREEN}, {YOUTUBE, Console::RED},
		    {PC, Console::CYAN},         {AMIGA, Console::LIGHT_BLUE},
		    {255, Console::ORANGE}};

		int color = 0;
		auto parts = utils::split(iquery->getResult(index), "\t");
		std::string text;
		int f = atoi(parts[3].c_str()) & 0xff;
		if(f == PLAYLIST) {
			if(parts[1] == "")
				text = utils::format("<%s>", parts[0]);
			else
				text = utils::format("<%s / %s>", parts[0], parts[1]);
		} else
			text = utils::format("%s / %s", parts[0], parts[1]);
	
		auto it = --colors.upper_bound(f);
		color = it->second;
		if(marked)
			c.put(text, Console::WHITE, color);
		else
			c.put(text, color, Console::CURRENT_COLOR);
	});
	std::string lastLine;
	int olds = -1;
	
	auto getSelectedSong = [&]() -> SongInfo {
		auto i = listView.marked();
		return ci.getSongInfo(iquery->getIndex(i));
	};
	
	int last_marked = -1;
	
	while(true) {
		int k = console->getKey(100);
		bool doFlush = false;
		if(k == 3) {
			console->clear();
			console->flush();
			break;
		} else
		if(k == Console::KEY_F1) {
			ci.pause(ci.playing());
		} else		
		if(k == Console::KEY_F3) {
			ci.nextSong();
		} else		
		if(k == Console::KEY_F2) {
            if(iquery->numHits() > 0) {
				ci.addSong(getSelectedSong());
				if(listView.putKey(Console::KEY_DOWN)) {
					listView.refresh();
					doFlush = true;
				}
			}
		} else
		if(k == Console::KEY_ENTER) {
            if(iquery->numHits() > 0) {
                ci.play(getSelectedSong());
                currentTune = 0;
                olds = -1;
            }
		} else if(k == Console::KEY_RIGHT) {
			if(currentTune < info.numtunes - 1)
				ci.setTune(++currentTune);
				olds = -1;
		} else if(k == Console::KEY_LEFT) {
			if(currentTune > 0)
				ci.setTune(--currentTune);
				olds = -1;
		} else
		if(k != Console::KEY_TIMEOUT) {
			bool sfr = false;
			
			bool lvr = listView.putKey(k);
			if(searchField.putKey(k)) {
				sfr = true;
				auto line = searchField.getResult();
				if(line != lastLine) {
					iquery->setString(line);
					listView.setLength(iquery->numHits());
					lastLine = line;
					lvr = true;
				}	
			}
			if(lvr)
				listView.refresh();
			if(sfr) {
				console->fill(Console::BLACK, 0, 0, width, 1);
				console->put(0,0, "#", Console::WHITE);
				searchField.refresh();
			}
			if(sfr || lvr)
				doFlush = true;
			int m = listView.marked();
			if(!sfr && m >= 0 && iquery->numHits() > 0 && m != last_marked) {
				auto song = getSelectedSong();	
				auto ext = utils::path_extension(song.path);
				bool isoffline = RemoteLoader::getInstance().isOffline(song.path);
				console->fill(Console::BLACK, 0, 0, width, 1);
				console->put(0,0, utils::format("Format: %s (%s)%s", song.format, ext, isoffline ? "*" : ""), Console::YELLOW);
				last_marked = m;
				doFlush = true;
			}
		} else {
			ci.update();
			int s = ci.seconds();
			if(s != olds) {
				auto state = ci.playing() ? "PLAYING" : " PAUSED";
				console->put(0, height-1, utils::format("%s %02d:%02d [%02d/%02d]", state, s/60, s%60, currentTune+1, info.numtunes), Console::CURRENT_COLOR, bgColor);
				doFlush = true;
				olds = s;
			}
		}
		if(doFlush)
			console->flush(true);
	}
}

} // namespace
