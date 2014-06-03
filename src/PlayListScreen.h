
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"
#include "SongList.h"

#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

#include <grappix/gui/list.h>

using namespace tween;
using namespace grappix;
using namespace utils;

namespace chipmachine {

class PlayListScreen : public SongList::Renderer {
public:
};

}