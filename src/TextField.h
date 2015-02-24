#ifndef TEXT_FIELD_H
#define TEXT_FIELD_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>


class TextField : public Renderable {
public:

	TextField() : Renderable(grappix::screenptr), pos(0, 0), scale(1.0), color(0xffffffff), add(0), text(""), tsize(-1, -1) {
	}

	TextField(const grappix::Font &font, const std::string &text = "", float x = 0.0, float y = 0.0, float sc = 1.0, uint32_t col = 0xffffffff) : Renderable(grappix::screenptr), pos(x, y), scale(sc), color(col), add(0), text(text), tsize(-1, -1), font(font) {
	}


	union {
		float data[8];
		struct {
			utils::vec2f pos;
			float scale;
			grappix::Color color;
			float add;
		};
	};

	float& operator[](int i) { return data[i]; }
	//int size() const { return 8; }

	virtual void setText(const std::string &t) {
		text = t;
		tsize.x = -1;
	}

	virtual std::string getText() const { return text; }

	int getWidth() {
		if(tsize.x == -1)
			tsize = font.get_size(text, scale);
		return tsize.x;
	}

	void setFont(const grappix::Font &f) {
		font = f;
	}

	virtual void render(uint32_t delta) override {
		if(color.a == 0.0)
			return;
		auto x = pos.x;
		auto y = pos.y;
		if(tsize.x == -1)
			tsize = font.get_size(text, scale);
		//if(x < 0) x = grappix::screen.width() - tlen + x;
		//if(y < 0) y = grappix::screen.height() + y;
		target->text(font, text, x, y, color + add, scale);
	}

	float *begin() { return std::begin(data); }
	float *end() { return std::end(data); }


protected:
	//float* f[8];
	std::string text;
	mutable utils::vec2i tsize;
	grappix::Font font;
};



#endif // TEXT_FIELD_H
