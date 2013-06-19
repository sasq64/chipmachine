#ifndef TERMINAL_H
#define TERMINAL_H

#include <vector>
#include <stdint.h>

class Terminal {

public:
	typedef uint8_t Char;

	virtual int write(const std::vector<Char> &source, int len) = 0;
	virtual int read(std::vector<Char> &target, int len) = 0;

	virtual int getWidth() const { return -1; }
	virtual int getHeight() const { return -1; }
	virtual std::string getTermType() const { return ""; }

};


#endif // TERMINAL_H