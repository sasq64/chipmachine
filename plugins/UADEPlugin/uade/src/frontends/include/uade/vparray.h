#ifndef _SHD_VPARRAY_H_
#define _SHD_VPARRAY_H_

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

struct vparray {
	size_t head;
	size_t tail;
	size_t allocated;
	void **l;
};


/* Don't touch */
void vparray_grow(struct vparray *v);

/* Add one element to the end of the list. Asymptotic: amortically O(1) */
static inline void vparray_append(struct vparray *v, void *item)
{
	if (v->tail == v->allocated)
		vparray_grow(v);

	v->l[v->tail++] = item;
}

/* Return void ** to the beginning of valid array inside the list. This
   is the same as &v->l[v->head]. Items before this are invalid. Asymptotic:
   trivially O(1) */
static inline void **vparray_array(struct vparray *v)
{
	return &v->l[v->head];
}

/* Same as vparray_search, but faster. vparray_sort() must be used before using
   this. Also, the function must be given the same sort function as
   vparray_sort() was given. The function returns an index of the found
   element, or -1 if not found. See vparray_get() to obtain the element based
   on the index. Asymptotic: O(log N) */
ssize_t vparray_bsearch(void *key, struct vparray *v,
			int (*compar)(const void *, const void *));

/* Create a new data structure */
struct vparray *vparray_create(size_t initial_length);

/* Remove all elements from the list */
void vparray_flush(struct vparray *v);

/* Free memory of the data structure. Asymptotic: that of free(array) */
void vparray_free(struct vparray *v);

/* Get one element by index in range [0, N), where N = vparray_len(v).
   Asymptotic: trivially O(1) */
static inline void *vparray_get(struct vparray *v, size_t i)
{
	assert(i < (v->tail - v->head));
	return v->l[v->head + i];
}

/* Return the number of valid items in the list. Asymptotic: trivially O(1) */
static inline size_t vparray_len(struct vparray *v)
{
	return v->tail - v->head;
}

/* Functionally same as vparray_remove(v, 0). Asymptotic: amortically O(1) */
void *vparray_pop_head(struct vparray *v);

/* Functionally same as vparray_remove(v, vparray_len(v) - 1). Asymptotic:
   amortically O(1) */
void *vparray_pop_tail(struct vparray *v);

/*
 * Remove index i from the list. WARNING: this function mixes the order of
 * entries in the array. In other words, do not mix with pop_head() and
 * pop_tail(). Asymptotic: that of pop_tail().
 */
void vparray_remove(struct vparray *v, size_t i);

/* Find a void *key from the list, returns the index of the element if found,
   otherwise returns -1. See vparray_get() to obtain the element based on the
   index. Asymptotic: O(n) */
ssize_t vparray_search(void *key, struct vparray *v);

/* Sort the list. The given compare function must act like the one given for
   qsort(). The compare function gets two pointers, which must be regarded
   as void ** types, because stdlib's qsort() passes pointers to the
   internal void * array of vparray. A typical compare function should look
   like this:
     int compar(void *x, void *y)
     {
       void *xdata = * (void **) x;
       void *ydata = * (void **) y;
       return compare_datas(x, y);
     }
   This wrapper was not implemented to vparray_sort() to avoid function call
   overhead.

   Note, any modification to the list may unsort the list (adding and removing
   items). If vparray_bsearch() is used, the list
   must always be re-sorted after adding or removing elements. Asymptotic:
   that of qsort() */
void vparray_sort(struct vparray *v, int (*compar)(const void *, const void *));

#endif
