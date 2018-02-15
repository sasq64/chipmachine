#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#include "TextField.h"

class LineEdit : public TextField {
public:


	LineEdit() {
		cursorColor = grappix::Color(0xffffffff);
	}

	LineEdit(const grappix::Font &font, const std::string &text = "", float x = 0.F, float y = 0.F,
	         float sc = 1.0F, uint32_t col = 0xffffffff)
	    : TextField(font, text, x, y, sc, col), prompt(font, "", x, y, sc, col) {
		cursorColor = grappix::Color(0xffffffff); // grappix::Color(col)/2.0F;
		std::tie(cursorW, cursorH) = font.get_size("o", scale).to_tuple();
		xpos = getWidth();
		tpos = text.length();
	}
	
	virtual void setFont(const grappix::Font &f) override {
		TextField::setFont(f);
		prompt.setFont(f);
		std::tie(cursorW, cursorH) = font.get_size("o", scale).to_tuple();
	}
	
	
	void on_ok(std::function<void(const std::string &)> cb) { onOk = cb; }

	void on_key(uint32_t key) {
		using grappix::Window;
		
		if(key < 0x80 && key >= 0x20) {
			text.insert(tpos++, 1, key);
		} else {
			switch(key) {
			case keycodes::LEFT:
				if(tpos > 0)
					tpos--;
				break;
			case keycodes::RIGHT:
				if(tpos < text.size())
					tpos++;
				break;
			case keycodes::BACKSPACE:
				if(tpos > 0) {
					text.erase(--tpos, 1);
				}
				break;
			}
		}
		TextField::setText(text);
		
		auto ts = font.get_size(text.substr(0,tpos), scale);
		xpos = ts.x;
	}

	virtual void setText(const std::string &t) override {
		TextField::setText(t);
		xpos  = getWidth();	
		tpos = text.length();
	}

	void setPrompt(const std::string &p) { 
		prompt.setText(p);
	}

	virtual void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
	
		prompt.scale = scale;
		prompt.pos = pos;
		auto saved = pos;
		pos.x += (prompt.getWidth() * 1.2);
		
		int xm = cursorW * 0.2;
		
		target->rectangle(xpos + pos.x + xm, pos.y, cursorW - xm, cursorH * 0.8, cursorColor);
		prompt.render(target, delta);
		TextField::render(target, delta);
		pos = saved;
	}
	grappix::Color cursorColor;
	std::function<void(const std::string &)> onOk;
	
	TextField prompt;
	
	int cursorH;
	int cursorW;
	int tpos = 0;
	int xpos = 0;
};

#endif // LINE_EDIT_H
