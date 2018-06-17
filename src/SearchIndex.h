#ifndef SEARCH_INDEX_H
#define SEARCH_INDEX_H

#include <coreutils/file.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <coreutils/thread.h>

#include <coreutils/log.h>
/*
template <typename T> class Worker {
public:
    Worker() {
        inProgress = 0;
        quit = false;
        // threadMutex.resize(threadCount);
        for(int tno = 0; tno < threadCount; tno++) {
            // workStart[tno] = workEnd[tno] = 0;
            LOGD("Creating thread %d", tno);
            threads.emplace_back([=]() mutable {
                while(!quit) {
                    {
                        std::unique_lock<std::mutex> l(m);
                        cv.wait(l);
                        inProgress++;
                    }
                    if(sourceVec) {
                        // LOGD("WORK STARTING");
                        for(int i = tno; i < sourceVec->size(); i += threadCount) {
                            const T &item = (*sourceVec)[i];
                            if(filterFunc(item)) {
                                // Dealing with index 'i'. Need to wait until all lower indexes has
                                // been dealt with.
                                // while(handled < i)
                                //	utils::sleepms(0);
                                auto p = counter.fetch_add(1);
                                (*targetVec)[p] = item;
                            }
                            // handled++;
                        }
                        // LOGD("WORK DONE");
                    }
                    inProgress--;
                    done_cv.notify_all();
                }
            });
        }
    }

    ~Worker() {
        quit = true;
        cv.notify_all();
        for(auto &t : threads)
            t.join();
    }

    std::vector<T> reduce(const std::vector<T> &vec, std::function<bool(const T &t)> filter) {
        std::vector<T> result;
        for(const auto &x : vec) {
            if(filter(x))
                result.push_back(x);
        }
        return result;
    }

    std::vector<T> reduce_p(const std::vector<T> &vec, std::function<bool(const T &t)> filter) {
        std::vector<T> result(vec.size());
        filterFunc = filter;
        sourceVec = &vec;
        targetVec = &result;
        counter = 0;
        handled = 0;
        cv.notify_all();
        while(true) {
            std::unique_lock<std::mutex> lock(done_mutex);
            done_cv.wait(lock);
            if(inProgress == 0)
                break;
        }
        sourceVec = nullptr;
        result.resize(counter);
        return result;
    }

private:
    int threadCount = 4;

    std::vector<std::thread> threads;
    std::atomic<size_t> counter;
    std::atomic<size_t> handled;
    std::atomic<int> inProgress;
    std::condition_variable cv;
    std::condition_variable seq_cv;
    std::mutex m;
    std::condition_variable done_cv;
    std::mutex done_mutex;
    std::atomic<bool> quit;
    const std::vector<T> *sourceVec;
    std::vector<T> *targetVec;
    std::function<bool(const T &)> filterFunc;
};
*/
class SearchProvider {
public:
	// Search for a string, return indexes of hits
	virtual int search(const std::string &word, std::vector<int> &result,
	                   unsigned int searchLimit) = 0;
	// Lookup internal string for index
	virtual std::string getString(int index) const = 0;
	// Get full data, may require SQL query
	virtual std::string getFullString(int index) const { return getString(index); }
};

class IncrementalQuery {
public:
	IncrementalQuery() : provider(nullptr) {}

	IncrementalQuery(SearchProvider *provider)
	    : newRes(false), provider(provider), searchLimit(20000), lastStart(-1), lastSize(-1) {}

	void addLetter(char c);
	void removeLast();
	void clear();
	void setString(const std::string &s);
	const std::string getString();
	const std::vector<std::string> &getResult(int start, int size);
	const std::string getResult(int start);

	int getIndex(int no) { return finalResult[no]; }

	int numHits() const;
	bool newResult() {
		bool r = newRes;
		newRes = false;
		return r;
	}

	void invalidate() {
		oldWords.clear();
	}
	
	// std::string getFull(int index) const {
	//	return provider->getFullString(finalResult[index]);
	//}

private:
	void search();

	bool newRes;

	SearchProvider *provider;
	unsigned int searchLimit;
	std::vector<char> query;
	std::vector<std::string> oldWords;
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

	int search(const std::string &word, std::vector<int> &result,
	           unsigned int searchLimit) override;
	std::string getString(int index) const override { return strings[index]; }

	int add(const std::string &str, bool stringonly = false);

	void dump(apone::File& f);
	void load(apone::File& f);

	static std::string& simplify(std::string &s);
	static unsigned int tlcode(const char *s);

	void setFilter(std::function<bool(int)> f = nullptr) { 
		filter = f;
	}

	uint32_t size() { return strings.size(); }	

private:
	// Worker<int> worker;

	std::function<bool(int)> filter;

	static void initTrans();

	static bool transInited;
	static std::vector<uint8_t> to7bit;
	static std::vector<uint8_t> to7bitlow;

	// Maps coded 3-letters to a list of indexes
	// std::unordered_map<uint16_t, std::vector<int>> stringMap;
	std::vector<int> stringMap[65536];
	// The actual strings
	std::vector<std::string> strings;
};

#endif // SEARCH_INDEX_H

