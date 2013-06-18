#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include "Terminal.h"

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>
#include <stdint.h>

class DummyTerminal : public Terminal {
public:
	virtual int write(const std::vector<Char> &source, int len) { return -1; }
	virtual int read(std::vector<Char> &target, int len) { return -1; }
};

extern DummyTerminal dummyTerminal;

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


	Screen(Terminal &terminal = dummyTerminal) : terminal(terminal), fgColor(-1), bgColor(-1), width(80), height(50) {
		resize(width, height);
	}

	virtual void clear();
	virtual void put(int x, int y, const std::string &text);
	virtual void setFg(int fg) { fgColor = fg; }
	virtual void setBg(int bg) { bgColor = bg; }
	virtual void resize(int w, int h);

	virtual int update(std::vector<int8_t>&) = 0;
	virtual void flush();

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

	Terminal &terminal;

	std::vector<Tile> grid;
	std::vector<Tile> oldGrid;

	int fgColor;
	int bgColor;

	int width;
	int height;

};

class AnsiScreen : public Screen {
public:
	AnsiScreen(Terminal &terminal) : Screen(terminal), curX(-1), curY(-1) {
		outBuffer = { '\x1b', '[', '2', 'J' };
	};

	virtual int update(std::vector<int8_t> &dest);

private:

	void setColor(std::vector<int8_t> &dest, int fg, int bg);
	void putChar(std::vector<int8_t> &dest, char c);
	void smartGoto(std::vector<int8_t> &dest, int x, int y);
	
	std::vector<int8_t> outBuffer;
	int curX;
	int curY;

};

class PetsciiScreen : public Screen {
public:
	PetsciiScreen(Terminal &terminal) : Screen(terminal), curX(-1), curY(-1) {
		outBuffer = { 19, 14 };
	}
	virtual int update(std::vector<int8_t> &dest);
	virtual void put(int x, int y, const std::string &text);

private:

	void setColor(std::vector<int8_t> &dest, int fg, int bg);
	void putChar(std::vector<int8_t> &dest, char c);
	void smartGoto(std::vector<int8_t> &dest, int x, int y);
	
	std::vector<int8_t> outBuffer;
	int curX;
	int curY;
};

class AnsiInput {
public:
	AnsiInput(Terminal &terminal) : terminal(terminal), pos(0) {}		

	enum {
		KEY_ESCAPE = 0x1b,
		KEY_BACKSPACE = 0x7f,
		KEY_LEFT = 256,
		KEY_UP,
		KEY_RIGHT,
		KEY_DOWN,

		KEY_TIMEOUT = 0xffff
	};

	int getKey(int timeout = -1);

private:
	Terminal &terminal;
	int pos;
	std::vector<int8_t> temp;
	std::queue<int8_t> buffer;

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