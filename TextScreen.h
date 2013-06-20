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


	Console(Terminal &terminal = dummyTerminal) : terminal(terminal), fgColor(-1), bgColor(-1), width(40), height(24) {
		/*int w = terminal.getWidth();
		int h = terminal.getHeight();
		if(w > 0) width = w;
		if(h > 0) height = h;
		resize(width, height);*/
	}

	virtual int getKey(int timeout);

	virtual void clear();
	virtual void put(int x, int y, const std::string &text);
	virtual void setFg(int fg) { fgColor = fg; }
	virtual void setBg(int bg) { bgColor = bg; }
	virtual void resize(int w, int h);
	virtual void flush();
	virtual void putChar(char c);
	virtual void moveCursor(int x, int y);
	virtual void fill(int x, int y, int width, int height);

	int getWidth() { return width; }
	int getHeight() { return height; }

protected:

	// Functions that needs to be implemented by real console implementations

	virtual void impl_color(int fg, int bg) = 0;
	virtual void impl_gotoxy(int x, int y) = 0;
	virtual int impl_handlekey() = 0;
	virtual void impl_clear() = 0;

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

	// Outgoing raw data to the terminal
	std::vector<uint8_t> outBuffer;

	// Incoming raw data from the terminal
	std::queue<uint8_t> inBuffer;

	// The contents of the screen after next flush.
	std::vector<Tile> grid;
	// The contents on the screen now. The difference is used to
	// send characters to update the console.
	std::vector<Tile> oldGrid;

	int fgColor;
	int bgColor;

	int width;
	int height;

	// The current REAL cursor position on the console
	int curX;
	int curY;

};

class AnsiConsole : public Console {
public:
	AnsiConsole(Terminal &terminal) : Console(terminal) {
		resize(width, height);
	};

protected:

	virtual void impl_color(int fg, int bg) override;
	virtual void impl_gotoxy(int x, int y) override;
	virtual int impl_handlekey() override;
	virtual void impl_clear() override;

};

class PetsciiConsole : public Console {
public:
	PetsciiConsole(Terminal &terminal) : Console(terminal) {
		resize(width, height);
	}
	virtual void put(int x, int y, const std::string &text) override;

protected:

	virtual void impl_color(int fg, int bg) override;
	virtual void impl_gotoxy(int x, int y) override;
	virtual int impl_handlekey() override;
	virtual void impl_clear() override;
};

class Editor {
public:
	Editor(std::shared_ptr<Console> screen) : screen(screen) {}
	virtual void put(std::vector<uint8_t> &data) = 0;
private:
	std::shared_ptr<Console> screen;
};


#endif // TEXT_SCREEN_H