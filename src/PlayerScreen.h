#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

#include "SongInfo.h"

class PlayerScreen {
public:

	struct TextField {
		TextField(const std::string &text, float x, float y, float sc, uint32_t color) : text(text), pos(x, y), scale(sc), f {&pos.x, &pos.y, &scale, &r, &g, &b, &alpha} {
			r = ((color>>16)&0xff)/255.0;
			g = ((color>>8)&0xff)/255.0;
			b = (color&0xff)/255.0;
			alpha = ((color>>24)&0xff)/255.0;
		}
		std::string text;
		utils::vec2f pos;

		float scale;
		float r;
		float g;
		float b;
		float alpha;

		float& operator[](int i) { return *f[i]; }

		int size() { return 7; }
	private:
		float* f[8];
	};

	void render(uint32_t d) {
		for(auto &f : fields) {
			uint32_t c = ((int)(f->alpha*255)<<24) | ((int)(f->r*255)<<16) | ((int)(f->g*255)<<8) | (int)(f->b*255);
			grappix::screen.text(font, f->text, f->pos.x, f->pos.y, c, f->scale);
		}
	}

	void setFont(const grappix::Font &font) {
		this->font = font;
	}

	std::shared_ptr<TextField> addField(const std::string &text, float x = 0, float y = 0, float scale = 1.0, uint32_t color = 0xffffffff) {
		fields.push_back(std::make_shared<TextField>(text, x, y, scale, color));
		return fields.back();
	}
private:

	grappix::Font font;
	std::vector<std::shared_ptr<TextField>> fields;
};

struct SongInfoField {

		SongInfoField() {
		}

		SongInfoField(int x, int y, float sc = 1.0, uint32_t color = 0xffffffff) {
			fields[0] = std::make_shared<PlayerScreen::TextField>("", x, y, sc, color);
			fields[1] = std::make_shared<PlayerScreen::TextField>("", x, y+60*sc, sc*0.6, color);
			fields[2] = std::make_shared<PlayerScreen::TextField>("", x, y+100*sc, sc*0.4, color);
		}

		SongInfoField(PlayerScreen &ps, int x, int y, float sc = 1.0, uint32_t color = 0xffffffff) : fields { ps.addField("", x, y, sc, color), ps.addField("", x, y+30*sc, sc*0.6, color),  ps.addField("", x, y+50*sc, sc*0.4, color) } {}

		void setInfo(const SongInfo &info) {
			fields[0]->text = info.title;
			fields[1]->text = info.composer;
			fields[2]->text = info.format;
		}

		SongInfo getInfo() {
			SongInfo si;
			si.title = fields[0]->text;
			si.composer = fields[1]->text;
			si.format = fields[2]->text;
			return si;
		}

		std::shared_ptr<PlayerScreen::TextField> fields[3];

		PlayerScreen::TextField& operator[](int i) { return *fields[i]; }
		int size() { return 3; }

};
