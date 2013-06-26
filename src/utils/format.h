#ifndef FORMAT_H
#define FORMAT_H

#include <stdint.h>
//#include <typeinfo>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

namespace utils {

// FORMAT

class Printable {
public:
	virtual std::string toText() const = 0;
};


char parse_format(std::stringstream &ss, std::string &fmt);

//void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<int8_t> &bytes);
//void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<uint8_t> &bytes);
void format_stream(std::stringstream &ss, std::string &fmt, const Printable &printable);

/*void format_stream(std::stringstream &ss, std::string &fmt, const slice<std::vector<int8_t>::const_iterator> &bytes);
void format_stream(std::stringstream &ss, std::string &fmt, const slice<std::vector<uint8_t>::const_iterator> &bytes);

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const slice<T> &arg) {
if(parse_format(ss, fmt)) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			ss << b;
			first = false;
		}
	}
}*/

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const T *arg) {
	if(parse_format(ss, fmt))
		ss << arg;
}

//void format_stream(std::stringstream &ss, std::string &fmt, const int arg);
//void format_stream(std::stringstream &ss, std::string &fmt, const int arg) { format_stream(ss, fmt, (int)arg); }

template<template <typename, typename> class Container, class V, class A> void format_stream(std::stringstream &ss, std::string &fmt, Container<V, A> const& arg) {
//template <template<class> class C, class T> void format_stream(std::stringstream &ss, std::string &fmt, C<T> const& arg) {
	if(parse_format(ss, fmt)) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			ss << b;
			first = false;
		}
	}
}


template<template <typename, typename> class Container, class A> void format_stream(std::stringstream &ss, std::string &fmt, Container<char, A> const& arg) {
	char letter;
	if((letter = parse_format(ss, fmt))) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			if(letter == 'd' || letter == 'x')
				ss << (int)(b&0xff);
			else
				ss << b;
			first = false;
		}
	}
}

template<template <typename, typename> class Container, class A> void format_stream(std::stringstream &ss, std::string &fmt, Container<unsigned char, A> const& arg) {
	char letter;
	if((letter = parse_format(ss, fmt))) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			if(letter == 'd' || letter == 'x')
				ss << (int)(b&0xff);
			else
				ss << b;
			first = false;
		}
	}
}

template<template <typename, typename> class Container, class A> void format_stream(std::stringstream &ss, std::string &fmt, Container<signed char, A> const& arg) {
	char letter;
	if((letter = parse_format(ss, fmt))) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			if(letter == 'd' || letter == 'x')
				ss << (int)(b&0xff);
			else
				ss << b;
			first = false;
		}
	}
}

/*
template <template<class> class C> void format_stream(std::stringstream &ss, std::string &fmt, C<unsigned char> const& arg) {
	char letter;
	if((letter = parse_format(ss, fmt))) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			if(letter == 'd' || letter == 'x')
				ss << (int)b;
			else if(letter == 'c')
				ss << (char)b;
			else if(letter == 'f')
				ss << (float)b;
			else
				ss << b;
			first = false;
		}
	}
} */

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const T& arg) {
	char letter;
	if((letter = parse_format(ss, fmt))) {
		ss << arg;
	}
}

/*template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<T>& arg) {
	if(parse_format(ss, fmt)) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			ss << b;
			first = false;
		}
	}
} */


template <class A, class... B>
void format_stream(std::stringstream &ss, std::string &fmt, const A &head, const B& ... tail)
{
	format_stream(ss, fmt, head);
	format_stream(ss, fmt, tail...);
}

template <class T>
std::string format(const std::string &fmt, const T& arg) {
	std::string fcopy = fmt;
	std::stringstream ss;
	format_stream(ss, fcopy, arg);
	ss << fcopy;
	return ss.str();  
}

std::string format(const std::string &fmt);

template <class A, class... B> std::string format(const std::string &fmt, const A &head, const B& ... tail)
{
	std::string fcopy = fmt;
	std::stringstream ss;
	format_stream(ss, fcopy, head);
	format_stream(ss, fcopy, tail...);
	ss << fcopy;
	return ss.str();
}


};

#endif // FORMAT_H
