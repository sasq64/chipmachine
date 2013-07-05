#ifndef LOGGING_H
#define LOGGING_H

#include "format.h"
#include <string>


namespace logging {

enum LogLevel {
	VERBOSE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 3,
	ERROR = 4,
	OFF = 100
};

void log(const std::string &text);
void log(const LogLevel level, const std::string &text);
void log2(const char *fn, int line, const LogLevel level, const std::string &text);

template <class... A>
void log(const std::string &fmt, const A& ... args) {
	log(utils::format(fmt, args...));
};

template <class... A>
void log(LogLevel level, const std::string &fmt, const A& ... args) {
	log(level, utils::format(fmt, args...));
};

template <class... A>
void log2(const char *fn, int line, LogLevel level, const std::string &fmt, const A& ... args) {
	log2(fn, line, level, utils::format(fmt, args...));
};

void setLevel(LogLevel level);
void setOutputFile(const std::string &fileName);

#define LOGV(...) logging::log2(__FILE__, __LINE__, logging::VERBOSE, __VA_ARGS__)
#define LOGD(...) logging::log2(__FILE__, __LINE__, logging::DEBUG, __VA_ARGS__)
#define LOGI(...) logging::log2(__FILE__, __LINE__, logging::INFO, __VA_ARGS__)
#define LOGW(...) logging::log2(__FILE__, __LINE__, logging::WARNING, __VA_ARGS__)
#define LOGE(...) logging::log2(__FILE__, __LINE__, logging::ERROR, __VA_ARGS__)

}

#endif // LOGGING_H