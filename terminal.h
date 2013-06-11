#ifndef TERMINAL_H
#define TERMINAL_H



class Terminal {

public:
	typedef int8_t Char;

	virtual int write(const std::vector<Char> &source, int len) = 0;
	virtual int read(std::vector<Char> &target, int len) = 0;
};


#endif // TERMINAL_H