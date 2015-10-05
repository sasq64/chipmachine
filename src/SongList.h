
//#include "MusicPlayerList.h"
//#include "TextScreen.h"
//#include "SongInfoField.h"

//#include <tween/tween.h>
#include <grappix/grappix.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

#include <grappix/gui/list.h>

namespace chipmachine {

class XSongList : public grappix::VerticalList /* ,public grappix::VerticalList::Renderer */ {

public:
	// SongList(std::function<void(Rectangle &rec, int y, uint32_t index, bool hilight)> renderFunc,
	// const Rectangle &area, int visibleItems) : renderFunc(renderFunc), area(area),
	// visibleItems(visibleItems), layout(area, visibleItems) {
	//}

	XSongList(Renderer *renderer, const Rectangle &area, int visibleItems)
	    : grappix::VerticalList(renderer, area, visibleItems) {}

	/*
renderer(renderer), area(area), visibleItems(visibleItems), layout(area, visibleItems)
	virtual void render_item(grappix::Rectangle &rec, int y, uint32_t index, bool hilight) override
{
	    auto info = getInfo(index);
	    auto text = format("%s / %s", info.title, info.composer);
	    auto c = hilight ? markColor : resultFieldTemplate->color;
	    grappix::screen.text(font, text, rec.x, rec.y, c, resultFieldTemplate->scale);
	};

	virtual SongInfo getInfo(uint32_t index) = 0;
*/
	bool on_key(grappix::Window::key k) {
		switch(k) {
		case Window::UP:
			select(selected() - 1);
			break;
		case Window::DOWN:
			select(selected() + 1);
			break;
		case Window::PAGEUP:
			pageup();
			break;
		case Window::PAGEDOWN:
			pagedown();
			break;
		default:
			return false;
		}
		return true;
	}
};
}