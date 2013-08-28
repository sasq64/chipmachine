#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <coreutils/utils.h>

using namespace std;

class Archive {
public:

	//virtual extractAll() = 0;
	virtual utils::File extract(const std::string &name) = 0;
	virtual std::string nameFromPosition(int pos) const = 0;
	virtual int totalFiles() const = 0;

	class const_iterator  {
	public:
    	const_iterator(const Archive *a, int pos = 0) : archive(a), position(pos) {}
    	const_iterator(const const_iterator& rhs) : archive(rhs.archive), position(rhs.position) {}

		bool operator!= (const const_iterator& other) const {
        	return position != other.position;
    	}
 
	    std::string operator* () const {
	    	return archive->nameFromPosition(position);
	    }
 
    	const const_iterator& operator++ () {
    		position++;
    		return *this;
    	}
    private:
	   	const Archive *archive;
    	int position;
    };

    const_iterator begin() const {
    	return const_iterator(this);
    }

    const_iterator end() const {
    	return const_iterator(this, totalFiles());
    }

	static Archive *open(const std::string &fileName, const std::string &targetDir = ".");
	static bool canHandle(const std::string &name);

};

#endif // ARCHIVE_H