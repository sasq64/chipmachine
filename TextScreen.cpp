#include "log.h"
#include "TextScreen.h"
#include "utils.h"

#include <array>
#include <algorithm>

using namespace std;

/** PETSCII->ASCII 0x20 -> 0xFF */

int petsciiTable[] = {
	0x0020,
	0x0021,
	0x0022,
	0x0023,
	0x0024,
	0x0025,
	0x0026,
	0x0027,
	0x0028,
	0x0029,
	0x002A,
	0x002B,
	0x002C,
	0x002D,
	0x002E,
	0x002F,
	0x0030,
	0x0031,
	0x0032,
	0x0033,
	0x0034,
	0x0035,
	0x0036,
	0x0037,
	0x0038,
	0x0039,
	0x003A,
	0x003B,
	0x003C,
	0x003D,
	0x003E,
	0x003F,
	0x0040,
	0x0061,
	0x0062,
	0x0063,
	0x0064,
	0x0065,
	0x0066,
	0x0067,
	0x0068,
	0x0069,
	0x006A,
	0x006B,
	0x006C,
	0x006D,
	0x006E,
	0x006F,
	0x0070,
	0x0071,
	0x0072,
	0x0073,
	0x0074,
	0x0075,
	0x0076,
	0x0077,
	0x0078,
	0x0079,
	0x007A,
	0x005B,
	0x00A3,
	0x005D,
	0x2191,
	0x2190,
	0x2501,
	0x0041,
	0x0042,
	0x0043,
	0x0044,
	0x0045,
	0x0046,
	0x0047,
	0x0048,
	0x0049,
	0x004A,
	0x004B,
	0x004C,
	0x004D,
	0x004E,
	0x004F,
	0x0050,
	0x0051,
	0x0052,
	0x0053,
	0x0054,
	0x0055,
	0x0056,
	0x0057,
	0x0058,
	0x0059,
	0x005A,
	0x253C,
	0xF12E,
	0x2502,
	0x2592,
	0xF139,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x000A,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x00A0,
	0x258C,
	0x2584,
	0x2594,
	0x2581,
	0x258F,
	0x2592,
	0x2595,
	0xF12F,
	0xF13A,
	0xF130,
	0x251C,
	0xF134,
	0x2514,
	0x2510,
	0x2582,
	0x250C,
	0x2534,
	0x252C,
	0x2524,
	0x258E,
	0x258D,
	0xF131,
	0xF132,
	0xF133,
	0x2583,
	0x2713,
	0xF135,
	0xF136,
	0x2518,
	0xF137,
	0xF138,
	0x2501,
	0x0041,
	0x0042,
	0x0043,
	0x0044,
	0x0045,
	0x0046,
	0x0047,
	0x0048,
	0x0049,
	0x004A,
	0x004B,
	0x004C,
	0x004D,
	0x004E,
	0x004F,
	0x0050,
	0x0051,
	0x0052,
	0x0053,
	0x0054,
	0x0055,
	0x0056,
	0x0057,
	0x0058,
	0x0059,
	0x005A,
	0x253C,
	0xF12E,
	0x2502,
	0x2592,
	0xF139,
	0x00A0,
	0x258C,
	0x2584,
	0x2594,
	0x2581,
	0x258F,
	0x2592,
	0x2595,
	0xF12F,
	0xF13A,
	0xF130,
	0x251C,
	0xF134,
	0x2514,
	0x2510,
	0x2582,
	0x250C,
	0x2534,
	0x252C,
	0x2524,
	0x258E,
	0x258D,
	0xF131,
	0xF132,
	0xF133,
	0x2583,
	0x2713,
	0xF135,
	0xF136,
	0x2518,
	0xF137,
	0x2592
};

DummyTerminal dummyTerminal;

void Screen::clear() {
	for(auto &t : grid) {
		t.c = 0x20;
		t.fg = t.bg = -1;
	}
}

void Screen::put(int x, int y, const string &text) {
	for(unsigned int i=0; i<text.length(); i++) {
		Tile &t = grid[(x+i) + y * width];
		t.c = text[i];
		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
	}
}

void Screen::resize(int w, int h) {
	width = w;
	height = h;

	grid.resize(w*h);
	oldGrid.resize(w*h);

	clear();
}

void Screen::flush() {
	vector<int8_t> temp;
	int rc = update(temp);
	if(rc > 0)
		terminal.write(temp, rc);
}


// ANSISCREEN

int AnsiScreen::update(std::vector<int8_t> &dest) {

	int w = terminal.getWidth();
	int h = terminal.getHeight();
	//LOGD("Size %dx%d", w, h);
	if(w > 0 && h > 0 && (w != width || h != height)) {
		resize(w, h);
		outBuffer = { '\x1b', '[', '2', 'J' };
		LOGD("Size now %dx%d", w, h);
	}

	int orgSize = dest.size();

	if(outBuffer.size() > 0) {
		dest.insert(dest.end(), outBuffer.begin(), outBuffer.end());
		outBuffer.resize(0);
	}

	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			Tile &t1 = grid[x+y*width];
			if(t0 != t1) {					
				if(curY != y or curX != x)
					smartGoto(dest, x, y);
				if(t0.fg != t1.fg || t0.bg != t1.bg)
					setColor(dest, t1.fg, t1.bg);
				putChar(dest, t1.c);
				t0 = t1;
			}
		}
	}

	return dest.size() - orgSize;

}

void AnsiScreen::setColor(vector<int8_t> &dest, int fg, int bg) {
	const string &s = utils::format("\x1b[%dm", fg + 30);
	dest.insert(dest.end(), s.begin(), s.end());			
};

void AnsiScreen::putChar(vector<int8_t> &dest, char c) {
	dest.push_back(c);
	curX++;
	if(curX > width) {
		curX -= width;
		curY++;
	}
}

void AnsiScreen::smartGoto(vector<int8_t> &dest, int x, int y) {
	// Not so smart for now
	const string &s = utils::format("\x1b[%d;%dH", y+1, x+1);
	dest.insert(dest.end(), s.begin(), s.end());			
	curX = x;
	curY = y;
}

// PETSCII SCREEN

int PetsciiScreen::update(std::vector<int8_t> &dest) {
	int orgSize = dest.size();

	if(outBuffer.size() > 0) {
		dest.insert(dest.end(), outBuffer.begin(), outBuffer.end());
		outBuffer.resize(0);
	}

	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			Tile &t1 = grid[x+y*width];
			if(t0 != t1) {					
				if(curY != y or curX != x)
					smartGoto(dest, x, y);
				//if(t0.fg != t1.fg || t0.bg != t1.bg)
				//	setColor(dest, t1.fg, t1.bg);
				putChar(dest, t1.c);
				t0 = t1;
			}
		}
	}

	return dest.size() - orgSize;
}

void PetsciiScreen::putChar(vector<int8_t> &dest, char c) {
	dest.push_back(c);
	curX++;
	if(curX > width) {
		curX -= width;
		curY++;
	}
}

void PetsciiScreen::smartGoto(vector<int8_t> &dest, int x, int y) {
	// Not so smart for now
	//const string &s = utils::format("\x1b[%d;%dH", y+1, x+1);
	//dest.insert(dest.end(), s.begin(), s.end());
	//int dx = x - curX;
	//int dy = y - dy;
	/*if(x < curX || y < curY) {
		dest.push_back(19);
		curX = curY = 0;
	}*/

	while(y > curY) {
		dest.push_back(17);
		curY++;
	}
	while(y < curY) {
		dest.push_back(145);
		curY--;
	}
	while(x > curX) {
		dest.push_back(29);
		curX++;
	}
	while(x < curX) {
		dest.push_back(157);
		curX--;
	}

	curX = x;
	curY = y;
}

void PetsciiScreen::put(int x, int y, const std::string &text) {
	for(unsigned int i=0; i<text.length(); i++) {
		Tile &t = grid[(x+i) + y * width];
		t.c = text[i];

		int *pc = std::find(begin(petsciiTable), end(petsciiTable), t.c);
		if(pc != end(petsciiTable))
			t.c = (pc - petsciiTable + 0x20);

		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
	}
}

// ANSI INPUT

int AnsiInput::getKey(int timeout) {

	std::chrono::milliseconds ms { 100 };

	while(true) {
		int rc = terminal.read(temp, temp.size());
		if(rc > 0) {
			LOGD("Got %d bytes", rc);
			for(int i=0; i<rc; i++)
				buffer.push(temp[i]);
			temp.resize(0);
		}
		if(buffer.size() > 0) {
			int8_t &c = buffer.front();
			if(c != 0x1b) {
				buffer.pop();
				return c;
			} else if(c == 0x1b && buffer.size() > 2) {
				buffer.pop();
				int8_t c2 = buffer.front();
				buffer.pop();
				int8_t c3 = buffer.front();
				buffer.pop();

				if(c2 == 0x5b) {
					switch(c3) {
					case 0x44:
						return KEY_LEFT;
					case 0x43:
						return KEY_RIGHT;
					case 0x41:
						return KEY_UP;
					case 0x42:
						return KEY_DOWN;
					case 0x35:
						buffer.pop();
						return KEY_PAGEUP;
					case 0x36:
						buffer.pop();
						return KEY_PAGEDOWN;
						
					}
				}

			}
		}
		std::this_thread::sleep_for(ms);
		if(timeout >= 0) {
			timeout -= 100;
			if(timeout < 0)
				return KEY_TIMEOUT;
		}
	}
}

int PetsciiInput::getKey(int timeout) {

	std::chrono::milliseconds ms { 100 };

	while(true) {
		int rc = terminal.read(temp, temp.size());
		if(rc > 0) {
			LOGD("Got %d bytes", rc);
			for(int i=0; i<rc; i++)
				buffer.push(temp[i]);
			temp.resize(0);
		}
		if(buffer.size() > 0) {
			int k = buffer.front();
			if(k >= 0x20)
				k = petsciiTable[k-0x20];
			buffer.pop();
			return k;
		}
		std::this_thread::sleep_for(ms);
		if(timeout >= 0) {
			timeout -= 100;
			if(timeout < 0)
				return KEY_TIMEOUT;
		}
	}
}