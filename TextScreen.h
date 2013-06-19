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

class Console {
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

	enum {
		KEY_ESCAPE = 0x1b,
		KEY_BACKSPACE = 0x7f,
		KEY_LEFT = 256,
		KEY_UP,
		KEY_RIGHT,
		KEY_DOWN,
		KEY_PAGEUP,
		KEY_PAGEDOWN,
		KEY_HOME,
		KEY_END,
		KEY_ENTER,
		KEY_TAB,

		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,

		KEY_TIMEOUT = 0xffff
	};


	Console(Terminal &terminal = dummyTerminal) : terminal(terminal), fgColor(-1), bgColor(-1), width(80), height(50) {
		int w = terminal.getWidth();
		int h = terminal.getHeight();
		if(w > 0) width = w;
		if(h > 0) height = h;
		resize(width, height);
	}

	virtual int getKey(int timeout);

	virtual void clear();
	virtual void put(int x, int y, const std::string &text);
	virtual void setFg(int fg) { fgColor = fg; }
	virtual void setBg(int bg) { bgColor = bg; }
	virtual void resize(int w, int h);
	virtual int update(std::vector<uint8_t>&);
	virtual void flush();
	virtual void putChar(std::vector<uint8_t> &dest, char c);

protected:

	virtual void impl_color(std::vector<uint8_t> &dest, int fg, int bg) = 0;
	virtual void impl_gotoxy(std::vector<uint8_t> &dest, int x, int y) = 0;
	virtual int impl_handlekey(std::queue<uint8_t> &buffer) = 0;
	virtual void impl_clear(std::vector<uint8_t> &dest) = 0;

	struct Tile {
		Tile(int c = ' ', int fg = -1, int bg = -1) : fg(fg), bg(bg), c(c) {}
		bool operator==(const Tile &o) const {
   			return (fg == o.fg && bg == o.bg && c == o.c);
  		}
  		bool operator!=(const Tile &o) const {
  			return !(*this == o);
  		}

		int fg;
		int bg;
		int c;
	};

	Terminal &terminal;

	std::vector<uint8_t> outBuffer;
	std::queue<uint8_t> inBuffer;

	std::vector<Tile> grid;
	std::vector<Tile> oldGrid;

	int fgColor;
	int bgColor;

	int width;
	int height;

	int curX;
	int curY;

	int pos;
};

class AnsiConsole : public Console {
public:
	AnsiConsole(Terminal &terminal) : Console(terminal) {
		outBuffer = { '\x1b', '[', '2', 'J' };
	};

private:

	virtual void impl_color(std::vector<uint8_t> &dest, int fg, int bg) override;
	virtual void impl_gotoxy(std::vector<uint8_t> &dest, int x, int y) override;
	virtual int impl_handlekey(std::queue<uint8_t> &buffer) override;
	virtual void impl_clear(std::vector<uint8_t> &dest) override;

};

class PetsciiConsole : public Console {
public:
	PetsciiConsole(Terminal &terminal) : Console(terminal) {
		outBuffer = { 147, 19, 14 };
	}
	virtual void put(int x, int y, const std::string &text) override;

private:

	virtual void impl_color(std::vector<uint8_t> &dest, int fg, int bg);
	virtual void impl_gotoxy(std::vector<uint8_t> &dest, int x, int y);
	virtual int impl_handlekey(std::queue<uint8_t> &buffer);
	virtual void impl_clear(std::vector<uint8_t> &dest) override {};
};

class Editor {
public:
	Editor(std::shared_ptr<Console> screen) : screen(screen) {}
	virtual void put(std::vector<uint8_t> &data) = 0;
private:
	std::shared_ptr<Console> screen;
};


#endif // TEXT_SCREEN_H