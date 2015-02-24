#ifndef SONG_INFO_FIELD_H
#define SONG_INFO_FIELD_H

#include "TextField.h"
#include "SongInfo.h"

struct SongInfoField {

	SongInfoField() {
		fields.resize(3);
		fields[0] = std::make_shared<TextField>();
		fields[1] = std::make_shared<TextField>();
		fields[2] = std::make_shared<TextField>();
	}

	SongInfoField(grappix::Font &font, int x = 0, int y = 0, float sc = 1.0, uint32_t color = 0xffffffff) {
		fields.resize(3);
		fields[0] = std::make_shared<TextField>(font, "", x, y, sc, color);
		fields[1] = std::make_shared<TextField>(font, "", x, y+50*sc, sc*0.6, color);
		fields[2] = std::make_shared<TextField>(font, "", x, y+100*sc, sc*0.4, color);
	}

	//SongInfoField(TextScreen &ps, int x, int y, float sc = 1.0, uint32_t color = 0xffffffff) : fields { ps.addField("", x, y, sc, color), ps.addField("", x, y+30*sc, sc*0.6, color),  ps.addField("", x, y+50*sc, sc*0.4, color) } {}

	void setInfo(const SongInfo &info) {
		LOGD("Set info '%s' '%s' '%s'", info.title, info.composer, info.format);
		fields[0]->setText(info.title);
		fields[1]->setText(info.composer);
		fields[2]->setText(info.format);
		path = info.path;
	}

	SongInfo getInfo() {
		SongInfo si;
		si.title = fields[0]->getText();
		si.composer = fields[1]->getText();
		si.format = fields[2]->getText();
		si.path = path;
		return si;
	}

	bool operator==(const SongInfoField &other) const {
		return other.path == path;
	}

	std::vector<std::shared_ptr<TextField>> fields;


	struct iterator  {
		iterator(std::vector<std::shared_ptr<TextField>> fields, int index) : fields(fields), index(index) {}
		iterator(const iterator& rhs) : fields(rhs.fields), index(rhs.index) {}

		bool operator!= (const iterator& other) const {
			return index != other.index;
		}

		TextField& operator* () {
			return *fields[index];
		}

		const iterator& operator++ () {
			++index;
			return *this;
		}

		std::vector<std::shared_ptr<TextField>> fields;
		int index;
	};

	iterator begin() {
		return iterator(fields, 0);
	}

	iterator end() {
		return iterator(fields, 3);
	}

	TextField& operator[](int i) { return *fields[i]; }
	//int size() const { return 3; }
	std::string path;

};


#endif // SONG_INFO_FIELD_H
