#ifndef SONG_INFO_FIELD_H
#define SONG_INFO_FIELD_H

#include "TextField.h"
#include "SongInfo.h"

struct SongInfoField : public Renderable {

	SongInfoField() {
		fields.resize(3);
		fields[0] = std::make_shared<TextField>();
		fields[1] = std::make_shared<TextField>();
		fields[2] = std::make_shared<TextField>();
	}

	SongInfoField(grappix::Font &font, int x = 0, int y = 0, float sc = 1.0,
	              uint32_t color = 0xffffffff) {
		fields.resize(3);
		fields[0] = std::make_shared<TextField>(font, "", x, y, sc, color);
		fields[1] = std::make_shared<TextField>(font, "", x, y + 50 * sc, sc * 0.6, color);
		fields[2] = std::make_shared<TextField>(font, "", x, y + 100 * sc, sc * 0.4, color);
	}

	void setInfo(const SongInfo &info) {
		LOGD("Set info '%s' '%s' '%s'", info.title, info.composer, info.format);
		auto t = info.title;
		if(info.game != "" && info.title == "")
			t = info.game;
		else
			t = info.title; 
			
		if(t == "")
			t = utils::path_filename(utils::urldecode(info.path, ""));

		fields[0]->setText(t);
		fields[1]->setText(info.composer);
		fields[2]->setText(info.format);
	}

	int getWidth(int no) {
		return fields[no]->getWidth();
	}

	SongInfoField& operator=(const SongInfoField &other) {
		for(int i=0; i<3; i++)
			fields[i]->setText(other.fields[i]->getText());
		return *this;
	}


	virtual void render(std::shared_ptr<grappix::RenderTarget> target,  uint32_t delta) override {
		for(const auto &f : fields)
			f->render(target,  delta);
	}

	std::vector<std::shared_ptr<TextField>> fields;

	struct iterator {
		iterator(std::vector<std::shared_ptr<TextField>> fields, int index)
		    : fields(fields), index(index) {}
		iterator(const iterator &rhs) : fields(rhs.fields), index(rhs.index) {}

		bool operator!=(const iterator &other) const { return index != other.index; }

		TextField &operator*() { return *fields[index]; }

		const iterator &operator++() {
			++index;
			return *this;
		}

		std::vector<std::shared_ptr<TextField>> fields;
		int index;
	};

	iterator begin() { return iterator(fields, 0); }

	iterator end() { return iterator(fields, 3); }

	TextField &operator[](int i) { return *fields[i]; }
};

#endif // SONG_INFO_FIELD_H
