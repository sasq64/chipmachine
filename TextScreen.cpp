#include "log.h"
#include "TextScreen.h"
#include "utils.h"

#include <array>
#include <algorithm>

using namespace std;

/** PETSCII->ASCII 0x20 -> 0xFF */

static int petsciiTable[] = {
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
	0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,0x0030,0x0031,
	0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,
	0x003C,0x003D,0x003E,0x003F,0x0040,0x0061,0x0062,0x0063,0x0064,0x0065,
	0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,
	0x007A,0x005B,0x00A3,0x005D,0x2191,0x2190,0x2501,0x0041,0x0042,0x0043,
	0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,
	0x004E,0x004F,0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
	0x0058,0x0059,0x005A,0x253C,0xF12E,0x2502,0x2592,0xF139,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x000A,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x00A0,
	0x258C,0x2584,0x2594,0x2581,0x258F,0x2592,0x2595,0xF12F,0xF13A,0xF130,
	0x251C,0xF134,0x2514,0x2510,0x2582,0x250C,0x2534,0x252C,0x2524,0x258E,
	0x258D,0xF131,0xF132,0xF133,0x2583,0x2713,0xF135,0xF136,0x2518,0xF137,
	0xF138,0x2501,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,
	0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,0x0050,0x0051,0x0052,
	0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x253C,0xF12E,
	0x2502,0x2592,0xF139,0x00A0,0x258C,0x2584,0x2594,0x2581,0x258F,0x2592,
	0x2595,0xF12F,0xF13A,0xF130,0x251C,0xF134,0x2514,0x2510,0x2582,0x250C,
	0x2534,0x252C,0x2524,0x258E,0x258D,0xF131,0xF132,0xF133,0x2583,0x2713,
	0xF135,0xF136,0x2518,0xF137,0x2592
};

static vector<uint8_t> petsciiColors = { 5, 28, 30, 31, 129, 144, 149, 150, 151, 152, 153, 154, 155, 156, 158, 159 };

DummyTerminal dummyTerminal;

void Console::clear() {
	for(auto &t : grid) {
		t.c = 0x20;
		t.fg = t.bg = BLACK;
	}
	for(auto &t : oldGrid) {
		t.c = 0x20;
		t.fg = t.bg = BLACK;
	}
	impl_clear();
	curX = curY = 0;
}

void Console::fill(int x, int y, int w, int h) {
	for(int yy = y; yy < y + h; yy++)
		for(int xx = x; xx < x + w; xx++) {
			Tile &t = grid[xx + width * yy];
			if(fgColor >= 0) t.fg = fgColor;
			if(bgColor >= 0) t.bg = bgColor;
			t.c = 0x20;
		}
}

void Console::put(int x, int y, const string &text) {
	if(y >= height)
		return;
	for(unsigned int i=0; i<text.length(); i++) {

		if(x+i >= width)
			return;

		Tile &t = grid[(x+i) + y * width];
		//LOGD("put to %d %d",x+i,y);	
		t.c = text[i];
		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
	}
}

void Console::resize(int w, int h) {
	width = w;
	height = h;
	LOGD("Resize");
	grid.resize(w*h);
	oldGrid.resize(w*h);
	clear();
}

void Console::flush() {

	int w = terminal.getWidth();
	int h = terminal.getHeight();
	if((w > 0 && w != width) || (h > 0 && h != height)) {
		resize(w, h);
	}

	int saveX = curX;
	int saveY = curY;

	//int saveFg = fgColor;
	//int saveBg = bgColor;

	//LOGD("update");
	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			Tile &t1 = grid[x+y*width];
			if(t0 != t1) {
				//LOGD("diff in %d %d",x,y);			
				if(curY != y or curX != x) {
					impl_gotoxy(x, y);
					curX = x;
					curY = y;
				}
				if(t1.fg != curFg || t1.bg != curBg) {
					impl_color(t1.fg, t1.bg);
					curFg = t1.fg;
					curBg = t1.bg;
				}
				putChar(t1.c);
				t0 = t1;
			}
		}
	}

	if(curFg != fgColor || curBg != bgColor) {
		impl_color(fgColor, bgColor);
		curFg = fgColor;
		curBg = bgColor;
	}

	impl_gotoxy(saveX, saveY);
	curX = saveX;
	curY = saveY;

	if(outBuffer.size() > 0) {
		LOGD("OUTBYTES: [%02x]", outBuffer);
		terminal.write(outBuffer, outBuffer.size());
		outBuffer.resize(0);
	}

}

void Console::putChar(char c) {
	//LOGD("putchar %02x", c);
	outBuffer.push_back(c);
	curX++;
	if(curX >= width) {
		curX -= width;
		curY++;
	}
}

void Console::moveCursor(int x, int y) {
	impl_gotoxy(x, y);
	if(outBuffer.size() > 0) {
		terminal.write(outBuffer, outBuffer.size());
		outBuffer.resize(0);
	}
	curX = x;
	curY = y;
}

int Console::getKey(int timeout) {

	std::chrono::milliseconds ms { 100 };

	std::vector<uint8_t> temp;
	temp.reserve(16);

	while(true) {
		int rc = terminal.read(temp, 16);
		if(rc > 0) {
			LOGD("Got %d bytes", rc);
			for(int i=0; i<rc; i++) {
				LOGD("Pushing %d", (int)temp[i]);
				inBuffer.push(temp[i]);
			}
		}
		if(inBuffer.size() > 0) {
			LOGD("Size %d", inBuffer.size());
			return impl_handlekey();
		}

		std::this_thread::sleep_for(ms);
		if(timeout >= 0) {
			timeout -= 100;
			if(timeout < 0)
				return KEY_TIMEOUT;
		}
	}
}


// ANSIS

void AnsiConsole::impl_color(int fg, int bg) {
	const string &s = utils::format("\x1b[%dm", fg + 30);
	outBuffer.insert(outBuffer.end(), s.begin(), s.end());			
};

void AnsiConsole::impl_clear() {
	for(auto x : vector<uint8_t> { '\x1b', '[', '2', 'J' })
		outBuffer.push_back(x);
}


void AnsiConsole::impl_gotoxy(int x, int y) {
	// Not so smart for now
	const string &s = utils::format("\x1b[%d;%dH", y+1, x+1);
	outBuffer.insert(outBuffer.end(), s.begin(), s.end());
}

int AnsiConsole::impl_handlekey() {
	uint8_t c = inBuffer.front();
	inBuffer.pop();
	if(c != 0x1b) {	
		LOGD("Normal key %d", (int)c);	
		return c;
	} else {
		if(inBuffer.size() > 0) {
			uint8_t c2 = inBuffer.front();
			inBuffer.pop();
			uint8_t c3 = inBuffer.front();
			inBuffer.pop();

			if(c2 == 0x5b || c2 == 0x4f) {
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
					inBuffer.pop();
					return KEY_PAGEUP;
				case 0x36:
					inBuffer.pop();
					return KEY_PAGEDOWN;
					
				}
			}
		}
		return c;
	}
}


// PETSCII

void PetsciiConsole::put(int x, int y, const std::string &text) {
	if(y >= height)
		return;

	for(unsigned int i=0; i<text.length(); i++) {

		if(x+i >= width)
			return;

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

void PetsciiConsole::impl_color(int fg, int bg) {
	if(bg == -1)
		exit(0);
	if(bg != BLACK) {
		outBuffer.push_back(18);
		outBuffer.push_back(petsciiColors[bg]);
	} else {
		outBuffer.push_back(146);
		outBuffer.push_back(petsciiColors[fg]);
	}
}

void PetsciiConsole::impl_clear() {
	outBuffer.push_back(147);
}

void PetsciiConsole::impl_gotoxy(int x, int y) {

	if(curX - x > x) {
		outBuffer.push_back(0x8d);
		if(curBg != BLACK) {
			curFg = curBg;
			curBg = BLACK;
		}
		curX=0;
		curY++;	
	}

	while(y > curY) {
		outBuffer.push_back(0x11);
		curY++;
	}
	while(y < curY) {
		outBuffer.push_back(0x91);
		curY--;
	}

	while(x > curX) {
		outBuffer.push_back(0x1d);
		curX++;
	}

	while(x < curX) {
		outBuffer.push_back(0x9d);
		curX--;
	}
}

int PetsciiConsole::impl_handlekey() {
	int k = inBuffer.front();
	inBuffer.pop();
	switch(k) {
	case 13 :
		return KEY_ENTER;
	case 20 :
		return KEY_BACKSPACE;
	case 17 :
		return KEY_DOWN;
	case 145 :
		return KEY_UP;
	}
	if(k >= 0x20) 
		k = petsciiTable[k-0x20];
	return k;
}




#ifdef UNIT_TEST

#include "catch.hpp"
#include <sys/time.h>

class TestTerminal : public Terminal {
public:
	virtual int write(const std::vector<Char> &source, int len) { 
		for(int i=0; i<len; i++)
			outBuffer.push_back(source[i]);
		return len;
	}
	virtual int read(std::vector<Char> &target, int len) { 
		int rc = -1;//outBuffer.size();
		//target.insert(target.back, outBuffer.begin(), outBuffer.end());
		//outBuffer.resize(0);
		return rc;
	}
	std::vector<Char> outBuffer;


};

TEST_CASE("console::basic", "Console") {

	TestTerminal terminal;
	AnsiConsole console { terminal };

	//1b 5b 32 4a 1b 5b 32 3b 32 48  74 65 73 74 69 6e 67

	console.put(1,1, "testing");
	console.flush();
	LOGD("%02x", terminal.outBuffer);

}

#endif