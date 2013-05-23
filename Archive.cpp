
#include <vector>

#include "Archive.h"
#include "zip.h"

using namespace std;
using namespace utils;

class ZipFile : public Archive {
public:
	ZipFile(const string &fileName, const string &workDir = ".") : workDir(workDir) {
		zipFile = zip_open(fileName.c_str(), 0, NULL);
	}

	~ZipFile() {
		if(zipFile)
			close();
	}

	void close() {
		zip_close(zipFile);
		zipFile = nullptr;
	}

	File extract(const string &name) {
		int i = zip_name_locate(zipFile, name.c_str(), ZIP_FL_NOCASE);
		if(i >= 0) {
			struct zip_file *zf = zip_fopen_index(zipFile, i, 0);
			File file(workDir + "/" + name);
			printf("Extracting %s (%p)\n", file.getName().c_str(), zf);
			vector<uint8_t> buffer(2048);
			while(true) {
				int bytes = zip_fread(zf, &buffer[0], buffer.size());
				if(bytes > 0)
					file.write(&buffer[0], bytes);
				else
					break;
			}
			file.close();
			zip_fclose(zf);
			return file;
		}
		return File();
	}

	virtual string nameFromPosition(int pos) const {
		struct zip_stat sz;
		zip_stat_index(zipFile, pos, 0, &sz);
		return string(sz.name);
	}

	virtual int totalFiles() const {
		return zip_get_num_files(zipFile);
	}

private:
	struct zip *zipFile;
	string workDir;
};


Archive *Archive::open(const std::string &fileName, const std::string &targetDir) {
	return new ZipFile(fileName, targetDir);
}

bool Archive::canHandle(const std::string &name) {
	return name.rfind(".zip") != string::npos;
}

