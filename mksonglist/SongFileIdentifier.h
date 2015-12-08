#ifndef SONG_FILE_IDENTIFIER_H
#define SONG_FILE_IDENTIFIER_H

#include <string>
#include "SongInfo.h"

bool identify_song(SongInfo &info, std::string ext = "");

#endif // SONG_FILE_IDENTIFIER_H
