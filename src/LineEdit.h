#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#include "TextField.h"

class LineEdit : public TextField {
public:
	LineEdit() : TextField() { cursorColor = grappix::Color::WHITE; }

	LineEdit(const grappix::Font &font, const std::string &text = "", float x = 0.F, float y = 0.F,
	         float sc = 1.0F, uint32_t col = 0xffffffff)
	    : TextField(font, "", x, y, sc, col) {
		cursorColor = grappix::Color::WHITE; // grappix::Color(col)/2.0F;
		this->text = prompt + text;
		// tween::make_tween().sine().repeating().to(cursorColor,
		// grappix::Color::WHITE).seconds(1.7);
	}

	void on_ok(std::function<void(const std::string &)> cb) { onOk = cb; }

	void on_key(uint32_t key) {
		if(key < 0x100)
			text = text + (char)key;
		else {
			switch(key) {
			case grappix::Window::BACKSPACE:
				if(text.length() > prompt.length())
					text = text.substr(0, text.length() - 1);
				break;
			}
		}
		tsize = font.get_size(text, scale);
	}

	virtual void setText(const std::string &t) override {
		text = prompt + t;
		tsize.x = -1;
	}

	virtual std::string getText() const override { return text.substr(prompt.length()); }

	void setPrompt(const std::string &p) { prompt = p; }

	virtual void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		TextField::render(target, delta);
		getWidth();
		// LOGD("REC %s %d %d", text, tsize.x, tsize.y);
		target->rectangle(pos.x + tsize.x + 2, pos.y + 2, 10, tsize.y - 4, cursorColor);
	}
	grappix::Color cursorColor;
	std::function<void(const std::string &)> onOk;

	std::string prompt = ">";
};

#endif // LINE_EDIT_H
