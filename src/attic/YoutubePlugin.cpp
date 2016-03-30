
std::vector<std::string> split_if(const std::string &text, std::function<bool(char)> f) {
	vector<string> res;
	int start = 0;
	for(int i = 0; i < text.length(); i++) {
		if(f(text[i])) {
			res.push_back(text.substr(start, i - start));
			start = i;
		}
	}
	if(start < text.length())
		res.push_back(text.substr(start));
	return res;
}

if(path.find("youtube.com/") != string::npos || path.find("youtu.be/") != string::npos) {
	bool inId = false;
	string id;
	auto parts = split_if(path, [&inId](char c) -> bool {
		const static string ytchars =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
		bool found;
		if(inId) {
			found = (ytchars.find(c) == string::npos);
		} else {
			found = (ytchars.find(c) != string::npos);
		}
		if(found)
			inId = !inId;
		return found;
	});
	for(int i = 1; i < parts.size(); i += 2) {
		if(parts[i].length() == 11) {
			id = parts[i];
		}
	}
	File ytDir = File::getCacheDir() / "youtube";
	if(!ytDir.exists())
		makedir(ytDir);
	loadedFile = ytDir / (id + ".mp3");
	if(!File::exists(loadedFile)) {
		File ytExe = File::getExeDir() / "ffmpeg/youtube-dl.exe";
#ifdef _WIN32
		string cl = format("%s -o %s/%s.m4a -x \"%s\" --audio-format=mp3", ytExe.getName(),
		                   ytDir.getName(), id, path);
#else
		string cl = format("youtube-dl -o %s/%s.m4a -x \"%s\" --audio-format=mp3", ytDir.getName(),
		                   id, path);
#endif
		LOGD("Async convert youtube\n%s", cl);
		ytfuture = std::async(std::launch::async, [=]() -> string {
			int rc = system(cl.c_str());
			return id;
		});
		return;
	} else
		currentInfo.path = loadedFile;
}
