#include "ChipMachine.h"

using namespace utils;

using tween::Tween;

namespace chipmachine {

void ChipMachine::setVariable(const std::string &name, int index, const std::string &val) {

	// The text fields that are configurable from lua
	static std::unordered_map<std::string, TextField *> fields = {
	    {"main_title", &currentInfoField[0]},
	    {"main_composer", &currentInfoField[1]},
	    {"main_format", &currentInfoField[2]},

	    {"next_title", &nextInfoField[0]},
	    {"next_composer", &nextInfoField[1]},
	    {"next_format", &nextInfoField[2]},

	    {"exit_title", &prevInfoField[0]},
	    {"exit_composer", &prevInfoField[1]},
	    {"exit_format", &prevInfoField[2]},

	    {"enter_title", &outsideInfoField[0]},
	    {"enter_composer", &outsideInfoField[1]},
	    {"enter_format", &outsideInfoField[2]},

	    {"length_field", &lengthField},
	    {"time_field", &timeField},
	    {"song_field", &songField},
	    {"next_field", &nextField},
	    {"xinfo_field", &xinfoField},
	    {"search_field", &searchField},
	    {"command_field", &commandField},
	    {"top_status", &topStatus},
	    {"toast_field", &toastField},
	    {"result_field", &resultFieldTemplate}};

	auto path = workDir;

	if(fields.count(name) > 0) {
		auto &f = (*fields[name]);
		if(index >= 4) {
			auto c = Color(stoll(val));
			if(name == "time_field")
				timeColor = c;
			f.color = c;
			if(name == "result_field") {
				markColor = c;
				markTween =
				    Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
				markTween.start();
			}
		} else {
			auto x = stod(val);
			if(index == 1)
				f.pos.x = x;
			else if(index == 2)
				f.pos.y = x;
			else
				f.scale = x;
		}
	} else if(name == "spectrum") {
		if(index <= 2)
			spectrumPos[index - 1] = stol(val);
		else if(index == 3)
			spectrumWidth = stol(val);
		else if(index == 4)
			spectrumHeight = stod(val);
		else if(index == 5)
			spectrumColorMain = Color(stoll(val));
		else
			spectrumColorSearch = Color(stoll(val));
	} else if(name == "font") {

		File fontFile = File::findFile(path, val);

		if(fontFile.exists()) {
			font = grappix::Font(fontFile.getName(), 48, 512 | grappix::Font::DISTANCE_MAP);
			for(auto &f : fields) {
				f.second->setFont(font);
			}
		} else
			throw file_not_found_exception(fontFile.getName());

	} else if(name == "list_font") {
		File fontFile = File::findFile(path, val);

		if(fontFile.exists()) {
			listFont = grappix::Font(fontFile.getName(), 32, 256); // | Font::DISTANCE_MAP);
			resultFieldTemplate.setFont(listFont);
		} else
			throw file_not_found_exception(fontFile.getName());

	} else if(name == "favicon") {
		favPos[index - 1] = stol(val);
	} else if(name == "background") {
		bgcolor = stol(val);
	} else if(name == "stars") {
		starsOn = stol(val) != 0;
	} else if(name == "top_left") {
		topLeft[index - 1] = stol(val);
		updateLists();
	} else if(name == "down_right") {
		downRight[index - 1] = stol(val);
		updateLists();
	} else if(name == "scroll") {
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
		case 4: {
			File fontFile = File::findFile(path, val);
			if(fontFile.exists())
				scrollEffect.set("font", fontFile.getName());
		} break;
		}
	} else if(name == "hilight_color") {
		hilightColor = Color(stoll(val));
		markTween = Tween::make().sine().repeating().from(markColor, hilightColor).seconds(1.0);
		markTween.start();
	} else if(name == "result_lines") {
		numLines = stol(val);
		songList.setVisible(numLines);
		commandList.setVisible(numLines);
	}
}

} // namespace chipmachine
