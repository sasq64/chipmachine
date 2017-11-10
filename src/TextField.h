#ifndef TEXT_FIELD_H
#define TEXT_FIELD_H

#include <grappix/gui/renderable.h>

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

#include <cstddef>

#pragma GCC diagnostic ignored "-Winvalid-offsetof"
class TextField : public Renderable {
public:
	TextField() : pos(0, 0), scale(1.0), color(0xffffffff), add(0), align(0), text(""), tsize(-1, -1) {
		static_assert(offsetof(TextField, add) - offsetof(TextField, color) == 7 * 4,
		              "Fields not packed");
	}

	TextField(const grappix::Font &font, const std::string &text = "", float x = 0.0, float y = 0.0,
	          float sc = 1.0, uint32_t col = 0xffffffff)
	    : pos(x, y), scale(sc), color(col), add(0), align(0), text(text), tsize(-1, -1), font(font) {}

#pragma pack(4)
	grappix::Color color;
	utils::vec2f pos;
	float scale;
	float add;
	float align;
#pragma pack()

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

	int getHeight() {
		if(tsize.x == -1)
			tsize = font.get_size(text, scale);
		return tsize.y;
	}

	virtual void setFont(const grappix::Font &f) { font = f; }

	virtual void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		if(color.a == 0.0)
			return;
		if(tsize.x == -1 || oldScale != scale) {
			tsize = font.get_size(text, scale);
			oldScale = scale;
		}
		target->text(font, text, pos.x - tsize.x * align, pos.y, color + add, scale);
	}

	struct iterator {
		iterator(TextField *field, int index) : data(&field->color[0]), index(index) {}
		iterator(const iterator &rhs) : data(rhs.data), index(rhs.index) {}

		bool operator!=(const iterator &other) const { return index != other.index; }

		float &operator*() { return data[index]; }

		const iterator &operator++() {
			++index;
			return *this;
		}
		float* data;
		int index;
	};

	iterator begin() { return iterator(this, 0); }

	iterator end() { return iterator(this, 9); }

protected:
	std::string text;
	mutable utils::vec2i tsize;
	grappix::Font font;
	float oldScale = -1;
};

#endif // TEXT_FIELD_H
