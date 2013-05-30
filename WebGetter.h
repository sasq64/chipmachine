#ifndef WEBGETTER_H
#define WEBGETTER_H

#include "utils.h"

#include <string>

#include <mutex>
#include <thread>

#include <stdio.h>

class WebGetter {
public:
	class Job {
	public:
		Job(const std::string &url, const std::string &targetDir);
		bool isDone();
		std::string getFile();
	private:
		void urlGet(std::string url);
		static size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *userdata);
		static size_t headerFunc(void *ptr, size_t size, size_t nmemb, void *userdata);

		std::mutex m;
		bool loaded;
		std::thread jobThread;
		std::string targetDir;
		FILE *fp;
		std::string target;
	};

	WebGetter(const std::string &workDir) ;
	Job *getURL(const std::string &url);
private:
	std::string workDir;
};

#endif // WEBGETTER_H