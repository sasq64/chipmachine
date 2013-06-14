
#include "log.h"

#include <stdio.h>
#include <string.h>

namespace logging {

static LogLevel defaultLevel = DEBUG;
static FILE *logFile = nullptr;

void log(const std::string &text) {
	log(defaultLevel, text);
}

void log(LogLevel level, const std::string &text) {
	if(level >= defaultLevel) {
		fwrite(text.c_str(), 1, text.length(), stdout);
		char c = text[text.length()-1];
		if(c != '\n' && c != '\r')
			putc('\n', stdout);			
	}
	if(logFile) {
		fwrite(text.c_str(), 1, text.length(), logFile);
		char c = text[text.length()-1];
		if(c != '\n' && c != '\r')
			putc('\n', logFile);
		fflush(logFile);
	}
}

void log2(const char *fn, int line, LogLevel level, const std::string &text) {
	char temp[2048];
	sprintf(temp, "[%s:%d] ", fn, line);
	log(level, std::string(temp).append(text));
}

void setLevel(LogLevel level) {
	defaultLevel = level;
}
void setOutputFile(const std::string &fileName) {
	if(logFile)
		fclose(logFile);
	logFile = fopen(fileName.c_str(), "wb");
}


}