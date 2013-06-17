
#include <string>
#include <vector>
#include <unordered_map>

class sqlite3;
class SongDatabase;

class database_exception : public std::exception {
public:
	database_exception(const char *txt, const char *file, int line) {
		LOGD("%s %d", file, line);
		msg = utils::format("Database exception: '%s' in %s:%d", txt, file, line);		
	}
	virtual const char *what() const throw() { return msg.c_str(); }
private:
	std::string msg;
};

class not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Not found exception"; }
};

#define THROW(e, args...) throw e(args, __FILE__, __LINE__)

class IncrementalQuery {
public:

	IncrementalQuery(sqlite3 *db, SongDatabase *sdb) : db(db), sdb(sdb), searchLimit(10000) {}

	void addLetter(char c);
	void removeLast();
	void clear();
	const std::string getString();
	const std::vector<std::string> &getResult() const;
	int numHits() const;
	std::string getFull(int pos);
private:
	void search();

	sqlite3 *db;
	SongDatabase *sdb;
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> oldParts;
	std::vector<std::string> firstResult;
	std::vector<std::string> finalResult;
};

class SongDatabase {
public:

	SongDatabase(const std::string &name);
	void generateIndex();
	IncrementalQuery find() {
		return IncrementalQuery(db, this);
	}

	void search(const char *query);
	void search(const std::string &query, std::vector<std::string> &result, unsigned int searchLimit);
private:

	void addSubStrings(const char *words, std::unordered_map<std::string, std::vector<int>> &stringMap, int index);
	sqlite3 *db;

	std::unordered_map<std::string, std::vector<int>> titleMap;
	std::unordered_map<std::string, std::vector<int>> composerMap;
	std::vector<std::pair<std::string, int>> titles;
	std::vector<std::pair<std::string, int>> composers;

};

