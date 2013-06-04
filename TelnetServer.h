#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include "utils.h"

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <tuple>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;


class TelnetServer { //: public Terminal {
public:

	class Session {
	public:
		void write(const std::vector<int8_t> &data, int len = -1) {
			socket->send(&data[0], data.size());
		}

		void write(const std::string &text) {
			socket->send(text.c_str(), text.length());
		}

		template <class... A>
		void write(const std::string &fmt, A... args) {
			std::string s = utils::format(fmt, args...);
			write(s);
		}

		Session(NL::Socket *socket = nullptr) : socket(socket), state(NORMAL) {}

		NL::Socket *getSocket() { return socket; }
		void handleIndata(std::vector<int8_t> &buffer);

		char getChar() {
			if(inBuffer.size() > 0) {
				char c = inBuffer[0];
				inBuffer.erase(inBuffer.begin());
				return c;
			}
			return 0;
		}

		bool hasChar() {
			return inBuffer.size() > 0;
		}

		std::string getLine() {
			auto f = std::find(inBuffer.begin(), inBuffer.end(), LF);
			if(f != inBuffer.end()) {
				std::string line = std::string(inBuffer.begin(), f);
				if(line[line.length()-1] == LF);
					line.resize(line.length()-1);
				inBuffer.erase(inBuffer.begin(), ++f);
				return line;
			}
			return "";
		}

		std::vector<int8_t> getBuffer() {
			return inBuffer;
		}

		bool hasLine() {
			return std::find(inBuffer.begin(), inBuffer.end(), LF) != inBuffer.end();
		}

	private:
		NL::Socket *socket;

		enum State {
			NORMAL,
			CR_READ,
			FOUND_IAC,
			OPTION,
			SUB_OPTION,
			FOUND_IAC_SUB	
		};

		State state;
		int8_t option;
		std::vector<int8_t> optionData;
		std::vector<int8_t> inBuffer;

		void setOption(int opt, int val) {
			LOGD("Set option %d %d", opt, val);
		}


		void handleOptionData() {
			LOGD("optionData %d : %d bytes", option, optionData.size());
			optionData.resize(0);
		}
	};

	enum {
		IAC = -1, //ff
		DONT = -2, //fe
		DO = -3, //fd
		WONT = -4, //fc
		WILL = -5,	 //fb
		SB = -6,
		GA = -7,
		EL = -8,
		EC = -9,
		AYT = -10,
		AO = -11,
		IP = -12,
		BRK = -13,
		DM = -14,
		NOP = -15,
		SE = -16,

		LF = 10,
		CR = 13
	};

	enum {
		TRANSMIT_BINARY = 0,
		ECHO = 1,
		SUPRESS_GO_AHEAD = 3,
		TERMINAL_TYPE = 24,
		WINDOW_SIZE = 31
	};

	typedef std::function<void(Session&, const std::vector<std::string>&)> CMDFunction;
	typedef std::function<void(Session&)> SessionFunction;

	TelnetServer(int port);
	void setPrompt(std::string);
	void addCommand(const std::string &cmd, CMDFunction callback);
	void run();
	void runThread();

	void setConnectCallback(SessionFunction callback) {
		connectCallback = callback;
	}

	Session &getSession(NL::Socket* socket) {

		for(Session &u : sessions) {
			if(u.getSocket() == socket)
				return u;
		}
		return no_session;
	}

	Session no_session;


private:


	TelnetInit *init;
	NL::Socket socketServer;
	std::string prompt;

	class OnAccept : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	class OnRead : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};


	class OnDisconnect : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	OnAccept onAccept;
	OnRead onRead;
	OnDisconnect onDisconnect;

	NL::SocketGroup group;

	std::thread mainThread;

	std::unordered_map<std::string, CMDFunction> callBacks;
	SessionFunction connectCallback;

	std::vector<Session> sessions;
};



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

	virtual void update(TelnetServer::Session &session) = 0;
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
	void update(TelnetServer::Session &session) {
		//auto &buffer = session.getBuffer();
		//for(auto &b : buffer {
		//}
	}

};


#endif // TELNET_SERVER_H
