


		static unordered_map<string, PlayerScreen::TextField*> fields = {
			{ "main_title", &currentInfoField[0] },
			{ "main_composer", &currentInfoField[1] },
			{ "main_format", &currentInfoField[2] },
			{ "next_title", &nextInfoField[0] },
			{ "next_composer", &nextInfoField[1] },
			{ "next_format", &nextInfoField[2] },
			{ "prev_title", &prevInfoField[0] },
			{ "prev_composer", &prevInfoField[1] },
			{ "prev_format", &prevInfoField[2] },
			{ "length_field", lengthField.get() },
			{ "time_field", timeField.get() },
			{ "song_field", songField.get() },
			{ "next_field", nextField.get() },
			{ "search_field", searchField.get() },
			{ "result_field", resultFieldTemplate.get() },
		};

		lua.registerFunction<void, string, uint32_t, string>("set_var", [=](string name, uint32_t index, string val) {
			LOGD("%s(%d) = %s", name, index, val);

			if(fields.count(name) > 0) {
				auto &f = (*fields[name]);
				if(index == 4) {
					f.color = Color(stoll(val));
				} else
					f[index-1] = stod(val);
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
			if(name == "top_left") {
				//currentInfoField.fields[0].color = stol(val);
				tv0[index-1] = stol(val);
			} else
			if(name == "down_right") {
				//currentInfoField.fields[0].color = stol(val);
				tv1[index-1] = stol(val);
			} else
			if(name == "font") {
				if(File::exists(val)) {
					auto font = Font(val, 32, 256 | Font::DISTANCE_MAP);
					mainScreen.setFont(font);
					searchScreen.setFont(font);
				}
			} else
			if(name == "result_lines") {
				numLines = stol(val);
				for(int i=0; i<resultField.size(); i++) {
					searchScreen.removeField(resultField[i]);
				}
				resultField.clear();
			}
		});

		Resources::getInstance().load_text("lua/init.lua", [=](const std::string &contents) {
			LOGD("init.lua");
			lua.load(R"(
				Settings = {}
			)");
			lua.load(contents);
			//LOGD(contents);
			lua.load(R"(
				for a,b in pairs(Settings) do 
					if type(b) == 'table' then
						for a1,b1 in ipairs(b) do
							set_var(a, a1, b1)
						end
					else
						set_var(a, 0, b)
					end
				end
			)");
		});
