#ifndef DIALOG_H
#define DIALOG_H

#include "TextField.h"
#include "LineEdit.h"

class Dialog : public Renderable {
public:
	Dialog(std::shared_ptr<grappix::RenderTarget> target, const grappix::Font &font,
	       const std::string &text, float scale = 1.0F)
	    : font(font), textField(font, text), lineEdit(font) {
		auto size = font.get_size(text, scale);
		bounds.w = size.x + 20;
		bounds.h = size.y * 3;
		bounds.x = (target->width() - bounds.w) / 2;
		bounds.y = (target->height() - bounds.h) / 2;
		textField.pos = {bounds.x + 10, bounds.y + 10};
		lineEdit.pos = {bounds.x + 10, bounds.y + size.y + 20};
	}

	void on_ok(std::function<void(const std::string &)> cb) {
		// lineEdit.on_ok(cb);
		onOk = cb;
	}

	void on_key(uint32_t key) {

		LOGD("DIALOG: %d", key);
		
		if(key == keycodes::ENTER) {
			if(onOk)
				onOk(lineEdit.getText());
			Renderable::remove();
		} else if(key == keycodes::ESCAPE) {
			Renderable::remove();
		} else {
			lineEdit.on_key(key);
		}
	}

	virtual void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		target->rectangle(bounds, 0x80ffffff);
		textField.render(target, delta);
		lineEdit.render(target, delta);
	}

	std::function<void(const std::string &)> onOk;

	grappix::Font font;
	std::string text;

	grappix::Rectangle bounds;
	TextField textField;
	LineEdit lineEdit;
};

#endif // DIALOG_H
