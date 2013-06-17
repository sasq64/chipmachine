#include "log.h"
#include "TextScreen.h"
#include "utils.h"

using namespace std;

DummyTerminal dummyTerminal;

void Screen::clear() {
	for(auto &t : grid) {
		t.c = 0x20;
		t.fg = t.bg = -1;
	}
}

void Screen::put(int x, int y, const string &text) {
	//for(int yy = 0; yy<height; y++) {
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
}

void Screen::flush() {
	vector<int8_t> temp;
	int rc = update(temp);
	if(rc > 0)
		terminal.write(temp, rc);
}


// ANSISCREEN

int AnsiScreen::update(std::vector<int8_t> &dest) {

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