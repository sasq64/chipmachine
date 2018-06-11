#include "GZPlugin.h"

#include <coreutils/file.h>
#include <zlib.h>
#include <cstdio>

using namespace std;
using namespace utils;

namespace chipmachine {

int static inflate(const char *infile, const char *outfile) {
		int ret;
		uint8_t in[32768];
		uint8_t out[32768];

		z_stream strm;
		memset(&strm, 0, sizeof(strm));
		ret = inflateInit2(&strm, 16+MAX_WBITS);
		if(ret != Z_OK)
	        return ret;
		FILE *fp = fopen(infile, "rb");
		FILE *fpo = fopen(outfile, "wb");
		/* decompriiiess until deflate stream ends or end of file */
		do {
			strm.avail_in = fread(in, 1, sizeof(in), fp);
			if (ferror(fp)) {
				fclose(fp); fclose(fpo);
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
			if (strm.avail_in == 0)
				break;
			strm.next_in = in;
	
			/* run inflate() on input until output buffer not full */
			do {
				strm.avail_out = sizeof(out);
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
				}
				int have = sizeof(out) - strm.avail_out;
				if (fwrite(out, 1, have, fpo) != have || ferror(fpo)) {
					fclose(fpo);
					(void)inflateEnd(&strm);
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);
		} while (ret != Z_STREAM_END);
		fclose(fp);
		fclose(fpo);
		(void)inflateEnd(&strm);
		return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}	

musix::ChipPlayer *GZPlugin::fromFile(const string &fileName) {
	/* done when inflate() says it's done */
	auto outFile = fileName.substr(0, fileName.length() - 3);
	int rc = inflate(fileName.c_str(), outFile.c_str());
	LOGD("Trying to gunzip %s to %s = %d", fileName, outFile, rc);

	for(auto plugin : plugins) {
		if(plugin->canHandle(outFile)) {
			return plugin->fromFile(outFile);
		}
	}
	LOGD("No plugin could handle it");
	return nullptr;
};

bool GZPlugin::canHandle(const string &name) {
	return utils::path_extension(name) == "gz";
}

}
