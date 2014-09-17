#include "ChipMachine.h"

using namespace std;
using namespace utils;
using namespace grappix;
using namespace tween;

namespace chipmachine {

void ChipMachine::setVariable(const std::string &name, int index, const std::string &val) {

	static unordered_map<string, TextField*> fields = {
		{ "main_title", &currentInfoField[0] },
		{ "main_composer", &currentInfoField[1] },
		{ "main_format", &currentInfoField[2] },
		{ "next_title", &nextInfoField[0] },
		{ "next_composer", &nextInfoField[1] },
		{ "next_format", &nextInfoField[2] },
		{ "exit_title", &prevInfoField[0] },
		{ "exit_composer", &prevInfoField[1] },
		{ "exit_format", &prevInfoField[2] },
		{ "enter_title", &outsideInfoField[0] },
		{ "enter_composer", &outsideInfoField[1] },
		{ "enter_format", &outsideInfoField[2] },
		{ "length_field", lengthField.get() },
		{ "time_field", timeField.get() },
		{ "song_field", songField.get() },
		{ "next_field", nextField.get() },
		{ "xinfo_field", xinfoField.get() },
		{ "search_field", searchField.get() },
		{ "top_status", topStatus.get() },
		{ "result_field", resultFieldTemplate.get() },
	};

	auto path = current_exe_path() + ":" + File::getAppDir();

	if(fields.count(name) > 0) {
		auto &f = (*fields[name]);
		if(index >= 4) {
			auto c = Color(stoll(val));
			if(name == "main_title" || name == "next_title")
				outsideInfoField[0].color = c;
			else if(name == "main_composer" || name == "next_composer")
				outsideInfoField[1].color = c;
			else if(name == "main_format" || name == "next_format")
				outsideInfoField[2].color = c;
			else if(name == "time_field")
				timeColor = c;
			// else if(name == "search_field") {
			// 	if(index == 5) {
			// 		formatColor = c;
			// 		return;
			// 	} else
			// 		searchColor = c;
			// }
			f.color = c;
			if(name == "result_field") {
				markColor = c;
				markTween = Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
			}
		} else {
			f[index-1] = stod(val);
		}
	} else
	if(name == "spectrum") {
		if(index <= 2)
			spectrumPos[index-1] = stol(val);
		else if(index == 3)
			spectrumWidth = stol(val);
		else if(index == 4)
			spectrumHeight = stod(val);
		else if(index == 5)
			spectrumColorMain = Color(stoll(val));
		else
			spectrumColorSearch = Color(stoll(val));
	} else
	if(name == "font") {

		File fontFile = File::findFile(path, val);

		if(fontFile.exists()) {
			font = Font(fontFile.getName(), 48, 512 | Font::DISTANCE_MAP);
			for(auto &f : fields) {
				f.second->setFont(font);
			}
			//listFont = Font(val, 32, 256);// | Font::DISTANCE_MAP);
			//resultFieldTemplate->setFont(listFont);
		}
	} else
	if(name == "list_font") {
		File fontFile = File::findFile(path, val);

		if(fontFile.exists()) {
			listFont = Font(fontFile.getName(), 32, 256);// | Font::DISTANCE_MAP);
			resultFieldTemplate->setFont(listFont);
		}
	} else
	if(name == "favicon") {
		favPos[index-1] = stol(val);
	} else
	if(name == "background") {
		bgcolor = stol(val);
	} else
	if(name == "stars") {
		starsOn = stol(val) != 0;
	} else
	if(name == "top_left") {
		tv0[index-1] = stol(val);
		songList.set_area(Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28));
	} else
	if(name == "down_right") {
		tv1[index-1] = stol(val);
		songList.set_area(Rectangle(tv0.x, tv0.y + 28, screen.width() - tv0.x, tv1.y - tv0.y - 28));
	} else
	if(name == "scroll") {
		switch(index) {
		case 1:
			scrollEffect.scrolly = stol(val);
			break;
		case 2:
			scrollEffect.scrollsize = stod(val);
			break;
		case 3:
			scrollEffect.scrollspeed = stol(val);
			break;
		case 4:
			{
				File fontFile = File::findFile(path, val);
				if(fontFile.exists())
					scrollEffect.set("font", fontFile.getName());
			}
			break;
		}
		LOGD("%d %f %d", scrollEffect.scrolly, scrollEffect.scrollsize, scrollEffect.scrollspeed);
	} else
	if(name == "hilight_color") {
		hilightColor = Color(stoll(val));
		markTween = Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
	} else
	if(name == "result_lines") {
		numLines = stol(val);
		songList.set_visible(numLines);
	}
}

} // namespace chipmachine
