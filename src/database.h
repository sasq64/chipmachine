#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>
#include <unordered_set>
#include <mutex>

using namespace std;
using namespace utils;


struct SongInfo {
	SongInfo(const std::string &path, const std::string &title = "", const std::string &composer = "", const std::string &format = "") :
		path(path), title(title), composer(composer), format(format) {}
	std::string path;
	std::string title;
	std::string composer;
	std::string format;
};

class MusicDatabase {
public:
	virtual void init() {}
	virtual bool ready() { return true; }
	virtual std::vector<SongInfo> search(const std::string &pattern) = 0;
};

class ModlandDatabase : public MusicDatabase {
public:
	ModlandDatabase() : db("modland.db") {}

	void init() override {

		if(db.has_table("song"))
			return;

		LOGD("Indexing...\n");

		static const unordered_set<string> ownDirFormats = {
			"Dreamcast Sound Format",
			"Gameboy Sound Format",
			"Nintendo Sound Format",
			"Nintendo SPC",
			"Video Game Music",
			"Ultra64 Sound Format",
			"Playstation Sound Format"
		};

		db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, composer STRING, format STRING, path STRING)");

		auto query = db.query("INSERT INTO song (title, game, composer, format, path) VALUES (?, ?, ?, ?, ?)");

		db.exec("BEGIN TRANSACTION");
		File file { "allmods.txt" };
		for(const auto &s : file.getLines()) {

			auto path = split(s, "\t")[1];
			auto parts = split(path, "/");
			auto l = parts.size();
			if(l < 3) {
				LOGD("%s", path);
				continue;
			}

			int i = l-1;
			bool ownDir = (ownDirFormats.count(parts[0]) > 0);

			string title = path_basename(parts[i--]);
			string game;

			if(ownDir && l >= 4) {
				game = parts[i--];
			}

			string composer = parts[i--];

			if(composer == "- unknown")
				composer = "?";

			if(composer.substr(0, 5) == "coop-")
				composer = parts[i--] + "+" + composer.substr(5);

			string format = parts[i--];

			query.bind(title, game, composer, format, path);
			
			query.step();
		}
		db.exec("COMMIT");
	}


	virtual std::vector<SongInfo> search(const std::string &pattern) override {

		lock_guard<mutex> guard(dbMutex);

		std::vector<SongInfo> songs;

		auto parts = split(pattern, " ");
		string qs = "SELECT path,title,composer,format FROM song WHERE ";
		for(int i=0; i<(int)parts.size(); i++) {
			if(i > 0)
				qs.append(" AND ");
			qs.append("path LIKE ?");
			parts[i] = format("%%%s%%",parts[i]);
		}
		qs.append(" COLLATE NOCASE");
		LOGD("Query: %s", qs);
		auto q = db.query<string, string, string,string>(qs, parts);
		//int i = 0;
		try {
			while(q.step()) {
				auto si = q.get<SongInfo>();
				si.path = "ftp://ftp.modland.com/pub/modules/" + si.path; 
				songs.push_back(si);
			}
		} catch(std::invalid_argument &e) {
			LOGD("Illegal shit");
		};

		return songs;
	}

private:
	std::mutex dbMutex;
	sqlite3db::Database db;
};


