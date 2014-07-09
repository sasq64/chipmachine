#ifndef TEXT_FIELD_H
#define TEXT_FIELD_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>


class TextField : public Renderable {
public:
	TextField(const grappix::Font &font, const std::string &text, float x, float y, float sc, uint32_t col) : pos(x, y), scale(sc), color(col), add(0), f {&pos.x, &pos.y, &scale, &color.r, &color.g, &color.b, &color.a, &add}, text(text), tlen(-1), font(font) {

	}

	utils::vec2f pos;

	float scale;
	grappix::Color color;
	float add;

	float& operator[](int i) { return *f[i]; }

	int size() const { return 8; }

	void setText(const std::string &t) {
		text = t;
		tlen = -1;
	}

	std::string getText() const { return text; }

	int getWidth() {
		if(tlen == -1)
			tlen = font.get_width(text, scale);
		return tlen;
	}

	void setFont(const grappix::Font &f) {
		font = f;
	}

	virtual void render(grappix::RenderTarget &target, uint32_t delta) override {
		if(color.a == 0.0)
			return;
		auto x = pos.x;
		auto y = pos.y;
		if(tlen == -1)
			tlen = font.get_width(text, scale);
		//if(x < 0) x = grappix::screen.width() - tlen + x;
		//if(y < 0) y = grappix::screen.height() + y;
		grappix::screen.text(font, text, x, y, color + add, scale);
	}

private:
	float* f[8];
	std::string text;
	mutable int tlen;
	grappix::Font font;
};


#endif // TEXT_FIELD_H
