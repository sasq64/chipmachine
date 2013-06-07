#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include <string>
#include <vector>
#include <memory>

#include <stdint.h>

class Screen {
public:

	enum Color {
		WHITE, //15
		RED, //1
		GREEN, //2
		BLUE,//4
		ORANGE,
		BLACK, //0
		BROWN,
		PINK, //9
		DARK_GREY, //8
		GREY,
		LIGHT_GREEN, //10
		LIGHT_BLUE, //12
		LIGHT_GREY, //7
		PURPLE, //5
		YELLOW,//3
		CYAN //6
	};


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

	virtual int update(std::vector<int8_t>&) = 0;

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

	virtual int update(std::vector<int8_t> &dest) {

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
				}
			}
		}
		/*if(outBuffer.size()) {
			session.write(outBuffer);
			outBuffer.resize(0);
		}*/

		return dest.size() - orgSize;

	}

private:

	void setColor(std::vector<int8_t> &dest, int fg, int bg) {

		const std::string &s = utils::format("\x1b[%dm", fg + 30);
		dest.insert(dest.end(), s.begin(), s.end());			
	};

	void putChar(std::vector<int8_t> &dest, char c) {
		dest.push_back(c);
		curX++;
		if(curX > width) {
			curX -= width;
			curY++;
		}
	}

	void smartGoto(std::vector<int8_t> &dest, int x, int y) {
		// Not so smart for now
		//char temp[16];
		//std::vector<int8_t> temp(16);
		//int sz = dest.size();
		//dest.resize(sz+9);
		//sprintf((char*)&dest[sz], "\x1b[%d;%dH", x+1, y+1);

		const std::string &s = utils::format("\x1b[%d;%dH", x+1, y+1);
		dest.insert(dest.end(), s.begin(), s.end());			
		//dest.resize(sz+strlen((char*)&dest[sz]));

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