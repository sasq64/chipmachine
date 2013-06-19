
#include <string>
#include <vector>
#include <unordered_map>

#include "SearchIndex.h"

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


class SongDatabase : public SearchProvider {
public:

	SongDatabase(const std::string &name);
	~SongDatabase();
	void generateIndex();
	IncrementalQuery find() {
		return IncrementalQuery(this);
	}

	int search(const std::string &query, std::vector<int> &result, unsigned int searchLimit);

	std::string getString(int index) {
		return utils::format("%s\t%s\t%d", getTitle(index), getComposer(index), index);
	}

	std::string getTitle(int index) { 
		return titleIndex.getString(index);
	}
	std::string getComposer(int index) {
		return composerIndex.getString(titleToComposer[index]);
	}

	void addSubStrings(const std::string &words, std::unordered_map<uint16_t, std::vector<int>> &stringMap, int index);

	virtual std::string getFullString(int index) override;

private:

	sqlite3 *db;

	SearchIndex composerIndex;
	SearchIndex titleIndex;
	std::vector<int> titleToComposer;
	std::vector<int> composerToTitle;
};

