
#include <string>
#include <vector>
#include <unordered_map>

class sqlite3;
class SongDatabase;

class database_exception : public std::exception {
public:
	database_exception(const char *txt) {
		msg = txt;
	}
	virtual const char *what() const throw() { return msg.c_str(); }
private:
	std::string msg;
};

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};

//#define THROW(e, args...) throw e(args, __FILE__, __LINE__)

class IncrementalQuery {
public:

	IncrementalQuery(sqlite3 *db, SongDatabase *sdb) : db(db), sdb(sdb), searchLimit(10000), lastStart(-1), lastSize(-1) {}

	void addLetter(char c);
	void removeLast();
	void clear();
	const std::string getString();
	const std::vector<std::string> &getResult(int start, int size);
	int numHits() const;
	std::string getFull(int pos);
private:
	void search();

	sqlite3 *db;
	SongDatabase *sdb;
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> oldParts;
	std::vector<int> firstResult;
	std::vector<int> finalResult;
	std::vector<std::string> textResult;
	int lastStart;
	int lastSize;
};

class SongDatabase {
public:

	SongDatabase(const std::string &name);
	~SongDatabase();
	void generateIndex();
	IncrementalQuery find() {
		return IncrementalQuery(db, this);
	}

	void search(const char *query);
	void search(const std::string &query, std::vector<int> &result, unsigned int searchLimit);

	const std::string &getTitle(int index) { return titles[index].first; }
	const std::string &getComposer(int index) { return composers[titles[index].second].first; }

private:

	void addSubStrings(const char *words, std::unordered_map<uint16_t, std::vector<int>> &stringMap, int index);
	sqlite3 *db;

	std::unordered_map<uint16_t, std::vector<int>> titleMap;
	std::unordered_map<uint16_t, std::vector<int>> composerMap;
	std::vector<std::pair<std::string, int>> titles;
	std::vector<std::pair<std::string, int>> composers;

};

