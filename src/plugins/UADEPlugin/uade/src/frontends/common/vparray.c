/*  Amortized O(1) dynamic list written by
    Heikki Orsila <heikki.orsila@iki.fi>. The code is in public domain, so
    it may be used without restrictions for anything.

    RESTRICTIONS: sizeof(size_t) must be same as sizeof(void *). In other
    words, the whole memory space must be indexable with a size_t. If this
    does not hold, do not use this data structure.
*/

#include <uade/vparray.h>

#include <stdio.h>
#include <string.h>

#define BASIC_LENGTH 5


static void shrink_vparray(struct vparray *v, size_t newsize)
{
	size_t ncopied = v->tail - v->head;
	void **newl;
	if (newsize >= v->allocated) {
		fprintf(stderr, "vparray not shrinked.\n");
		return;
	}

	memmove(v->l, vparray_array(v), ncopied * sizeof(v->l[0]));
	v->head = 0;
	v->tail = ncopied;
	v->allocated = newsize;
	if ((newl = realloc(v->l, v->allocated * sizeof(v->l[0]))) == NULL) {
		fprintf(stderr, "Not enough memory for shrinking vparray.\n");
		abort();
	}
	v->l = newl;
}


ssize_t vparray_bsearch(void *key, struct vparray *v,
		       int (*compar)(const void *, const void *))
{
	void *res;
	void **array = vparray_array(v);
	size_t n = vparray_len(v);

	res = bsearch(&key, array, n, sizeof(v->l[0]), compar);
	if (res == NULL)
		return -1;

	return (ssize_t) (((size_t) res - (size_t) array) / sizeof(void *));
}


void vparray_grow(struct vparray *v)
{
	size_t newsize = v->allocated * 2;
	void **newl;

	if (newsize == 0)
		newsize = BASIC_LENGTH;

	newl = realloc(v->l, newsize * sizeof(v->l[0]));
	if (newl == NULL) {
		fprintf(stderr, "Not enough memory for growing vparray.\n");
		abort();
	}

	v->l = newl;
	v->allocated = newsize;
}


struct vparray *vparray_create(size_t initial_length)
{
	struct vparray *v;

	v = calloc(1, sizeof(*v));
	if (v == NULL) {
		fprintf(stderr, "No memory for vparray.\n");
		abort();
	}

	if (initial_length == 0)
		initial_length = BASIC_LENGTH;

	v->allocated = initial_length;

	v->l = malloc(v->allocated * sizeof(v->l[0]));
	if (v->l == NULL) {
		fprintf(stderr, "Can not create a vparray.\n");
		abort();
	}

	return v;
}


void vparray_flush(struct vparray *v)
{
	v->head = 0;
	v->tail = 0;

	if (v->allocated >= (2 * BASIC_LENGTH))
		shrink_vparray(v, BASIC_LENGTH);
}


void vparray_free(struct vparray *v)
{
	free(v->l);
	memset(v, 0, sizeof(*v));
	free(v);
}


void *vparray_pop_head(struct vparray *v)
{
	void *item;

	if (v->head == v->tail) {
		fprintf(stderr, "Error: can not pop head from an empty vparray.\n");
		abort();
	}

	item = v->l[v->head++];

	/* If 3/4 of a list is unused, free half the list */
	if (v->allocated >= BASIC_LENGTH && v->head >= ((v->allocated / 4) * 3))
		shrink_vparray(v, v->allocated / 2);

	return item;
}


void *vparray_pop_tail(struct vparray *v)
{
	void *item;

	if (v->head == v->tail) {
		fprintf(stderr, "Error: can not pop tail from an empty vparray.\n");
		abort();
	}

	item = v->l[v->tail--];

	/* If 3/4 of a list is unused, free half the list */
	if (v->allocated >= BASIC_LENGTH && v->tail < (v->allocated / 4))
		shrink_vparray(v, v->allocated / 2);

	return item;
}


void vparray_remove(struct vparray *v, size_t i)
{
	size_t n = vparray_len(v);
	void **array = vparray_array(v);

	assert(i < n);

	array[i] = array[n - 1];
	vparray_pop_tail(v);
}


ssize_t vparray_search(void *key, struct vparray *v)
{
	size_t i;
	size_t n = vparray_len(v);
	void **array = vparray_array(v);

	for (i = 0; i < n; i++) {
		if (array[i] == key)
			return (ssize_t) i;
	}

	return -1;
}


void vparray_sort(struct vparray *v, int (*compar)(const void *, const void *))
{
	size_t n = vparray_len(v);

	if (n <= 1)
		return;

	qsort(vparray_array(v), n, sizeof v->l[0], compar);
}
