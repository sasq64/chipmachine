#include <string>
#include <vector>
#include <unordered_map>

class SearchProvider {
public:
	// Search for a string, return indexes of hits
	virtual int search(const std::string &word, std::vector<int> &result, unsigned int searchLimit) = 0;
	// Lookup internal string for index
	virtual std::string getString(int index) = 0;
	// Get full data, may require SQL query
	virtual std::string getFullString(int index) { return getString(index); }
};

class IncrementalQuery {
public:

	IncrementalQuery(SearchProvider *provider) : provider(provider), searchLimit(10000), lastStart(-1), lastSize(-1) {}

	void addLetter(char c);
	void removeLast();
	void clear();
	const std::string getString();
	const std::vector<std::string> &getResult(int start, int size);
	int numHits() const;

	std::string getFull(int index) {
		return provider->getFullString(finalResult[index]);
	}


private:
	void search();

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
	int search(const std::string &word, std::vector<int> &result, unsigned int searchLimit) override;
	std::string getString(int index) override { return strings[index]; }

	int add(const std::string &str);

	static void simplify(std::string &s);
	static unsigned int tlcode(const char *s);


private:

	static void initTrans();

	static bool transInited;
	static std::vector<uint8_t> to7bit;
	static std::vector<uint8_t> to7bitlow;



	// Maps coded 3-letters to a list of indexes
	std::unordered_map<uint16_t, std::vector<int>> stringMap;

	// The actual strings
	std::vector<std::string> strings;

};
