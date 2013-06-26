/*
 * @file    istream68_mem.c
 * @brief   implements istream68 VFS for memory buffer
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-15 16:20:00 ben>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "file68_api.h"
#include "istream68_mem.h"

/* define this if you don't want MEM support. */
#ifndef ISTREAM68_NO_MEM

#include "istream68_def.h"
#include "alloc68.h"

#include <stdio.h>
#include <string.h>

/** istream file structure. */
typedef struct {
  istream68_t istream;                  /**< istream function.   */
  char * buffer;                        /**< memory buffer.      */
  int size;                             /**< memory buffer size. */
  int pos;                              /**< current position.   */

  /** Open modes. */
  int mode;        /**< Allowed open mode bit-0:read bit-1:write.   */
  int open;        /**< Currently open mode bit-0:read bit-1:write. */

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[16 + 2 * 2 * sizeof(void*)]; /**< filename (mem://start:end). */

} istream68_mem_t;

static const char * ism_name(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;

  return !ism->name[0]
    ? 0
    : ism->name
    ;
}

static int ism_open(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;

  if (!ism->mode || ism->open) {
    return -1;
  }
  ism->open = ism->mode;
  ism->pos = 0;
  return 0;
}

static int ism_close(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;

  if (!ism->open) {
    return -1;
  }
  ism->open = 0;
  return 0;
}

static int ism_read_or_write(istream68_t * istream, void * data, int n, int mode)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;
  int pos, endpos;

  if (!(ism->open & mode) || n < 0) {
    return -1;
  }
  if (!n) {
    return 0;
  }

  pos = ism->pos;
  endpos = pos + n;
  if (endpos > ism->size) {
    endpos = ism->size;
    n = endpos - pos;
  }
  if (n > 0) {
    void *src, *dst;
    if (mode == ISTREAM68_OPEN_READ) {
      src = ism->buffer+pos;
      dst = data;
    } else {
      src = data;
      dst = ism->buffer+pos;
    }
    memcpy(dst, src, n);
  }
  ism->pos = endpos;
  return n;
}

static int ism_read(istream68_t * istream, void * data, int n)
{
  return ism_read_or_write(istream, data, n, ISTREAM68_OPEN_READ);
}

static int ism_write(istream68_t * istream, const void * data, int n)
{
  void * rw = (void *)data;
  return ism_read_or_write(istream, rw, n, ISTREAM68_OPEN_WRITE);
}

static int ism_flush(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;
  return -!ism->open;
}

static int ism_length(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;

  return !ism->open
    ? -1
    : ism->size
    ;
}

static int ism_tell(istream68_t * istream)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;

  return !ism->open
    ? -1
    : ism->pos
    ;
}

static int ism_seek(istream68_t * istream, int offset)
{
  istream68_mem_t * ism = (istream68_mem_t *)istream;
  int pos;

  if (!ism->open) {
    return -1;
  }
  pos = ism->pos + offset;
  if (pos < 0 || pos > ism->size) {
    return -1;
  }
  ism->pos = pos;
  return 0;
}

static void ism_destroy(istream68_t * istream)
{
  free68(istream);
}

static const istream68_t istream68_mem = {
  ism_name,
  ism_open, ism_close,
  ism_read, ism_write, ism_flush,
  ism_length, ism_tell, ism_seek, ism_seek,
  ism_destroy
};

istream68_t * istream68_mem_create(const void * addr, int len, int mode)
{
  istream68_mem_t *ism;

  if (len < 0 || (!addr && len)) {
    return 0;
  }

  ism = calloc68(sizeof(istream68_mem_t));
  if (!ism) {
    return 0;
  }

  ism->istream = istream68_mem;
  ism->buffer = (char *)addr;
  ism->size   = len;
  ism->mode   = mode & (ISTREAM68_OPEN_READ|ISTREAM68_OPEN_WRITE);
  ism->open   = 0;
  ism->pos    = 0;
  sprintf(ism->name,"mem://%p:%p", addr, (char *)addr+len);

  return &ism->istream;
}

#else /* #ifndef ISTREAM68_NO_FILE */

/* istream mem must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

#include "istream68_mem.h"

istream68_t * istream68_mem_create(const void * addr, int len, int mode)
{
  msg68_error("mem68: create -- *NOT SUPPORTED*\n");
  return 0;
}

#endif
