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

class LocalTerminal : public Terminal {
public:
	virtual int write(const std::vector<Char> &source, int len) { return fwrite(&source[0], 1, len, stdout); }
	virtual int read(std::vector<Char> &target, int len) { return fread(&target[0], 1, len, stdin); }
};

extern LocalTerminal localTerminal;

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
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,

		KEY_TIMEOUT = 0xffff
	};

	typedef uint16_t Char;


	Console(Terminal &terminal) : terminal(terminal), fgColor(-1), bgColor(-1), width(40), height(24), curFg(-1), curBg(-1) {
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
	virtual void putChar(Char c);
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
	virtual void impl_translate(Char &c) {}

	struct Tile {
		Tile(Char c = ' ', int fg = -1, int bg = -1) : fg(fg), bg(bg), c(c) {}
		bool operator==(const Tile &o) const {
   			return (fg == o.fg && bg == o.bg && c == o.c);
  		}
  		bool operator!=(const Tile &o) const {
  			return !(*this == o);
  		}

		int fg;
		int bg;
		Char c;
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

	// The current REAL colors of the console (cursor)
	int curFg;
	int curBg;

};

Console *createLocalConsole();

class AnsiConsole : public Console {
public:
	AnsiConsole(Terminal &terminal);

	void putChar(Char c);

protected:

	virtual void impl_color(int fg, int bg) override;
	virtual void impl_gotoxy(int x, int y) override;
	virtual int impl_handlekey() override;
	virtual void impl_clear() override;
	//virtual void impl_translate(Char &c) override;

};

class PetsciiConsole : public Console {
public:

	enum {
		STOP = 3,
		WHITE = 5,
		DOWN = 0x11,
		RVS_ON = 0x12,
		HOME = 0x13,
		DEL = 0x14,
		RIGHT = 0x1d,
		RUN = 131,
		F1 = 133,
		F2 = 134,
		F3 = 135,
		F4 = 136,
		F5 = 137,
		F6 = 138,
		F7 = 139,
		F8 = 140,
		SHIFT_RETURN = 0x8d,
		UP = 0x91,
		RVS_OFF = 0x92,
		CLEAR = 0x93,
		INS = 0x94,
		LEFT = 0x9d
	};


	PetsciiConsole(Terminal &terminal) : Console(terminal) {
		resize(width, height);
	}

	virtual void putChar(Char c);

protected:

	virtual void impl_color(int fg, int bg) override;
	virtual void impl_gotoxy(int x, int y) override;
	virtual int impl_handlekey() override;
	virtual void impl_clear() override;
	virtual void impl_translate(Char &c) override;
};

class Editor {
public:
	Editor(std::shared_ptr<Console> screen) : screen(screen) {}
	virtual void put(std::vector<uint8_t> &data) = 0;
private:
	std::shared_ptr<Console> screen;
};


#endif // TEXT_SCREEN_H