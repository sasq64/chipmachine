/* chrarray:
   Copyright (C) Heikki Orsila 2003-2005
   Licensed under LGPL and GPL.

   TODO:
    - remove from tail
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chrarray.h"

int chrarray_init(struct chrarray *s)
{
  s->n_entries = 0;
  s->max_entries = 1;
  s->entries = malloc(s->max_entries * sizeof(struct chrentry));
  s->data_size = 0;
  s->max_data_size = 16;
  s->data = malloc(s->max_data_size);
  if (!s->entries || !s->data) {
    free(s->entries);
    free(s->data);
    return 0;
  }
  return 1;
}

void chrarray_destroy(struct chrarray *s)
{
  free(s->data);
  free(s->entries);
  memset(s, 0, sizeof(*s));
}

int chrarray_add(struct chrarray *s, const char *data, int len)
{
  int new_max;
  char *new_data;
  char *entry;

  /* loop while free space < len. On every iteration double the max_data_size
     until there is enough space. */
  while ((s->max_data_size - s->data_size) < len) {
    new_max = s->max_data_size * 2;
    if (!(new_data = realloc(s->data, new_max))) {
      return 0;
    }
    s->max_data_size = new_max;
    s->data = new_data;
  }

  if (s->n_entries == s->max_entries) {
    struct chrentry *new_entries;
    new_max = s->max_entries * 2;
    if (!(new_entries = realloc(s->entries, new_max * sizeof(struct chrentry)))) {
      return 0;
    }
    s->max_entries = new_max;
    s->entries = new_entries;
  }

  entry = &s->data[s->data_size];
  memcpy(entry, data, len);
  s->entries[s->n_entries].off = s->data_size;
  s->entries[s->n_entries].len = len;
  s->data_size += len;
  s->n_entries++;

  return s->n_entries;
}

void chrarray_flush(struct chrarray *s)
{
  s->n_entries = 0;
  s->data_size = 0;
}
