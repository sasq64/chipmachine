#include <coreutils/log.h>
#include <coreutils/utils.h>

#include "SearchIndex.h"

#include <cstring>
#include <algorithm>
#include <set>

using namespace std;
using namespace utils;

#define USE_THREADS

// BOYER MOORE STUFF

#define ALPHABET_LEN 256
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)


class BMSearch {

public:
	BMSearch(const std::string &pattern) {
		patlen = pattern.length();
		pat = (const unsigned char*)malloc(patlen+1);
		strcpy((char*)pat, pattern.c_str());
    	delta2 = (int*)malloc(patlen * sizeof(int));
    	make_delta1(delta1, pat, patlen);
    	make_delta2(delta2, pat, patlen);
	}

	const char *search(const char *st, int stringlen) {

	    int i;

	   	const unsigned char *string = (const unsigned char*)st;

	    i = patlen-1;
	    while (i < stringlen) {
	        int j = patlen-1;
	        while (j >= 0 && (tolower(string[i]) == pat[j])) {
	            --i;
	            --j;
	        }
	        if (j < 0) {
	            return (st + i+1);
	        }

	        i += max(delta1[tolower(string[i])], delta2[j]);
	    }
	    return nullptr;
	}

private:

	void make_delta1(int *delta1, const unsigned char *pat, int32_t patlen) {
	    int i;
	    for (i=0; i < ALPHABET_LEN; i++) {
	        delta1[i] = NOT_FOUND;
	    }
	    for (i=0; i < patlen-1; i++) {
	        delta1[pat[i]] = patlen-1 - i;
	    }
	}

	// true if the suffix of word starting from word[pos] is a prefix
	// of word
	int is_prefix(const unsigned char *word, int wordlen, int pos) {
	    int i;
	    int suffixlen = wordlen - pos;
	    // could also use the strncmp() library function here
	    for (i = 0; i < suffixlen; i++) {
	        if (word[i] != word[pos+i]) {
	            return 0;
	        }
	    }
	    return 1;
	}

	// length of the longest suffix of word ending on word[pos].
	// suffix_length("dddbcabc", 8, 4) = 2
	int suffix_length(const unsigned char *word, int wordlen, int pos) {
	    int i;
	    // increment suffix length i to the first mismatch or beginning
	    // of the word
	    for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
	    return i;
	}

	void make_delta2(int *delta2, const unsigned char *pat, int32_t patlen) {
	    int p;
	    int last_prefix_index = patlen-1;

	    // first loop
	    for (p=patlen-1; p>=0; p--) {
	        if (is_prefix(pat, patlen, p+1)) {
	            last_prefix_index = p+1;
	        }
	        delta2[p] = last_prefix_index + (patlen-1 - p);
	    }

	    // second loop
	    for (p=0; p < patlen-1; p++) {
	        int slen = suffix_length(pat, patlen, p);
	        if (pat[p - slen] != pat[patlen-1 - slen]) {
	            delta2[patlen-1 - slen] = patlen-1 - p + slen;
	        }
	    }
	}
	const unsigned char *pat;
	int patlen;
    int delta1[ALPHABET_LEN];
    int *delta2;
};

///////////////////

void IncrementalQuery::addLetter(char c) {
	if(c == ' ') {
		if(query.size() == 0 || query.back() == ' ')
			return;
	}
	LOGV("Adding %c", c);
	query.push_back(c);
	if(query.size() > 0) {
		search();
	}
}

void IncrementalQuery::removeLast() {
	if(!query.empty()) {
		query.pop_back();
		if(query.size() > 0) {
			search();
		} else {
			lastStart = -1;
			newRes = true;
			finalResult.clear();
		}
	}
}

void IncrementalQuery::setString(const std::string &s) {
	query.resize(0);
	for(const auto &c : s) {
		query.push_back(c);
	}
	search();
}

void IncrementalQuery::clear() {
	query.resize(0);
}

const string IncrementalQuery::getString() {
	return string(&query[0], query.size());
}

const string IncrementalQuery::getResult(int i) {
	int index = finalResult[i];
	return provider->getFullString(index);
}

const vector<string> &IncrementalQuery::getResult(int start, int size) {

	if(lastStart != start || lastSize < size) {
		textResult.resize(0);

		for(int i = start; i<start+size && i < (int)finalResult.size(); i++) {
			int index = finalResult[i];
			textResult.push_back(provider->getFullString(index));
		}
		lastStart = start;
		lastSize = size;
	}
	return textResult;
}
int IncrementalQuery::numHits() const {
	return finalResult.size();
}

void IncrementalQuery::search() {

	lastStart = -1;
	newRes = true;

	string q = string(&query[0], query.size());

	auto words = split(q);

	// Words : IRON LORD -> 3L= "IRO"

	// Remove empty strings
	words.erase(remove_if(words.begin(), words.end(), [&](const string &a) { return a.size() == 0; }), words.end());
	LOGD("words: [%s]", words);


	if(oldWords.size() == 0 || oldWords[0] != words[0]) {
		LOGD("## First word changed");
		// First word has changed
		//  - If something was just added, we can filter our firstResult more
		//  - Otherwise do a new search
		if(words[0].size() > 4 && words[0].find(oldWords[0]) == 0) {
			// Erase things that don't match from existing result
			LOGD("## FAST: Filter prevous result");
			firstResult.erase(
				std::remove_if(firstResult.begin(), firstResult.end(), [=](const int &index) -> bool {
					string str = provider->getString(index);
					SearchIndex::simplify(str);
					return str.find(words[0]) == string::npos;
				}),
			firstResult.end());
		} else {
			// Do full search
			// In chipmachine this is a proxy that searches in two separate providers
			// This will do the 3L 16bit lookup and grep for the word in all hits. May take time
			LOGD("## SLOW: Full search");
			provider->search(words[0], firstResult, searchLimit);
		}
	}
	oldWords = words;

	if(words.size() == 1) {
		finalResult = firstResult;
		return;
	}


	// Check if the other words (or words) is contained in the result

	finalResult.resize(0);

	// TODO: Parallellize this!

	LOGD("## OTHER PARTS");
	for(auto &index : firstResult) {
		//string rc = r;
		//makeLower(rc);
		bool found = true;
		//for(auto p : words) {

		// Get the full searchable string for this result
		string str = provider->getString(index);
		// Simplify it
		SearchIndex::simplify(str);

		// Check against the other words from the searchline 
		for(size_t i=1; i<words.size(); i++) {


			size_t pos = str.find(words[i-1]);
			if(pos != string::npos) {
				// Remove the previous match from the result string to avoid double matching
				str.erase(pos, words[i-1].length());
			}

			const auto &p = words[i];
			if(str.find(p) == string::npos) {
				// All words must match
				found = false;
				break;
			}
		}
		if(found)
			finalResult.push_back(index);//format("%s\t%s\t%d", sdb->getTitle(index), sdb->getComposer(index), index));
	}
}

bool SearchIndex::transInited = false;
std::vector<uint8_t> SearchIndex::to7bit(256);
std::vector<uint8_t> SearchIndex::to7bitlow(256);

static const char *translit = "!c$oY|S\"ca<n-R 0/23'uP.,1o>   ?AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo:ouuuuyty";

void SearchIndex::initTrans() {
	transInited = true;

	for(int i=0; i<256; i++) {
		if(i>=0xa1)
			to7bit[i] = translit[i-0xa1];
		else if(i>=0x80)
			to7bit[i] = '?';
		else
			to7bit[i] = i;
		to7bitlow[i] = tolower(to7bit[i]);
		if(to7bitlow[i] == '-' || to7bitlow[i] == '\'')
			to7bitlow[i] = 0;
	}
}

void SearchIndex::simplify(string &s) {

	if(!transInited) {
		initTrans();
	}
	unsigned char *p = (unsigned char*)&s[0];
	unsigned char *conv = &to7bitlow[0];
	while(*p) {
		if(!(*p = conv[*p])) {
			int i = p - (unsigned char*)&s[0];
			s.erase(i, 1);
			p = (unsigned char*)&s[0];
		}
		p++;
	}
}


unsigned int SearchIndex::tlcode(const char *s) {
	int l = 0;
	while(s[0]) {
		l *= 40;
		l += ((s[0] > '9' ? s[0] - 'a' + 10 : s[0] - '0') + 1);
		s++;
	}
	return l;
}

int SearchIndex::search(const string &q, vector<int> &result, unsigned int searchLimit) {

	result.resize(0);
	//if(q.size() < 3)
	//	return 0;

	bool q3 = (q.size() <= 3);

	string query = q;
	simplify(query);

	//LOGV("Checking '%s' among %d+%d sub strings", query, titleMap.size(), composerMap.size());

	uint16_t v = tlcode(query.substr(0,3).c_str());

	const auto &tv = stringMap[v];

	LOGV("Searching %d candidates for '%s'", tv.size(), query);
	if(filter) {
		LOGV("Filtering");
		if(q3) {
			for(int index : tv) {
				if(!filter(index))
					result.push_back(index);
			}
		} else {
			for(int index : tv) {
				if(filter(index))
					continue;
				string s = strings[index];
				simplify(s);
				if(s.find(query) != string::npos) {
					result.push_back(index);
				}
			}
		}
	} else {
		if(q3) {
			result = tv;
		} else {
			LOGD("## SLOW: First word filtering");

#ifdef USE_THREADS
			result = worker.reduce(tv, [=](int i) {
				string s = strings[i];
				simplify(s);
				return (s.find(query) != string::npos);
			});
#else
			auto sz = tv.size();
			for(int i = 0; i<sz; i++) {
				auto index = tv[i];
				string s = strings[index];
				simplify(s);
				if(s.find(query) != string::npos) {
					result.push_back(index);
				}
			}
#endif
		}
	}
	return result.size();
}

void SearchIndex::dump(utils::File &f) {

	for(int i=0; i<65536; i++) {
		auto sz = stringMap[i].size();
		f.write<uint32_t>(sz);
		if(sz > 0)
			f.write((uint8_t*)&stringMap[i][0], sz*sizeof(uint32_t));
	}
	f.write<uint32_t>(strings.size());
	for(int i=0; i<(int)strings.size(); i++) {
		f.write<uint8_t>(strings[i].length());
		f.write(strings[i]);
	}
}

void SearchIndex::load(utils::File &f) {

	if(!transInited) {
		initTrans();
	}

	for(int i=0; i<65536; i++) {
		auto sz = f.read<uint32_t>();
		stringMap[i].resize(sz);
		if(sz > 0)
			f.read((uint8_t*)&stringMap[i][0], sz*sizeof(uint32_t));
	}
	uint8_t temp[256];
	auto sz = f.read<uint32_t>();
	strings.resize(sz);
	for(int i=0; i<(int)strings.size(); i++) {
		auto l = f.read<uint8_t>();
		f.read(temp, l);
		temp[l] = 0;
		strings[i] = (char*)temp;
	}
}

int SearchIndex::add(const string &str, bool stringonly) {

	strings.push_back(str);
	int index = strings.size()-1;

	if(stringonly)
		return index;

	set<uint16_t> used;
	string tl;
	bool wordAdded = true;

	if(!transInited) {
		initTrans();
	}

	for(char c : str) {
		if(c == '-' || c == '\'')
			continue;

		c = to7bitlow[c&0xff];

		if(!isalnum(c)) {
			if(!wordAdded) {
				uint16_t code = tlcode(tl.c_str());
				//LOGV("Adding '%s'", tl);
				if(used.count(code) == 0) {
					stringMap[code].push_back(index);
					used.insert(code);
				}
				wordAdded = true;
			}
			tl.resize(0);
			continue;
		}
		wordAdded = false;
		tl.push_back(c);

		if(tl.size() == 3) {
			uint16_t code = tlcode(tl.c_str());
			//LOGV("Adding '%s'", tl);
			if(used.count(code) == 0) {
				stringMap[code].push_back(index);
				used.insert(code);
			}
			wordAdded = true;
			tl.erase(0,1);
		}
	}

	return index;
}
