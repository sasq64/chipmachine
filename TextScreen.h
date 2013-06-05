#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include <string>
#include <vector>
#include <memory>

#include <stdint.h>

class Screen {
public:
	Screen() : fgColor(-1), bgColor(-1), width(40), height(20) {
		resize(width, height);
	}

	//virtual void gotoxy(int ax, int ay) { x = ax; y = ay; }
	//virtual std::tuple<int, int> getxy() { return std::make_tuple(x,y); }

	//virtual void write(const std::string &text) {
	//	fragments.push_back(Fragment(x, y, color, text));
	//}

	virtual void put(int x, int y, const std::string &text) {
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

	virtual void setFg(int fg) {
		fgColor = fg;
	}

	virtual void setBg(int bg) {
		bgColor = bg;
	}

	virtual void resize(int w, int h) {
		width = w;
		height = h;
		grid.resize(w*h);
		oldGrid.resize(w*h);
	}

	virtual const std::vector<int8_t> &update() = 0;
protected:

	struct Tile {
		Tile(char c = ' ', int fg = -1, int bg = -1) : fg(fg), bg(bg), c(c) {}
		bool operator==(const Tile &o) const {
   			return (fg == o.fg && bg == o.bg && c == o.c);
  		}
  		bool operator!=(const Tile &o) const {
  			return !(*this == o);
  		}

		int fg;
		int bg;
		char c;
	};
	//typedef std::tuple<int, int, char> Tile;

	std::vector<Tile> grid;
	std::vector<Tile> oldGrid;

	int fgColor;
	int bgColor;

	int width;
	int height;

};

class AnsiScreen : public Screen {
public:
	AnsiScreen() : Screen(), curX(0), curY(0) {
		outBuffer = { '\x1b', '[', '2', 'J' };
		//sprintf((char*)&outBuffer[0], "\x1b""2J");
	};

	virtual void update(TelnetServer::Session &session) {

		for(int y = 0; y<height; y++) {
			for(int x = 0; x<width; x++) {
				Tile &t0 = oldGrid[x+y*width];
				Tile &t1 = grid[x+y*width];
				if(t0 != t1) {					
					if(curY != y or curX != x)
						smartGoto(x, y);
					if(t0.fg != t1.fg || t0.bg != t1.bg)
						setColor(t1.fg, t1.bg);
					putChar(t1.c);
				}
			}
		}
		if(outBuffer.size()) {
			session.write(outBuffer);
			outBuffer.resize(0);
		}

	}

private:

	void setColor(int fg, int bg) {

	};

	void putChar(char c) {
		outBuffer.push_back(c);
		curX++;
		if(curX > width) {
			curX -= width;
			curY++;
		}
	}

	void smartGoto(int x, int y) {
		// Not so smart for now
		//char temp[16];
		//std::vector<int8_t> temp(16);
		int sz = outBuffer.size();
		outBuffer.resize(sz+9);
		sprintf((char*)&outBuffer[sz], "\x1b[%d;%dH", x+1, y+1);
		outBuffer.resize(sz+strlen((char*)&outBuffer[sz]));

		curX = x;
		curY = y;
	}

private:
	std::vector<int8_t> outBuffer;
	int curX;
	int curY;

};

class Editor {
public:
	Editor(std::shared_ptr<Screen> screen) : screen(screen) {}
	virtual void put(std::vector<int8_t> &data) = 0;
private:
	std::shared_ptr<Screen> screen;
};


class LineEditor : public Editor {
public:
	void update() {
		//auto &buffer = session.getBuffer();
		//for(auto &b : buffer {
		//}
	}

};

#endif // TEXT_SCREEN_H