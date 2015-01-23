#ifndef DIALOG_H
#define DIALOG_H


#include "renderable.h"

class Dialog : public Renderable {
public:
	Dialog(std::shared_ptr<grappix::RenderTarget> target, const grappix::Font &font, const std::string &text, float scale = 1.0F) : Renderable(target), font(font), textField(font, text), lineEdit(font) {
		auto size = font.get_size(text, scale);
		bounds.w = size.x + 20;
		bounds.h = size.y * 3;
		bounds.x = (target->width() - bounds.w) / 2;
		bounds.y = (target->height() - bounds.h) / 2;
		textField.pos = { bounds.x + 10, bounds.y + 10 };
		lineEdit.pos = { bounds.x + 10, bounds.y + size.y + 20 };
	}

	void on_ok(std::function<void(const std::string&)> cb) {
		//lineEdit.on_ok(cb);
		onOk = cb;
	}

	void on_key(uint32_t key) {

		if(key == grappix::Window::ENTER) {
			if(onOk)
				onOk(lineEdit.getText());
			remove();
		} else if(key == grappix::Window::ESCAPE) {
			remove();
		} else {
			lineEdit.on_key(key);
		}
	}

	virtual void setTarget(std::shared_ptr<grappix::RenderTarget> target) {
		textField.setTarget(target);
		lineEdit.setTarget(target);
		this->target = target;
	}

	virtual void render(uint32_t delta) override {
		target->rectangle(bounds, 0x80ffffff);
		textField.render(delta);
		lineEdit.render(delta);
	}

	std::function<void(const std::string&)> onOk;

	grappix::Font font;
	std::string text;

	grappix::Rectangle bounds;
	TextField textField;
	LineEdit lineEdit;

};


#endif // DIALOG_H
