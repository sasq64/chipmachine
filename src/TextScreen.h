#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

//class TextScreen;

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

	//friend TextScreen;
};

class RenderSet {
public:

	void add(std::shared_ptr<Renderable> r) {
		fields.push_back(r);
	}

/*
	template <typename T, class = typename std::enable_if<std::is_compound<T>::value>::type>
	void add(const T &t) {
		for(int i=0; i<t.size(); i++)
			add(t.fields[i]);
	}
*/
	void remove(std::shared_ptr<Renderable> r) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(r.get() == it->get())
				it = fields.erase(it);
			else
				it++;
		}
	}

	void render(grappix::RenderTarget &target, uint32_t delta) {
		for(auto &r : fields)
			r->render(target, delta);
	}

	std::vector<std::shared_ptr<Renderable>> fields;

};

#if 0

class TextScreen {
public:


	void render(uint32_t d) {
		for(auto &f : fields) {
			if(f->color.a == 0.0)
				continue;
			auto x = f->pos.x;
			auto y = f->pos.y;
			if(f->tlen == -1)
				f->tlen = font.get_width(f->text, f->scale);
			//if(x < 0) x = grappix::screen.width() - f->tlen + x;
			//if(y < 0) y = grappix::screen.height() + y;
			grappix::screen.text(font, f->text, x, y, f->color + f->add, f->scale);
		}
	}

	int getWidth(const std::shared_ptr<TextField> &f) {
		if(f->tlen == -1)
			f->tlen = font.get_width(f->text, f->scale);
		return f->tlen;

	}

	void setFont(const grappix::Font &font) {
		this->font = font;
	}

	grappix::Font& getFont() {
		return font;
	}

	std::shared_ptr<TextField> addField(const std::string &text, float x = 0, float y = 0, float scale = 1.0, uint32_t color = 0xffffffff) {
		fields.push_back(std::make_shared<TextField>(text, x, y, scale, color));
		return fields.back();
	}

	void removeField(const std::shared_ptr<TextField> &field) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(field.get() == it->get())
				it = fields.erase(it);
			else
				it++;
		}
	}

private:

	grappix::Font font;
	std::vector<std::shared_ptr<TextField>> fields;
};

#endif

#endif // TEXT_SCREEN_H
