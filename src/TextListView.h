#ifndef TEXTLISTVIEW_H
#define TEXTLISTVIEW_H

#include <bbsutils/console.h>
#include <string>

class TextListView
{
public:
    TextListView(bbs::Console& console, int height, int width = -1)
        : height(height), width(width), console(console)
    {
        xpos = console.getCursorX();
        ypos = console.getCursorY();
        bgColor = console.getBg();
        spaces = std::string(width, ' ');
    }

    bool putKey(int key)
    {
        using namespace bbs;
        bool moved = false;
        if (key != Console::KEY_TIMEOUT) {
            moved = true;
            switch (key) {
            case Console::KEY_UP: mark--; break;
            case Console::KEY_DOWN: mark++; break;
            case Console::KEY_PAGEDOWN:
            case Console::KEY_F7: mark += height; break;
            case Console::KEY_PAGEUP:
            case Console::KEY_F5: mark -= height; break;
            default: moved = false;
            }
        }

        if (moved) {
            if (mark < 0) mark = 0;
            if (mark >= length) mark = length - 1;

            if (mark < start) start = mark;
            if (mark >= start + height) start = mark - height + 1;
        }
        return moved;
    }

    void setCallback(std::function<void(bbs::Console&, int index, bool)> f)
    {
        render_cb = f;
    }

    void setLength(int l)
    {
        length = l;
        if (start >= length - height) start = length - height;
        if (start < 0) start = 0;
        if (mark >= length) mark = length - 1;
    }

    int marked() { return mark; }

    void refresh()
    {
        console.showCursor(false);
        if (start == -1 || length == 0) return;
        for (int i = 0; i < height; i++) {
            int pos = i + start;
            console.setColor(bbs::Console::CURRENT_COLOR, bgColor);
            console.put(xpos, i + ypos, spaces);
            if (pos < length) {
                console.moveCursor(xpos, i + ypos);
                render_cb(console, pos, pos == mark);
            }
        }
    }

private:
    int mark = 0;
    int start = 0;
    int xpos = 0;
    int ypos = 0;
    int height;
    int width;
    int bgColor;
    bbs::Console& console;
    std::function<void(bbs::Console&, int, bool)> render_cb;
    int length = 0;
    std::string spaces;
};

#endif // TEXTLISTVIEW_H
