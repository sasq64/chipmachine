#pragma once

#include <coreutils/utils.h>
#include <grappix/grappix.h>
#include <grappix/gui/list.h>

#include <memory>


namespace chipmachine {

class Icon : public Renderable {
public:
	Icon() {}

	Icon(std::shared_ptr<grappix::Texture> tx, float x, float y, float w, float h) : texture(tx), rec(x, y, w, h) {}

	Icon(const image::bitmap &bm, int x = 0, int y = 0) : rec(x, y, bm.width(), bm.height()) {
		setBitmap(bm);
	}
	
	Icon(const image::bitmap &bm, float x, float y, float w, float h) : rec(x, y, w, h) {
		setBitmap(bm);
	}

	void render(std::shared_ptr<grappix::RenderTarget> target, uint32_t delta) override {
		if(!texture || (color >> 24) == 0)
			return;
		target->draw(*texture, rec.x, rec.y, rec.w, rec.h, nullptr, color);
	}
	
	void setBitmap(const image::bitmap &bm, bool filter = false) {
		texture = std::make_shared<grappix::Texture>(bm);
		rec.w = bm.width();
		rec.h = bm.height();
		glBindTexture(GL_TEXTURE_2D, texture->id());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	}
	
	void clear() {
		texture = nullptr;
	}

	void setArea(const grappix::Rectangle &r) { rec = r; }

	grappix::Color color{0xffffffff};
	grappix::Rectangle rec;

private:
	std::shared_ptr<grappix::Texture> texture;
};

} // namespace
