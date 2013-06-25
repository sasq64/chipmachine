#include <uade/uade.h>
#include <uade/ossupport.h>
#include <uade/rmc.h>

#include <bencodetools/bencode.h>
#include <assert.h>
#include <string.h>

#define RMC_PREFIX "l9:rmc\x00\xfb\x13\xf6\x1f\xa2"
#define RMC_PREFIX_LEN 12

int uade_is_rmc(const char *buf, size_t size)
{
	if (size < RMC_PREFIX_LEN)
		return 0;
	return memcmp(buf, RMC_PREFIX, RMC_PREFIX_LEN) == 0;
}

int uade_is_rmc_file(const char *fname)
{
	char buf[RMC_PREFIX_LEN];
	size_t bufsize;
	FILE *f = fopen(fname, "rb");
	if (f == NULL)
		return 0;
	bufsize = uade_atomic_fread(buf, 1, sizeof buf, f);
	fclose(f);
	return uade_is_rmc(buf, bufsize);
}

static struct bencode *get_files(const struct bencode *rmc)
{
	return ben_list_get(rmc, 2);
}

static int check_subsongs(const struct bencode *meta)
{
	size_t pos;
	struct bencode *subsong;
	struct bencode *playtime;
	const struct bencode *subsongs = ben_dict_get_by_str(meta, "subsongs");
	long long totalplaytime = 0;

	if (subsongs == NULL) {
		uade_warning("Subsongs not found\n");
		return -1;
	}

	ben_dict_for_each(subsong, playtime, pos, subsongs) {
		if (!ben_is_int(subsong) || ben_int_val(subsong) < 0 ||
		    !ben_is_int(playtime) || ben_int_val(playtime) <= 0) {
			uade_warning("Invalid subsong data in RMC meta\n");
			return -1;
		}
		totalplaytime += ben_int_val(playtime);
	}

	/* totalplaytime must fit in 32 bit integer to be valid */
	if (totalplaytime >= (1UL << 31)) {
		uade_warning("Too long a song\n");
		return -1;
	}

	return totalplaytime;
}

int uade_rmc_get_module(struct uade_file **module, const struct bencode *rmc)
{
	struct bencode *meta = uade_rmc_get_meta(rmc);
	struct bencode *files = get_files(rmc);
	struct bencode *modulename = NULL;
	struct bencode *moduledata = NULL;
	size_t pos;

	if (module != NULL)
		*module = NULL;

	if (!ben_is_dict(meta) || !ben_is_dict(files))
		return -1;

	modulename = ben_dict_get_by_str(meta, "song");
	if (modulename == NULL) {
		if (ben_dict_len(files) > 1) {
			fprintf(stderr, "Ambiguous song file. Can not select which file to play. Alternatives are:\n");
			ben_dict_for_each(modulename, moduledata, pos, files)
				fprintf(stderr, "File: %s\n", ben_str_val(modulename));
			return -1;
		}
		/* Read the only file name from the container */
		ben_dict_for_each(modulename, moduledata, pos, files)
			break;
	}

	if (check_subsongs(meta) < 0)
		return -1;

	assert(modulename != NULL);

	moduledata = ben_dict_get(files, modulename);
	if (moduledata == NULL) {
		fprintf(stderr, "Module %s not in the container\n", ben_str_val(modulename));
		return -1;
	}

	if (!ben_is_str(modulename) || !ben_is_str(moduledata)) {
		uade_warning("Non-string entries in files dictrionary\n");
		return -1;
	}

	if (module != NULL) {
		*module = uade_file(ben_str_val(modulename),
				    ben_str_val(moduledata),
				    ben_str_len(moduledata));
		if (*module == NULL)
			return -1;
	}

	return 0;
}

struct bencode *uade_rmc_decode(const void *data, size_t size)
{
	struct bencode *magic;
	struct bencode *rmc;

	rmc = ben_decode(data, size);
	if (rmc == NULL)
		return NULL;

	if (!ben_is_list(rmc) || ben_list_len(rmc) < 3)
		goto error;

	magic = ben_list_get(rmc, 0);
	if (!ben_is_str(magic))
		goto error;
	if (memcmp(ben_str_val(magic), RMC_MAGIC, RMC_MAGIC_LEN) != 0)
		goto error;

        if (uade_rmc_get_module(NULL, rmc))
		goto error;

	return rmc;

error:
	ben_free(rmc);
	return NULL;
}

struct bencode *uade_rmc_decode_file(const char *fname)
{
	size_t size;
	void *data;
	struct bencode *rmc;
	data = uade_read_file(&size, fname);
	if (data == NULL)
		return NULL;
	rmc = uade_rmc_decode(data, size);
	free(data);
	return rmc;
}

/* Case insensitive name search in a directory */
static struct bencode *scan_dict(struct bencode *files, const char *name)
{
	size_t pos;
	struct bencode *key;
	struct bencode *value;

	/* Try fast exact match */
	value = ben_dict_get_by_str(files, name);
	if (value != NULL)
		return value;

	ben_dict_for_each(key, value, pos, files) {
		if (ben_is_str(key) && strcasecmp(name, ben_str_val(key)) == 0)
			return value;
	}

	return NULL;
}

struct uade_file *uade_rmc_get_file(const struct bencode *rmc, const char *name)
{
	char path[PATH_MAX];
	char *namepart;
	char *separator;
	struct bencode *files = get_files(rmc);
	struct bencode *f;

	if (name[0] == '.' || name[0] == '/' || strstr(name, "/.") != NULL) {
		uade_warning("rmc: Reject amiga name: %s\n", name);
		return NULL;
	}

	strlcpy(path, name, sizeof path);
	namepart = path;
	while (1) {
		separator = strchr(namepart, '/');
		if (separator == NULL)
			break;
		*separator = 0;
		/* Scan for a directory */
		files = scan_dict(files, namepart);
		if (files == NULL || !ben_is_dict(files))
			return NULL;
		namepart = separator + 1;
	}

	f = scan_dict(files, namepart);
	if (f == NULL)
		return NULL;

	return uade_file(name, ben_str_val(f), ben_str_len(f));
}

struct bencode *uade_rmc_get_meta(const struct bencode *rmc)
{
	return ben_list_get(rmc, 1);
}

double uade_rmc_get_song_length(const struct bencode *rmc)
{
	uint64_t totalplaytime = 0;
	size_t pos;
	struct bencode *subsong;
	struct bencode *playtime;
	const struct bencode *subsongs = uade_rmc_get_subsongs(rmc);

	assert(subsongs != NULL);

	ben_dict_for_each(subsong, playtime, pos, subsongs)
		totalplaytime += (uint64_t) ben_int_val(playtime);

	return totalplaytime / 1000.0;
}

const struct bencode *uade_rmc_get_subsongs(const struct bencode *rmc)
{
	return ben_dict_get_by_str(uade_rmc_get_meta(rmc), "subsongs");
}

int uade_rmc_record_file(struct bencode *rmc, const char *name,
			 const void *data, size_t len)
{
	char path[PATH_MAX];
	char *separator;
	char *namepart;
	struct bencode *dir;
	char *thispart;
	struct bencode *files = get_files(rmc);
	struct bencode *blob;

	if (name[0] == '.' || name[0] == '/') {
		uade_warning("Collected file name may not begin with "
			     "'.' or '/': %s\n", name);
		return -1;
	}
	if (strstr(name, "/.") != NULL || strstr(name, "./") != NULL) {
		uade_warning("Collected file name may not contain "
			     "\"./\" or \"/.\": %s\n", name);
		return -1;
	}

	strlcpy(path, name, sizeof path);
	namepart = path;

	while (1) {
		separator = strchr(namepart, '/');
		if (separator == NULL)
			break;
		*separator = 0;

		thispart = namepart;
		namepart = separator + 1;

		dir = scan_dict(files, thispart);
		if (dir != NULL && !ben_is_dict(dir)) {
			uade_warning("rmc: %s is not a directory as would be "
				     "expected. Refusing to take this file.\n",
				     thispart);
			return -1;
		}
		if (dir != NULL) {
			files = dir;
			continue;
		}
		/*
		 * This is somewhat awkward, but we just iterate this loop to
		 * create all necessary subdirectories.
		 */
		dir = ben_dict();
		if (dir == NULL || ben_dict_set_by_str(files, thispart, dir)) {
			uade_warning("No memory for directory entry: %s\n",
				     thispart);
			ben_free(dir);
			return -1;
		}
		files = dir;
	}

	assert(strlen(namepart) > 0);

	blob = scan_dict(files, namepart);
	if (blob != NULL) {
		fprintf(stderr, "File has already been recorded: %s\n", name);
		return -1;
	}
	blob = ben_blob(data, len);
	if (blob == NULL || ben_dict_set_by_str(files, namepart, blob)) {
		fprintf(stderr, "No memory to collect a file: %s\n", name);
		ben_free(blob);
		return -1;
	}
	return 0;
}
