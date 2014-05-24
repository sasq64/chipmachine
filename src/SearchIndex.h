#ifndef SEARCH_INDEX_H
#define SEARCH_INDEX_H

#include <coreutils/file.h>

#include <string>
#include <vector>
#include <unordered_map>

class SearchProvider {
public:
	// Search for a string, return indexes of hits
	virtual int search(const std::string &word, std::vector<int> &result, unsigned int searchLimit) = 0;
	// Lookup internal string for index
	virtual std::string getString(int index) const = 0;
	// Get full data, may require SQL query
	virtual std::string getFullString(int index) const { return getString(index); }
};

class IncrementalQuery {
public:

	IncrementalQuery() : provider(nullptr) {}

	IncrementalQuery(SearchProvider *provider) : newRes(false), provider(provider), searchLimit(10000), lastStart(-1), lastSize(-1) {}

	void addLetter(char c);
	void removeLast();
	void clear();
	void setString(const std::string &s);
	const std::string getString();
	const std::vector<std::string> &getResult(int start, int size);
	int numHits() const;
	bool newResult() {
		bool r = newRes;
		newRes = false;
		return r;
	}

	std::string getFull(int index) const {
		return provider->getFullString(finalResult[index]);
	}


private:
	void search();

	bool newRes;

	SearchProvider *provider;
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> oldParts;
	std::vector<int> firstResult;
	std::vector<int> finalResult;
	std::vector<std::string> textResult;
	int lastStart;
	int lastSize;
};


class SearchIndex : public SearchProvider {
public:

	SearchIndex() {}
	~SearchIndex() {}

	void reserve(uint32_t sz) { strings.reserve(sz); }

	int search(const std::string &word, std::vector<int> &result, unsigned int searchLimit) override;
	std::string getString(int index) const override { return strings[index]; }

	int add(const std::string &str, bool stringonly = false);


	void dump(utils::File &f);
	void load(utils::File &f);

	static void simplify(std::string &s);
	static unsigned int tlcode(const char *s);


private:

	static void initTrans();

	static bool transInited;
	static std::vector<uint8_t> to7bit;
	static std::vector<uint8_t> to7bitlow;

	// Maps coded 3-letters to a list of indexes
	//std::unordered_map<uint16_t, std::vector<int>> stringMap;
	std::vector<int> stringMap[65536];
	// The actual strings
	std::vector<std::string> strings;

};

#endif // SEARCH_INDEX_H