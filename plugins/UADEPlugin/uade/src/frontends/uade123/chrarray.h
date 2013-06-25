/* chrarray:
   Copyright (C) Heikki Orsila 2003-2005
   Licensed under LGPL and GPL.
   email: heikki.orsila@eetut.fi
*/

#ifndef _CHRARRAY_H_
#define _CHRARRAY_H_

struct chrentry {
	int off;
	int len;
};

struct chrarray {
	int max_entries;
	int n_entries;
	int max_data_size;
	int data_size;
	struct chrentry *entries;
	char *data;
};

int chrarray_init(struct chrarray *s);
void chrarray_destroy(struct chrarray *s);
int chrarray_add(struct chrarray *s, const char *data, int len);
void chrarray_flush(struct chrarray *s);

#endif
