/*
 * A fifo implementation that is amortized O(n) for reading and writing n bytes.
 * Capacity is at least doubled when the fifo gets full.
 * Capacity is halved when only a fourth of the fifo capacity is used.
 * Maximum capacity of the fifo is approximately half the address space.
 *
 * The fifo source code is in public domain. This does not apply to other files.
 *
 * Originally written by Heikki Orsila <heikki.orsila@iki.fi> in 2012.
 */

#include "fifo.h"

#include <stdlib.h>
#include <string.h>

struct fifo *fifo_create(void)
{
	struct fifo *fifo = calloc(1, sizeof(fifo[0]));
	if (fifo == NULL)
		return NULL;
	return fifo;
}

int fifo_erase_tail(struct fifo *fifo, size_t bytes)
{
	if (fifo_len(fifo) < bytes)
		return -1;
	fifo->upper -= bytes;
	assert(fifo->lower <= fifo->upper);
	return 0;
}

void fifo_free(struct fifo *fifo)
{
	if (fifo == NULL)
		return;
	free(fifo->buf);
	memset(fifo, 0, sizeof fifo[0]);
	free(fifo);
}

static void halve_fifo(struct fifo *fifo)
{
	size_t len = fifo_len(fifo);
	size_t newcapacity;
	uint8_t *newbuf;

	if (fifo->capacity < 8)
		return;

	assert((fifo->lower + len) <= fifo->upper);
	memmove(fifo->buf, fifo->buf + fifo->lower, len);

	fifo->lower = 0;
	fifo->upper = len;

	newcapacity = fifo->capacity / 2;
	assert(newcapacity >= len);
	newbuf = realloc(fifo->buf, newcapacity);
	if (newbuf == NULL)
		return;

	fifo->buf = newbuf;
	fifo->capacity = newcapacity;
}

size_t fifo_read(void *data, size_t bytes, struct fifo *fifo)
{
	size_t len = fifo_len(fifo);
	if (!len)
		return 0;

	if (bytes > len)
		bytes = len;

	assert(fifo->lower + bytes <= fifo->upper);
	memcpy(data, fifo->buf + fifo->lower, bytes);
	fifo->lower += bytes;
	assert(fifo->lower <= fifo->upper);

	if (fifo_len(fifo) <= (fifo->capacity / 4))
		halve_fifo(fifo);

	return bytes;
}

int fifo_write(struct fifo *fifo, const void *data, size_t bytes)
{
	assert(bytes < (((size_t) -1) / 2));

	if ((fifo->upper + bytes) > fifo->capacity) {
		void *newbuf;
		size_t newcapacity = (fifo->upper + bytes) * 2;
		if (newcapacity > (((size_t) -1) / 2))
			return -1;
		assert(fifo->capacity <= (((size_t) -1) / 2));
		newbuf = realloc(fifo->buf, newcapacity);
		if (newbuf == NULL)
			return -1;
		fifo->buf = newbuf;
		fifo->capacity = newcapacity;
	}

	assert((fifo->upper + bytes) <= fifo->capacity);

	memcpy(fifo->buf + fifo->upper, data, bytes);

	fifo->upper += bytes;
	assert(fifo->upper <= fifo->capacity);

	return 0;
}
