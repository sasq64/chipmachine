#pragma once

#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>

/** Get basename, but also handles urlencoded path names */
inline std::string getBaseName(std::string const& filename)
{
    auto fnstart = std::string::npos;
    while (true) {
        fnstart = filename.find_last_of("%/\\", fnstart);
        if ((fnstart == std::string::npos) || (filename[fnstart] != '%')) break;
        if ((filename[fnstart + 1] == '2') && (filename[fnstart + 2] == 'f')) {
            fnstart += 2;
            break;
        }
        fnstart--;
    }

    return filename.substr(fnstart + 1);
}

inline std::tuple<std::string, std::string> getTypeAndBase(std::string const& filename)
{
    constexpr char const* knownExts[] = {
        "jpn", "mdat", "mod", "smp", "smpl", "sng",
    };

    auto base = getBaseName(filename);

    auto firstDot = base.find_first_of('.');
    auto lastDot = base.find_last_of('.');
    if (firstDot != std::string::npos) {
        auto prefix = base.substr(0, firstDot);
        auto suffix = base.substr(lastDot + 1);

        auto it = std::lower_bound(std::begin(knownExts), std::end(knownExts),
                                   prefix.c_str(),
                                   [](char const* a, char const* b) -> bool {
                                       return strcmp(a, b) < 0;
                                   });
        if (it != std::end(knownExts) && strcmp(*it, prefix.c_str()) == 0) return std::make_tuple(prefix, base.substr(firstDot+1));
		return std::make_tuple(suffix, base.substr(0, lastDot));
    }
    return std::make_tuple("", base);
}

inline std::string getTypeFromName(std::string const& filename)
{
	return std::get<0>(getTypeAndBase(filename));
}

