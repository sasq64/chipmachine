#ifndef _UADE123_PLAYLIST_H_
#define _UADE123_PLAYLIST_H_

#include <stdio.h>
#include <stdlib.h>

#include "chrarray.h"

struct playlist {
  int valid;
  size_t pos;
  int randomize;
  int repeat;
  struct chrarray list;
};


struct playlist_iterator {
  size_t index;
  struct playlist *pl;
};


enum {
  UADE_PLAY_PREVIOUS = -1,
  UADE_PLAY_CURRENT,
  UADE_PLAY_NEXT,
  UADE_PLAY_EXIT,
  UADE_PLAY_FAILURE
};


int playlist_add(struct playlist *pl, const char *name, int recursive, int cygwin);
int playlist_empty(struct playlist *pl);
int playlist_get(char *name, size_t maxlen, struct playlist *pl, int dir);
int playlist_init(struct playlist *pl);
void playlist_iterator(struct playlist_iterator *pli, struct playlist *pl);
char *playlist_iterator_get(struct playlist_iterator *pli);
int playlist_random(struct playlist *pl, int enable);
void playlist_randomize(struct playlist *pl);
void playlist_repeat(struct playlist *pl);

#endif
