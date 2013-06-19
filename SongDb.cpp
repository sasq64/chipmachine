#include "log.h"

#include "SongDb.h"

#include "sqlite3/sqlite3.h"
#include "utils.h"

#include <cstring>
#include <algorithm>

using namespace std;
using namespace utils;


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
	query.push_back(c);	
	if(query.size() > 2) {
		search();
	}
}

void IncrementalQuery::removeLast() {
	if(!query.empty()) {
		query.pop_back();
		if(query.size() > 2) {
			search();
		}
	}
}

void IncrementalQuery::clear() {
	query.resize(0);
}

const string IncrementalQuery::getString() { 
	return string(&query[0], query.size());
}

const vector<string> &IncrementalQuery::getResult(int start, int size) {

	if(lastStart != start || lastSize < size) {
		textResult.resize(0);

		for(int i = start; i<start+size && i < (int)finalResult.size(); i++) {
			int index = finalResult[i];		
			textResult.push_back(format("%s\t%s\t%d", sdb->getTitle(index), sdb->getComposer(index), index));
		}
		lastStart = start;
		lastSize = size;
	}
	return textResult;
}
int IncrementalQuery::numHits() const {
	return finalResult.size();
}

//#define BM

void IncrementalQuery::search() {

	lastStart = -1;

	string q = string(&query[0], query.size());

	auto parts = split(q, " ");
	// Remove empty strings
	parts.erase(remove_if(parts.begin(), parts.end(), [&](const string &a) { return a.size() == 0; }), parts.end());
	//LOGD("Parts: [%s]", parts);

	if(oldParts.size() == 0 || oldParts[0] != parts[0]) {
		sdb->search(parts[0], firstResult, searchLimit);
	}
	oldParts = parts;

	if(parts.size() == 1) {
		finalResult = firstResult;
		return;
	}

	finalResult.resize(0);

	for(auto &index : firstResult) {
		//string rc = r;		
		//makeLower(rc);
		bool found = true;
		//for(auto p : parts) {
		for(size_t i=1; i<parts.size(); i++) {
			const auto &p = parts[i];
			string title = sdb->getTitle(index);
			makeLower(title);
			string composer = sdb->getComposer(index);
			makeLower(composer);
			if(title.find(p) == string::npos && composer.find(p) == string::npos) {
				found = false;
				break;
			}

		}
		if(found)
			finalResult.push_back(index);//format("%s\t%s\t%d", sdb->getTitle(index), sdb->getComposer(index), index));
	}
}
/*
#ifdef BM
		int rcl = r.length();
		for(auto *b : bs) {
			if(!b->search(rc.c_str(), rcl)) {
				found = false;
				break;
			}
		} 
#else
#ifdef BM
	for(auto *b : bs) {
		delete b;
	}
#endif
*/


SongDatabase::SongDatabase(const string &name) {
	db = nullptr;
	int rc = sqlite3_open(name.c_str(), &db);//, SQLITE_READONLY, NULL);
	if(rc != SQLITE_OK) {	
		throw database_exception(format("%s: %s", name, sqlite3_errstr(rc)).c_str());
	};
}

SongDatabase::~SongDatabase() {
	if(db)
		sqlite3_close(db);
}


/*

Logic read all titles and composers into 2 arrays
At the same time create 2 3L-maps referring those arrays

*/

string IncrementalQuery::getFull(int pos) {

	//string r = finalResult[pos];
	//auto parts = split(r, "\t");
	//int id = atoi(parts[2].c_str());
	int id = finalResult[pos]+1;

	LOGD("ID %d", id);

	//id++;

	sqlite3_stmt *s;
	const char *tail;
	int rc = sqlite3_prepare_v2(db, "SELECT title, composer, path, metadata FROM songs WHERE _id == ?", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc != SQLITE_OK)
		throw database_exception("Select failed");
	sqlite3_bind_int(s, 1, id);
	int ok = sqlite3_step(s);
	
	if(ok == SQLITE_ROW) {
		const char *title = (const char *)sqlite3_column_text(s, 0);
		const char *composer = (const char *)sqlite3_column_text(s, 1);
		const char *path = (const char *)sqlite3_column_text(s, 2);
		const char *metadata = (const char *)sqlite3_column_text(s, 3);
		sqlite3_finalize(s);
		return format("%s\t%s\t%s\t%s", title, composer, path, metadata);
	} else {
		sqlite3_finalize(s);
		throw not_found_exception();
	}
	//return "";
}

static uint16_t tlcode(const char *s) {

	int x = s[0] > '9' ? s[0] - 'a' : s[0] - '0';
	int y = s[1] > '9' ? s[1] - 'a' : s[1] - '0';
	int z = s[2] > '9' ? s[2] - 'a' : s[2] - '0';
	return x + y*40 + z*40*40;
}

void SongDatabase::addSubStrings(const char *words, unordered_map<uint16_t, std::vector<int>> &stringMap, int index) {
	//auto parts = ansplit(title)
	const char *ptr = words;
	char tl[4];
	tl[3] = 0;
	unsigned int sl = strlen(words);
	if(sl >= 3) {
		for(unsigned int i=0; i<sl; i++) {
			char c = words[i];
			const char *pc = &words[i];
			if(!isalnum(c)) {
				ptr = &words[i+1];
				continue;
			}

			if(pc - ptr == 2) {
				tl[0] = tolower(ptr[0]);
				tl[1] = tolower(ptr[1]);
				tl[2] = tolower(ptr[2]);
				stringMap[tlcode(tl)].push_back(index);
				ptr++;
			}
		}
	}
}

void SongDatabase::generateIndex() {

	sqlite3_stmt *s;
	const char *tail;
	char oldComposer[256] = "";

	int rc = sqlite3_prepare_v2(db, "SELECT title, composer FROM songs;", -1, &s, &tail);
	LOGD("Result '%d'\n", rc);
	if(rc != SQLITE_OK)
		throw database_exception("Select failed");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;
	while(count < 100000) {
		count++;
		int ok = sqlite3_step(s);
		if(ok == SQLITE_ROW) {
			const char *title = (const char *)sqlite3_column_text(s, 0);
			const char *composer = (const char *)sqlite3_column_text(s, 1);

			int tindex = titles.size();

			if(strcmp(composer, oldComposer) != 0) {
				composers.push_back(make_pair(composer, tindex));
				cindex = composers.size()-1;
				strcpy(oldComposer, composer);

				addSubStrings(composer, composerMap, cindex);
			}

			titles.push_back(make_pair(title, cindex));


			addSubStrings(title, titleMap, tindex);


		} else if(ok == SQLITE_DONE) {
			break;
		}
	}

	sqlite3_finalize(s);

	LOGD("%d titles by %d composer generated %d + %d 3 letter indexes\n", titles.size(), composers.size(), titleMap.size(), composerMap.size());
}
/*
static const vector<const string>> t9array { "", "", "abc", "def", "ghi", "jkl", "mno", "prqs", "tuv", "wxyz" };

vector<string> makeT9(const string &numbers) {

	for(auto s : numbers) {
		int n = atoi(s.c_str());
		for(auto letters : t9array[n]) {

		}
	}
}

void SongDatabase::t9search(const string &query, vector<string> &result, unsigned int searchLimit) {



}
*/

void SongDatabase::search(const string &q, vector<int> &result, unsigned int searchLimit) {

	result.resize(0);
	if(q.size() < 3)
		return;
	bool q3 = (q.size() == 3);

	string query = q;
	makeLower(query);

	//LOGD("Checking '%s' among %d+%d sub strings", query, titleMap.size(), composerMap.size());

	uint16_t v = tlcode(query.c_str());

	const auto &tv = titleMap[v];
	const auto &cv = composerMap[v];

	LOGD("Searching %d candidates for '%s'", tv.size(), query);

	if(q3) {
		result = tv;
	} else {

		for(int index : tv) {
			string title = titles[index].first;
			makeLower(title);
			/*if(q3) {
				result.push_back(format("%s\t%s\t%d", titles[index].first, composers[titles[index].second].first, index));
				continue;
			} */
			if(title.find(query) != string::npos) {
				// NOTE: This line takes the bulk of all time	
				// Should push just the index and provice getters instead
				result.push_back(index); //format("%s\t%s\t%d", titles[index].first, composers[titles[index].second].first, index));
				if(result.size() >= searchLimit)
					break;
			}
		}
	}

	//LOGD("Searching %d candidates", cv.size());
	for(int index : cv) {
		int title_index = composers[index].second;
		string composer = composers[index].first;
		makeLower(composer);
		if(composer.find(query) != string::npos) {
			while(true) {
				auto title = titles[title_index];
				if(title.second != index)
					break;
				//LOGD("%s / %s", title.first, composers[index].first);
				//result.push_back(format("%s\t%s\t%d", title.first, composers[index].first, title_index));
				result.push_back(title_index);
				title_index++;
				if(result.size() >= searchLimit)
					break;
			}
		}
	}
}

void SongDatabase::search(const char *query) {
}


#ifdef UNIT_TEST

#include "catch.hpp"
#include <sys/time.h>

TEST_CASE("db::index", "Generate index") {

	SongDatabase db {"hvsc.db"};
	db.generateIndex();

	string search_string = "tune tel fre";
	vector<int> results { 0, 0, 2931, 2681, 2681, 2681, 524, 10, 10, 1, 1, 1 };
	IncrementalQuery q = db.find();
	int i = 0;
	for(char c : search_string) {		
		q.addLetter(c);
		//LOGD("%s %d", q.getString(), q.numHits());
		REQUIRE(q.numHits() == results[i]);
		i++;
	}
	string res = q.getResult(0, 10)[0];
	REQUIRE(res.find("Freaky Tune") != string::npos);
}

TEST_CASE("db::find", "Search Benchmark") {

	timeval tv;
	gettimeofday(&tv, NULL);
	long d0 = tv.tv_sec * 1000000 + tv.tv_usec;

	logging::setLevel(logging::INFO);

	SongDatabase db {"hvsc.db"};
	db.generateIndex();


	vector<IncrementalQuery> iqs;

	for(int i=0; i<250; i++) {
		iqs.push_back(db.find());
	}

	string search_string = "tune tel";
	int i = 0;

	gettimeofday(&tv, NULL);
	long d1 = tv.tv_sec * 1000000 + tv.tv_usec;

	for(char c : search_string) {		
		for(auto &q : iqs) {
			q.addLetter(c);			
			//REQUIRE(q.numHits() == results[i]);
		}
		i++;
	}

	gettimeofday(&tv, NULL);
	long d2 = tv.tv_sec * 1000000 + tv.tv_usec;

	LOGI("Search took %dms", (d2-d1)/1000);
}

#endif
