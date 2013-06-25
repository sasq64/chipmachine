/*
 * Copyright (C) Heikki Orsila 2003-2010
 * email: heikki.orsila@iki.fi
 * License: LGPL and GPL
*/

#include <uade/uade.h>
#include <uade/unixatomic.h>
#include <uade/ossupport.h>

#include "playlist.h"
#include "unixwalkdir.h"
#include "uade123.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>

static int random_fd = -1;
#ifdef UADE_CONFIG_HAVE_URANDOM
static int using_urandom = 1;
#else
static int using_urandom = 0;
#endif

static void random_init(void)
{
	if (using_urandom == 0)
		srandom(time(NULL));
}

static int get_random(int max)
{
	ssize_t ret;
	uint32_t rdata;
	if (!using_urandom)
		goto nourandom;

	if (random_fd == -1) {
		random_fd = open("/dev/urandom", O_RDONLY);
		if (random_fd < 0) {
			fprintf(stderr, "not using urandom anymore: %s\n", strerror(errno));
			using_urandom = 0;
			goto nourandom;
		}
	}

	ret = uade_atomic_read(random_fd, &rdata, sizeof rdata);
	if (ret != ((size_t) sizeof rdata)) {
		fprintf(stderr, "error on reading urandom: %s\n", strerror(errno));
		using_urandom = 0;
		goto nourandom;
	}

	return (((double) max) * (rdata & 0x3fffffff)) / 0x40000000;

nourandom:
	return (((double) max) * random()) / (RAND_MAX + 1.0);
}

int playlist_init(struct playlist *pl)
{
	random_init();
	pl->pos = 0;
	pl->randomize = 0;
	pl->repeat = 0;
	pl->valid = (chrarray_init(&pl->list) == 0);
	return pl->valid;
}

void playlist_iterator(struct playlist_iterator *pli, struct playlist *pl)
{
	assert(pl != NULL);
	pli->index = 0;
	pli->pl = pl;
}

char *playlist_iterator_get(struct playlist_iterator *pli)
{
	char *s;
	struct chrarray *arr;
	struct chrentry *e;

	arr = &pli->pl->list;

	if (pli->index >= arr->n_entries)
		return NULL;

	e = &arr->entries[pli->index];
	s = &arr->data[e->off];

	pli->index++;

	return s;
}

/* enable == 0: disable random play
   enable == 1: enable random play
   enable == -1: toggle random play state between enabled and disabled

   Returns new state value.
*/
int playlist_random(struct playlist *pl, int enable)
{
	if (enable < 0) {
		pl->randomize = pl->randomize ? 0 : 1;
	} else {
		pl->randomize = enable ? 1 : 0;
	}
	return pl->randomize;
}

/* Shuffle the whole deck at once */
void playlist_randomize(struct playlist *pl)
{
	size_t i;
	size_t n;
	struct chrentry t;
	struct chrarray *l = &pl->list;
	size_t ri;

	assert(l->n_entries >= 0);

	n = l->n_entries;

	for (i = 0; i < n; i++) {
		ri = i + get_random(n - i);
		if (ri != i) {
			/* swap i and ri */
			t = l->entries[i];
			l->entries[i] = l->entries[ri];
			l->entries[ri] = t;
		}
	}
}

void playlist_repeat(struct playlist *pl)
{
	pl->repeat = 1;
}

int playlist_empty(struct playlist *pl)
{
	if (!pl->valid) {
		fprintf(stderr, "playlist invalid\n");
		return 1;
	}
	if (pl->list.n_entries == 0)
	  return 1;
	if (pl->repeat)
		return 0;
	return pl->pos == pl->list.n_entries;
}

static void *recursive_func(const char *file, enum uade_wtype wtype, void *pl)
{
	if (wtype != UADE_WALK_REGULAR_FILE)
		return NULL;
	if (!playlist_add(pl, file, 0))
		fprintf(stderr, "error enqueuing %s\n", file);
	return NULL;
}

int playlist_add(struct playlist *pl, const char *name, int recursive)
{
	int ret = 0;
	struct stat st;
	char *path = strdup(name);

	if (path == NULL) {
		fprintf(stderr, "playlist_add: No memory for path.\n");
		goto out;
	}

	if (!pl->valid)
		goto out;

	if (stat(path, &st))
		goto out;

	if (S_ISREG(st.st_mode)) {
		/* Is a regular file */
		ret = (chrarray_add(&pl->list, path, strlen(path) + 1) >= 0);

	} else if (S_ISDIR(st.st_mode)) {
		/* add directories to playlist only if 'recursive' is non-zero */
		if (recursive) {

			/* strip directory path of ending '/' characters */
			char *strippedpath = strdup(path);
			size_t len = strlen(path);

			if (strippedpath == NULL)
				uade_die("Not enough memory for directory path.\n");

			while (len > 0) {
				len--;
				if (strippedpath[len] != '/')
					break;
				strippedpath[len] = 0;
			}

			/* walk directory hierarchy */
			uade_walk_directories(strippedpath, recursive_func, pl);

			/* free stripped path */
			free(strippedpath);

		} else {
			fprintf(stderr, "Not adding directory %s. Use -r to add recursively.\n", path);
		}
		ret = 1;
	}

out:
	free(path);
	return ret;
}

static int pl_get_random(char **s, int *len, struct playlist *pl)
{
	int i;
	struct chrentry t;

	pl->pos++;
	if (pl->pos >= pl->list.n_entries) {
		if (!pl->repeat)
			return 0;
		pl->pos = 0;
	}

	i = pl->pos + get_random(pl->list.n_entries - pl->pos);

	t = pl->list.entries[i];

	if (i != pl->pos) {
		pl->list.entries[i] = pl->list.entries[pl->pos];
		pl->list.entries[pl->pos] = t;
	}

	*s = &pl->list.data[t.off];
	*len = t.len;
	return 1;
}

static void pl_get_cur(char **s, int *len, struct playlist *pl)
{
	struct chrentry *t;
	t = &pl->list.entries[pl->pos];
	*s = &pl->list.data[t->off];
	*len = t->len;
}

static int pl_get_next(char **s, int *len, struct playlist *pl)
{
	pl->pos++;
	if (pl->pos >= pl->list.n_entries) {
		if (!pl->repeat)
			return 0;
		pl->pos = 0;
	}

	pl_get_cur(s, len, pl);
	return 1;
}

static void pl_get_prev(char **s, int *len, struct playlist *pl)
{
	if (pl->pos == 0) {
		if (pl->repeat)
			pl->pos = pl->list.n_entries - 1;
	} else {
		pl->pos--;
	}

	pl_get_cur(s, len, pl);
}

int playlist_get(char *name, size_t maxlen, struct playlist *pl, int dir)
{
	int len;
	char *s;

	if (!pl->valid)
		return 0;

	if (pl->list.n_entries == 0)
		return 0;

	if (!maxlen) {
		fprintf(stderr, "uade123: playlist_get(): given maxlen = 0\n");
		return 0;
	}

	if (dir == UADE_PLAY_NEXT) {
		if (pl->randomize) {
			if (pl_get_random(&s, &len, pl) == 0)
				return 0;
		} else {
			if (pl_get_next(&s, &len, pl) == 0)
				return 0;
		}
	} else if (dir == UADE_PLAY_PREVIOUS) {
		pl_get_prev(&s, &len, pl);
	} else if (dir == UADE_PLAY_CURRENT) {
		pl_get_cur(&s, &len, pl);
	} else {
		uade_die("invalid playlist direction: %d\n", dir);
	}

	if (len > maxlen) {
		fprintf(stderr, "uade: playlist_get(): too long a string: %s\n", s);
		return 0;
	}

	memcpy(name, s, len);
	return 1;
}
