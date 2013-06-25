#ifndef _UADE_SONGDB_H_
#define _UADE_SONGDB_H_

#include <uade/eagleplayer.h>
#include <uade/vparray.h>
#include <uade/uadeutils.h>

#include <time.h>

struct uade_content {
	char md5[33];
	uint32_t playtime;	/* in milliseconds */
};

struct eaglesong {
	int flags;
	char md5[33];
	struct uade_attribute *attributes;
};

struct uade_songdb {
	struct uade_content *contentchecksums;
	size_t nccused;	      /* number of valid entries in content db */
	size_t nccalloc;      /* number of allocated entries for content db */
	int ccmodified;
	int cccorrupted;
	time_t ccloadtime;
	char ccfilename[PATH_MAX];

	size_t nsongs;
	struct eaglesong *songstore;
};

struct uade_state;

struct uade_content *uade_add_playtime(struct uade_state *state, const char *md5, uint32_t playtime);
void uade_free_song_db(struct uade_state *state);
void uade_lookup_song(const struct uade_file *module, struct uade_state *state);
int uade_read_content_db(const char *filename, struct uade_state *state);
int uade_read_song_conf(const char *filename, struct uade_state *state);
void uade_save_content_db(const char *filename, struct uade_state *state);
int uade_test_silence(void *buf, size_t size, struct uade_state *state);
void uade_unalloc_song(struct uade_state *state);
int uade_update_song_conf(const char *songconf,  const char *songname, const char *options);

#endif
