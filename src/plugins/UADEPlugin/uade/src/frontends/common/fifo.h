#ifndef _BUFFER_FIFO_H_
#define _BUFFER_FIFO_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct fifo {
	size_t lower;
	size_t upper;
	size_t capacity;
	uint8_t *buf; /* There is valid data in range [lower, upper) */
};

/* Create a fifo */
struct fifo *fifo_create(void);

/*
 * Erase a number of bytes on the tail of the fifo. This erases the last written
 * bytes on the fifo. This can be used to cancel the latest write to fifo.
 *
 * Returns 0 if successful (enough bytes), -1 otherwise (not enough bytes).
 */
int fifo_erase_tail(struct fifo *fifo, size_t bytes);

/*
 * Frees the fifo. You may not use the fifo after calling this.
 * Passing NULL to fifo_free() has no effect, same as with free().
 */
void fifo_free(struct fifo *fifo);

/*
 * Flushes away all data from fifo. This does not free memory, so the
 * operation is fast.
*/
static inline void fifo_flush(struct fifo *fifo)
{
	fifo->lower = 0;
	fifo->upper = 0;
}

static inline size_t fifo_len(const struct fifo *fifo)
{
	assert(fifo->lower <= fifo->upper);
	return fifo->upper - fifo->lower;
}

/*
 * Returns the number of bytes read. It can return a value less than the
 * requested number of bytes.
 */
size_t fifo_read(void *data, size_t bytes, struct fifo *fifo);

/* Return 0 on success, -1 on error (no memory or too large buffer). */
int fifo_write(struct fifo *fifo, const void *data, size_t bytes);

#endif
