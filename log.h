
#include "utils.h"
#include <string>


namespace logging {

typedef enum {
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3
} LogLevel;

void log(const std::string &text);
void log(LogLevel level, const std::string &text);

template <class... A>
void log(const std::string &fmt, A... args) {
	log(utils::format(fmt, args...));
};

template <class... A>
void log(LogLevel level, const std::string &fmt, A... args) {
	log(level, utils::format(fmt, args...));
};

}