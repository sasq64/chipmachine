
#include <string>
#include <vector>
#include <unordered_map>

class sqlite3;
class SongDatabase;

class IncrementalQuery {
public:

	IncrementalQuery(sqlite3 *db, SongDatabase *sdb) : db(db), sdb(sdb), searchLimit(40) {}

	void addLetter(char c);
	void removeLast();
	const std::string getString();
	const std::vector<std::string> &getResult() const;
	int numHits() const;
private:
	void search();

	sqlite3 *db;
	SongDatabase *sdb;
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> result;
};

class SongDatabase {
public:

	SongDatabase(const std::string &name);
	void generateIndex();
	IncrementalQuery find() {
		return IncrementalQuery(db, this);
	}

	void search(const char *query);
	void search(const std::string &query, std::vector<std::string> &result, int searchLimit);
private:

	void addSubStrings(const char *words, std::unordered_map<std::string, std::vector<int>> &stringMap, int index);
	sqlite3 *db;

	std::unordered_map<std::string, std::vector<int>> titleMap;
	std::unordered_map<std::string, std::vector<int>> composerMap;
	std::vector<std::pair<std::string, int>> titles;
	std::vector<std::pair<std::string, int>> composers;

};

