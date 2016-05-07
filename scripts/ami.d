import std.net.curl, std.stdio, std.xml, std.format, std.file, std.string, std.regex, std.conv;

// Look for next 'tag' and get text contained inside <tag[ args]>HERE</tag>
string getTagContents(string src, string tag) {
	auto startTag = "<" ~ tag;
	auto endTag = "</" ~ tag ~ ">";

	auto start = indexOf(src, startTag, 0, CaseSensitive.no);
	auto end = indexOf(src, '>', start);

	if(start < 0 || end < 0)
		return null;

	auto pos = end + 1;
	int depth = 0;
	while(true) {
		auto a = indexOf(src, startTag, pos, CaseSensitive.no);
		auto b = indexOf(src, endTag, pos, CaseSensitive.no);
		//writefln("FOUND %d %d", a, b);
		if(b < 0)
			return null;
		if(a < 0 || b < a) {
			// Found an endtag and no startTag
			if(depth > 0) {
				depth--;
				pos = b + endTag.length;
				continue;
			}
			else
				return src[end+1 .. b];
		}
		// Found a nesting start tag
		depth++;
		pos = a + startTag.length;
	}
	return src[end+1 .. $];
}

string stripTags(string src) {
	return replaceAll(src, regex(`<[^>]*>`), "");
}

string findTag(T)(string src, string tag, T[string] attributes, out long outPos) {

	auto startTag = "<" ~ tag;
	auto endTag = "</" ~ tag ~ ">";

	static auto r = regex(`(\w+)=(?:["']([^"']*)["'])`);

	long start = 0;
	while(true) {
		long pos;
		long end;
		try {
			pos = indexOf(src, startTag, start, CaseSensitive.no);
			end = indexOf(src, '>', pos);
		} catch (std.utf.UTFException e) {
			break;
		}
		if(pos < 0 && end < 0)
			return null;

		outPos = pos;

		auto s = src[pos .. end];
		int match = 0;
		if(attributes) {
			//writefln("Match %s", s);
			// Find all attributes in tag and compare against hash
			foreach(c; matchAll(s, r)) {
				//writefln("'%s' '%s' '%s'", c[0], c[1], c[2]);
				if(c[1] in attributes && attributes[c[1]] == c[2])
					match++;
			}
		}
		if(match == attributes.length) {
			return getTagContents(src[pos .. $], tag);
		}
		start = end+1;
	}
	return "";
}

string findTag(T)(string src, string tag, T[string] attributes) {
	long dummy;
	return findTag!T(src, tag, attributes, dummy);
}

string findTag(string src, string tag) {
	long dummy;
	string[string] x;
	return findTag(src, tag, x, dummy);
}

void main() {

	auto outFile = File("bitworld.txt", "w");
	int[string] ampcoll;

	auto metaRx = ctRegex!`(.*)\s+\(([^\)]*)\)\s+by\s+(.*)`;
	auto hrefRx = ctRegex!`href='([^']+)'`;
	auto imgRx = ctRegex!`<img\s+[^>]*src=["']([^"']+)["']`;
	auto akaRx = ctRegex!`<span.*>aka .*<\/span>`;

	foreach(id ; 1 .. 100000) {
		auto fileName = format("bitworld.%s.html", id);
		char[] data;
		if(!exists(fileName)) {
			break;
			//data = get(format("http://janeway.exotica.org.uk/release.php?id=%s", id));
			//std.file.write(fileName, data);
		} else
			data = cast(char[])read(fileName);

		string s = data.idup;

		auto contents = findTag(s, "div", [ "class" : "area symbolspace" ]);
		if(contents) {

			auto header = findTag(contents, "h1");
			header = replaceAll(header, akaRx, "");
			auto line = stripTags(header);
			auto c = matchFirst(line, metaRx);
			if(c) {
				//string[string] x;
				//long pos;
				//auto title = stripTags(findTag(contents, "a", x, pos));
				//writefln("TITLE: %s", title);
				//auto group = stripTags(findTag(contents[pos + title.length .. $], "a", x));
				//writefln("GROUP: %s", group);
				string type = c[2];
				if(startsWith(type, "Crack"))
					continue;
				if(endsWith(type, "emo") || endsWith(type, "ntro") || endsWith(type, "ackmo") || type == "Musicdisk") {
					long pos;
					string modland = "ftp://modland.ziphoid.com/pub/modules/";
					string amp = "http://amp.dascene.net/modules/";
					auto downloads = findTag(s, "div", ["id" : "downloads"]);
					string[] songs;
					foreach(c2; matchAll(downloads, hrefRx)) {
						string url = c2[1];
						url = replace(url, "\n", ";");
						//writefln(">>> %s", url);
						if(startsWith(url, modland))
							songs ~= ("M:" ~ url[modland.length .. $]);
						else if(startsWith(url, amp)) {
							songs ~= ("A:" ~ url[amp.length .. $]);
							ampcoll[url[amp.length .. $]] = 1;
						}
					}

					if(songs.length == 0)
						continue;

					writefln("RELEASE %s", id);

					auto screenies = "http://kestra.exotica.org.uk/files/screenies/";
					auto shots = findTag(s, "div", ["class" : "screenshots"]);
					//writeln(shots);
					//<img class="" alt="shot" src="http://kestra.exotica.org.uk/files/screenies/5000/5523.PNG">
					string[] screenshots;
					foreach(c3; matchAll(shots, imgRx)) {
						//writeln(c3[1]);
						if(startsWith(c3[1], screenies))
							screenshots ~= c3[1][screenies.length .. $];
					}
					//writefln("%s\n%s\n", line, c[0]);
					outFile.writeln(join([to!string(id), c[1], c[3], c[2], join(songs, ";"), join(screenshots, ";")], "\t"));
				}
			}
		}
/*
		string x = findTag(s, "span", ["class" : "title_td_icons"]);
		if(x) {
			//writeln(x);
			auto r = regex(`href='([^']+)'`);
			auto c = matchFirst(x, r);
			if(c)
				writefln(">>> %s", c[1]);
		}
*/
	}
	outFile.close();

	auto outFile2 = File("amp.txt", "w");
	foreach(a ; ampcoll.keys())
		outFile2.writeln(a);
	outFile2.close();


}
