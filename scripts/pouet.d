import std.net.curl, std.stdio, std.json, std.format, std.file, std.string;

void main() {

	auto outFile = File("pouet.txt", "w");
	foreach(id ; 1 .. 67268) {
		auto fileName = format("pouet.%s", id);
		char[] data;
		if(!exists(fileName)) {
			//data = get(format("http://api.pouet.net/v1/prod/?id=%s", id));
			//std.file.write(fileName, data);
		} else
			data = cast(char[])read(fileName);

		auto j = parseJSON(data);
		if("success" in j) {

			auto prod = j["prod"];
			auto name = prod["name"].str();
			string composer, link, groupName, platform;
	
			foreach(dl ; prod["downloadLinks"].array()) {
				auto type = dl["type"].str();
				if(type == "youtube") {
					link = dl["link"].str();
					break;
				}
				if(link == "" && indexOf(type, "youtube", CaseSensitive.no) >= 0)
					link = dl["link"].str();
			}

			if(link == "")
				continue;

			foreach(group ; prod["groups"].array()) {
				if(groupName != "")
					groupName ~= "+";
				groupName ~= group["name"].str();
			}

			foreach(user ; prod["credits"].array()) {
				auto role = user["role"].str();
				if(indexOf(role, "usic") >= 0) {
					if(composer != "")
						composer ~= "+";
					composer ~= user["user"]["nickname"].str();
				}
			}

			foreach(string index, JSONValue value ; prod["platforms"].object()) {
				platform = value["name"].str();
				break;
			}

			if(composer != "")
				groupName ~= (" (" ~ composer ~ ")");

			outFile.writefln("%s\t%s\t%s\tYoutube (%s)\t%s", name, "", groupName, platform, link);
		}
	}
	outFile.close();
}
