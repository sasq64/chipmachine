#include <uade/uade.h>
#include <uade/uadestate.h>
#include <uade/ossupport.h>
#include <uade/unixatomic.h>

#include "md5.h"
#include "support.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

#define NORM_ID "n="
#define NORM_ID_LENGTH 2

static int escompare(const void *a, const void *b);
static struct uade_content *get_content(const char *md5,
					struct uade_state *state);


/* Compare function for bsearch() and qsort() to sort songs with respect
   to their md5sums */
static int contentcompare(const void *a, const void *b)
{
	return strcasecmp(((struct uade_content *)a)->md5,
			  ((struct uade_content *)b)->md5);
}

static int escompare(const void *a, const void *b)
{
	return strcasecmp(((struct eaglesong *)a)->md5,
			  ((struct eaglesong *)b)->md5);
}

static struct uade_content *get_content(const char *md5, struct uade_state *state)
{
	struct uade_content key;
	struct uade_songdb *db = &state->songdb;

	if (db->contentchecksums == NULL)
		return NULL;

	memset(&key, 0, sizeof key);
	strlcpy(key.md5, md5, sizeof key.md5);

	return bsearch(&key, db->contentchecksums, db->nccused,
		       sizeof db->contentchecksums[0], contentcompare);
}

static struct uade_content *create_content_checksum(struct uade_state *state,
						    const char *md5,
						    uint32_t playtime)
{
	struct uade_content *n;
	struct uade_songdb *db = &state->songdb;

	if (db->nccused == db->nccalloc) {
		db->nccalloc = MAX(db->nccalloc * 2, 16);
		n = realloc(db->contentchecksums, db->nccalloc * sizeof(n[0]));
		if (n == NULL) {
			fprintf(stderr, "uade: No memory for new content "
				"checksums.\n");
			return NULL;
		}
		db->contentchecksums = n;
	}

	n = &db->contentchecksums[db->nccused];

	if (md5 == NULL)
		return n;

	db->nccused++;

	db->ccmodified = 1;

	memset(n, 0, sizeof(*n));
	strlcpy(n->md5, md5, sizeof(n->md5));
	n->playtime = playtime;

	return n;
}

static void md5_from_buffer(char *dest, size_t destlen, const uint8_t *buf, size_t bufsize)
{
	uint8_t md5[16];
	int ret;
	uade_MD5_CTX ctx;
	uade_MD5Init(&ctx);
	uade_MD5Update(&ctx, buf, bufsize);
	uade_MD5Final(md5, &ctx);
	assert(destlen > 32);
	ret = snprintf(dest, destlen, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		       md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6],
		       md5[7], md5[8], md5[9], md5[10], md5[11], md5[12],
		       md5[13], md5[14], md5[15]);
	assert(ret == 32);
}

static void update_playtime(struct uade_state *state, struct uade_content *n,
			    uint32_t playtime)
{
	if (n->playtime != playtime) {
		state->songdb.ccmodified = 1;
		n->playtime = playtime;
	}
}

static void sort_content_checksums(struct uade_state *state)
{
	struct uade_songdb *db = &state->songdb;

	if (db->contentchecksums == NULL)
		return;

	qsort(db->contentchecksums, db->nccused,
	      sizeof db->contentchecksums[0], contentcompare);
}

/* replace must be zero if content db is unsorted */
struct uade_content *uade_add_playtime(struct uade_state *state,
				       const char *md5, uint32_t playtime)
{
	struct uade_content *n;

	/* If content db hasn't been read into memory already, it is not used */
	if (state->songdb.contentchecksums == NULL)
		return NULL;

	/* Do not record song shorter than 3 secs */
	if (playtime < 3000)
		return NULL;

	if (strlen(md5) != 32)
		return NULL;

	n = get_content(md5, state);
	if (n != NULL) {
		update_playtime(state, n, playtime);
		return n;
	}

	n = create_content_checksum(state, md5, playtime);

	sort_content_checksums(state);

	return n;
}

static void get_song_flags_and_attributes_from_songstore(struct uade_state *state)
{
	struct eaglesong key;
	struct eaglesong *es;
	struct uade_songdb *db = &state->songdb;

	if (db->songstore == NULL)
		return;
	/* Lookup md5 from the songdb */
	strlcpy(key.md5, state->song.info.modulemd5, sizeof key.md5);
	es = bsearch(&key, db->songstore, db->nsongs, sizeof db->songstore[0],
		     escompare);
	if (es == NULL)
		return;
	/* Found -> copy flags and attributes from database */
	state->song.flags |= es->flags;
	state->song.songattributes = es->attributes;
}

void uade_lookup_song(const struct uade_file *module, struct uade_state *state)
{
	struct uade_content *content;
	struct uade_song_state *song = &state->song;
	const struct bencode *rmc = uade_get_rmc_from_state(state);

	/* Compute an md5sum of the song */
	md5_from_buffer(song->info.modulemd5, sizeof song->info.modulemd5,
			(const uint8_t *) module->data, module->size);

	/* Needs state->song.info.modulemd5 */
	get_song_flags_and_attributes_from_songstore(state);

	if (rmc != NULL) {
		song->info.duration = uade_rmc_get_song_length(rmc);
	} else {
		/* Lookup playtime from content database */
		content = get_content(song->info.modulemd5, state);
		if (content != NULL && content->playtime > 0)
			song->info.duration = content->playtime / 1000.0;
	}
}

static int uade_open_and_lock(const char *filename, int create)
{
	int fd, ret;
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		if (errno == ENOENT && create) {
			fd = open(filename, O_RDWR | O_CREAT,
				  S_IRUSR | S_IWUSR);
			if (fd < 0)
				return -1;
		} else {
			return -1;
		}
	}
#ifndef UADE_HAVE_CYGWIN
	ret = lockf(fd, F_LOCK, 0);
	if (ret) {
		fprintf(stderr, "uade: Unable to lock song.conf: %s (%s)\n",
			filename, strerror(errno));
		uade_atomic_close(fd);
		return -1;
	}
#endif

	return fd;
}


static struct uade_content *store_playtime(const char *md5, long playtime,
					   int *newccmodified,
					   size_t oldnccused,
					   struct uade_state *state)
{
	struct uade_content *n = NULL;
	struct uade_songdb *db = &state->songdb;

	if (oldnccused > 0) {
		struct uade_content key;
		memset(&key, 0, sizeof key);
		strlcpy(key.md5, md5, sizeof key.md5);

		/* We use "oldnccused" here as the length, while new entries
		   are added in unsorted manner to the end of the array */
		n = bsearch(&key, db->contentchecksums, oldnccused,
			    sizeof db->contentchecksums[0], contentcompare);
		if (n == NULL)
			/* new songs on disk db -> merge -> need saving */
			*newccmodified = 1;
	}

	/* We value a playtime determined during run-time over
	   a database value */
	if (n == NULL) {
		/* Note, create_content_checksum() makes "ccmodified"
		   true, which we work-around later with the "newccmodified" */
		n = create_content_checksum(state, md5, (uint32_t) playtime);
	}

	if (n == NULL) {
		/* No memory, fuck. We shouldn't save anything to
		   avoid losing data. */
		fprintf(stderr, "uade: Warning, no memory for the song database\n");
		db->cccorrupted = 1;
	}

	return n;
}

void uade_free_song_db(struct uade_state *state)
{
	free(state->songdb.contentchecksums);
	free(state->songdb.songstore);
	memset(&state->songdb, 0, sizeof state->songdb);
}

int uade_read_content_db(const char *filename, struct uade_state *state)
{
	char line[1024];
	FILE *f;
	size_t lineno = 0;
	long playtime;
	int i, j, nexti;
	char *id, *eptr;
	char numberstr[1024];
	char *md5;
	struct uade_songdb *db = &state->songdb;
	struct stat st;

	/* We make backups of some variables because following loop will
	   make it always true, which is not what we want. The end result should
	   be that ccmodified is true in following cases only:
	   1. the in-memory db is already dirty
	   2. the in-memory db gets new data from disk db (merge operation)
	   Otherwise ccmodified should be false. */
	int newccmodified = db->ccmodified;
	size_t oldnccused = db->nccused;
	int fd;
	struct uade_content *n;

	/* Try to create a database if it doesn't exist */
	if (db->contentchecksums == NULL &&
	    create_content_checksum(state, NULL, 0) == NULL)
		return 0;

	fd = uade_open_and_lock(filename, 0);
	if (fd < 0) {
		fprintf(stderr, "uade: Can not find %s\n", filename);
		return 0;
	}

	f = fdopen(fd, "r");
	if (f == NULL) {
		fprintf(stderr, "uade: Can not create FILE structure for %s\n",
			filename);
		close(fd);
		return 0;
	}

	while (uade_xfgets(line, sizeof line, f) != NULL) {
		lineno++;

		if (line[0] == '#')
			continue;

		md5 = line;
		i = uade_skip_and_terminate_word(line, 0);
		if (i < 0)
			continue; /* playtime doesn't exist */

		for (j = 0; isxdigit(line[j]); j++);

		if (j != 32)
			continue; /* is not a valid md5sum */

		/* Grab and validate playtime (in milliseconds) */
		nexti = uade_skip_and_terminate_word(line, i);

		playtime = strtol(&line[i], &eptr, 10);
		if (*eptr != 0 || playtime < 0) {
			fprintf(stderr, "Invalid playtime for md5 %s on contentdb line %zd: %s\n", md5, lineno, numberstr);
			continue;
		}

		n = store_playtime(md5, playtime, &newccmodified, oldnccused, state);
		if (n == NULL)
			continue;

		i = nexti; /* Note, it could be that i < 0 */

		/* Get rest of the directives in a loop */
		while (i >= 0) {
			id = &line[i];
			i = uade_skip_and_terminate_word(line, i);
			fprintf(stderr,	"Unknown contentdb directive on line %zd: %s\n", lineno, id);
		}
	}

	if (!fstat(fd, &st))
		state->songdb.ccloadtime = st.st_mtime;

	fclose(f);

	db->ccmodified = newccmodified;

	sort_content_checksums(state);

	return 1;
}

int uade_read_song_conf(const char *filename, struct uade_state *state)
{
	FILE *f = NULL;
	struct eaglesong *s;
	size_t allocated;
	size_t lineno = 0;
	size_t i;
	int fd;
	struct uade_songdb *db = &state->songdb;
	int parse_error = 0;

	state->songdbname[0] = 0;

	fd = uade_open_and_lock(filename, 1);
	/* open_and_lock() may fail without harm (it's actually supposed to
	   fail if the process does not have lock (write) permissions to
	   the song.conf file */

	f = fopen(filename, "r");
	if (f == NULL)
		goto error;

	db->nsongs = 0;
	allocated = 16;
	db->songstore = calloc(allocated, sizeof db->songstore[0]);
	if (db->songstore == NULL) {
		uade_warning("No memory for song store.");
		goto error;
	}

	while (1) {
		char **items;
		size_t nitems;

		items = uade_read_and_split_lines(&nitems, &lineno, f,
						  UADE_WS_DELIMITERS);
		if (items == NULL)
			break;

		assert(nitems > 0);

		if (db->nsongs == allocated) {
			void *songstore;
			allocated *= 2;
			songstore = realloc(db->songstore,
					    allocated * sizeof(db->songstore[0]));
			if (songstore == NULL) {
				uade_warning("No memory for players.");
				goto parse_error;
			}
			db->songstore = songstore;
		}

		s = &db->songstore[db->nsongs];
		db->nsongs++;

		memset(s, 0, sizeof s[0]);

		if (strncasecmp(items[0], "md5=", 4) != 0) {
			fprintf(stderr, "Line %zd must begin with md5= in %s\n",
				lineno, filename);
			free(items);
			continue;
		}
		if (strlcpy(s->md5, items[0] + 4, sizeof s->md5) !=
		    ((sizeof s->md5) - 1)) {
			fprintf(stderr,
				"Line %zd in %s has too long an md5sum.\n",
				lineno, filename);
			free(items);
			continue;
		}

		for (i = 1; i < nitems; i++) {
			if (strncasecmp(items[i], "comment:", 7) == 0)
				break;
			if (uade_parse_attribute_from_string(&s->attributes, &s->flags, items[i], lineno))
				continue;
			fprintf(stderr, "song option %s is invalid\n", items[i]);
		}

		goto parse_ok;

	parse_error:
		parse_error = 1;
	parse_ok:
		for (i = 0; items[i] != NULL; i++)
			free_and_null(items[i]);

		free_and_null(items);

		if (parse_error)
			goto error;
	}

	fclose_and_null(f);

	/* we may not have the file locked */
	if (fd >= 0)
		uade_atomic_close(fd);	/* lock is closed too */

	/* Sort MD5 sums for binary searching songs */
	qsort(db->songstore, db->nsongs, sizeof db->songstore[0], escompare);

	snprintf(state->songdbname, sizeof(state->songdbname), "%s", filename);
	return 1;

      error:
	if (f)
		fclose_and_null(f);
	if (fd >= 0)
		uade_atomic_close(fd);
	return 0;
}

void uade_save_content_db(const char *filename, struct uade_state *state)
{
	int fd;
	FILE *f;
	size_t i;
	struct uade_songdb *db = &state->songdb;

	if (db->ccmodified == 0 || db->cccorrupted)
		return;

	fd = uade_open_and_lock(filename, 1);
	if (fd < 0) {
		fprintf(stderr, "uade: Can not write content db: %s\n",
			filename);
		return;
	}

	f = fdopen(fd, "w");
	if (f == NULL) {
		fprintf(stderr, "uade: Can not create a FILE structure for content db: %s\n", filename);
		close(fd);
		return;
	}

	for (i = 0; i < db->nccused; i++) {
		struct uade_content *n = &db->contentchecksums[i];
		fprintf(f, "%s %u\n", n->md5, (unsigned int) n->playtime);
	}

	db->ccmodified = 0;

	fclose(f);

	uade_debug(state, "uade: Saved %zd entries into content db.\n", db->nccused);
}

int uade_test_silence(void *buf, size_t size, struct uade_state *state)
{
	int i, s, exceptioncount;
	int16_t *sm;
	int nsamples;
	int64_t count = state->song.silencecount;
	int end = 0;

	if (state->config.silence_timeout < 0)
		return 0;

	exceptioncount = 0;
	sm = buf;
	nsamples = size / 2;

	for (i = 0; i < nsamples; i++) {
		s = (sm[i] >= 0) ? sm[i] : -sm[i];
		if (s >= (32767 * 1 / 100)) {
			exceptioncount++;
			if (exceptioncount >= (size * 2 / 100)) {
				count = 0;
				break;
			}
		}
	}

	if (i == nsamples) {
		count += size;
		if (count / (UADE_BYTES_PER_FRAME * state->config.frequency) >= state->config.silence_timeout) {
			count = 0;
			end = 1;
		}
	}

	state->song.silencecount = count;

	return end;
}

int uade_update_song_conf(const char *songconf,
			  const char *songname, const char *options)
{
	int ret;
	int fd;
	char md5[33];
	void *mem = NULL;
	size_t filesize, newsize;
	int found = 0;
	size_t inputsize;
	char *input, *inputptr, *outputptr;
	size_t inputoffs;
	char newline[256];
	size_t i;
	int need_newline = 0;

	if (strlen(options) > 128) {
		fprintf(stderr, "Too long song.conf options.\n");
		return 0;
	}

	fd = uade_open_and_lock(songconf, 1);

	input = uade_read_file(&inputsize, songconf);
	if (input == NULL) {
		fprintf(stderr, "Can not read song.conf: %s\n", songconf);
		uade_atomic_close(fd);	/* closes the lock too */
		return 0;
	}

	newsize = inputsize + strlen(options) + strlen(songname) + 64;
	mem = realloc(input, newsize);
	if (mem == NULL) {
		fprintf(stderr,	"Can not realloc the input file buffer for song.conf.\n");
		free(input);
		uade_atomic_close(fd);	/* closes the lock too */
		return 0;
	}
	input = mem;

	mem = uade_read_file(&filesize, songname);
	if (mem == NULL)
		goto error;

	md5_from_buffer(md5, sizeof md5, mem, filesize);

	inputptr = outputptr = input;
	inputoffs = 0;

	while (inputoffs < inputsize) {
		if (inputptr[0] == '#')
			goto copyline;

		if ((inputoffs + 37) >= inputsize)
			goto copyline;

		if (strncasecmp(inputptr, "md5=", 4) != 0)
			goto copyline;

		if (strncasecmp(inputptr + 4, md5, 32) == 0) {
			if (found) {
				fprintf(stderr,
					"Warning: dupe entry in song.conf: %s (%s)\n"
					"Need manual resolving.\n", songname,
					md5);
				goto copyline;
			}
			found = 1;
			snprintf(newline, sizeof newline, "md5=%s\t%s\n", md5,
				 options);

			/* Skip this line. It will be appended later to the end of the buffer */
			for (i = inputoffs; i < inputsize; i++) {
				if (input[i] == '\n') {
					i = i + 1 - inputoffs;
					break;
				}
			}
			if (i == inputsize) {
				i = inputsize - inputoffs;
				found = 0;
				need_newline = 1;
			}
			inputoffs += i;
			inputptr += i;
			continue;
		}

	      copyline:
		/* Copy the line */
		for (i = inputoffs; i < inputsize; i++) {
			if (input[i] == '\n') {
				i = i + 1 - inputoffs;
				break;
			}
		}
		if (i == inputsize) {
			i = inputsize - inputoffs;
			need_newline = 1;
		}
		memmove(outputptr, inputptr, i);
		inputoffs += i;
		inputptr += i;
		outputptr += i;
	}

	if (need_newline) {
		snprintf(outputptr, 2, "\n");
		outputptr += 1;
	}

	/* there is enough space */
	ret = snprintf(outputptr, PATH_MAX + 256, "md5=%s\t%s\tcomment %s\n",
		       md5, options, songname);
	outputptr += ret;

	if (ftruncate(fd, 0)) {
		fprintf(stderr, "Can not truncate the file.\n");
		goto error;
	}

	/* Final file size */
	i = (size_t) (outputptr - input);

	if (uade_atomic_write(fd, input, i) < i)
		fprintf(stderr, "Unable to write file contents back. Data loss happened. CRAP!\n");

      error:
	uade_atomic_close(fd);	/* Closes the lock too */
	free(input);
	free(mem);
	return 1;
}
