
#include "log.h"

#include <stdio.h>
#include <string.h>

namespace logging {

static LogLevel defaultLevel = DEBUG;

void log(const std::string &text) {
	log(defaultLevel, text);
}

void log(LogLevel level, const std::string &text) {
	if(level >= DEBUG) {
		fwrite(text.c_str(), 1, text.length(), stdout);
		char c = text[text.length()-1];
		if(c != '\n' && c != '\r')
			putc('\n', stdout);			
	}
}

void log2(const char *fn, int line, LogLevel level, const std::string &text) {
	char temp[2048];
	sprintf(temp, "[%s:%d] ", fn, line);
	log(level, std::string(temp).append(text));
}


}