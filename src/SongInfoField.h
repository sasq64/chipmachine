#ifndef SONG_INFO_FIELD_H
#define SONG_INFO_FIELD_H

#include "TextScreen.h"
#include "SongInfo.h"

struct SongInfoField {

	SongInfoField() {
	}

	SongInfoField(int x, int y, float sc = 1.0, uint32_t color = 0xffffffff) {
		fields[0] = std::make_shared<TextScreen::TextField>("", x, y, sc, color);
		fields[1] = std::make_shared<TextScreen::TextField>("", x, y+50*sc, sc*0.6, color);
		fields[2] = std::make_shared<TextScreen::TextField>("", x, y+100*sc, sc*0.4, color);
	}

	SongInfoField(TextScreen &ps, int x, int y, float sc = 1.0, uint32_t color = 0xffffffff) : fields { ps.addField("", x, y, sc, color), ps.addField("", x, y+30*sc, sc*0.6, color),  ps.addField("", x, y+50*sc, sc*0.4, color) } {}

	void setInfo(const SongInfo &info) {
		fields[0]->setText(info.title);
		fields[1]->setText(info.composer);
		fields[2]->setText(info.format);
	}

	SongInfo getInfo() {
		SongInfo si;
		si.title = fields[0]->getText();
		si.composer = fields[1]->getText();
		si.format = fields[2]->getText();
		return si;
	}

	std::shared_ptr<TextScreen::TextField> fields[3];

	TextScreen::TextField& operator[](int i) { return *fields[i]; }
	int size() { return 3; }

};


#endif // SONG_INFO_FIELD_H
