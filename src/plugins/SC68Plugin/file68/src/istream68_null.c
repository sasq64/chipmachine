/*
 * @file    istream68_null.c
 * @brief   implements istream68 VFS for for null
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-15 16:18:03 ben>
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
#include "istream68_null.h"

/* define this if you don't want NULL support. */
#ifndef ISTREAM68_NO_NULL

#include "istream68_def.h"
#include "alloc68.h"

#include <string.h>

/** istream file structure. */
typedef struct {
  istream68_t istream; /**< istream function.           */
  int size;            /**< max pos R or W.             */
  int pos;             /**< current position.           */
  int open;            /**< has been opened.            */
  char name[1];        /**< filename (null://filename). */
} istream68_null_t;

static const char * isn_name(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  return (!isn->name[0])
    ? 0
    : isn->name
    ;
}

static int isn_open(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  if (isn->open) {
    return -1;
  }
  isn->open = 1;
  isn->pos  = 0;
  isn->size = 0;
  return 0;
}

static int isn_close(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  if (!isn->open) {
    return -1;
  }
  isn->open = 0;
  return 0;
}

static int isn_read_or_write(istream68_t * istream, int n)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  if (!isn->open || n < 0) {
    return -1;
  }
  if (n) {
    /* No op: do not update size */
    isn->pos += n;
    if (isn->pos > isn->size) {
      isn->size = isn->pos;
    }
  }
  return n;
}

static int isn_read(istream68_t * istream, void * data, int n)
{
  return isn_read_or_write(istream, n);
}

static int isn_write(istream68_t * istream, const void * data, int n)
{
  return isn_read_or_write(istream, n);
}

static int isn_flush(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;
  return -!isn->open;
}

static int isn_length(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  return isn->size;
}

static int isn_tell(istream68_t * istream)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  return !isn->open
    ? -1
    : isn->pos
    ;
}

static int isn_seek(istream68_t * istream, int offset)
{
  istream68_null_t * isn = (istream68_null_t *)istream;

  if (isn) {
    offset += isn->pos;
    if (offset >= 0) {
      isn->pos = offset;
      return 0;
    }
  }
  return -1;
}

static void isn_destroy(istream68_t * istream)
{
  free68(istream);
}

static const istream68_t istream68_null = {
  isn_name,
  isn_open, isn_close,
  isn_read, isn_write, isn_flush,
  isn_length, isn_tell, isn_seek, isn_seek,
  isn_destroy
};

istream68_t * istream68_null_create(const char * name)
{
  istream68_null_t *isn;
  int size;
  static const char hd[] = "null://";

  if (!name) {
    name = "default";
  }

  size = sizeof(istream68_null_t) + sizeof(hd)-1 + strlen(name);

  isn = alloc68(size);
  if (!isn) {
    return 0;
  }

  isn->istream = istream68_null;
  isn->size    = 0;
  isn->pos     = 0;
  isn->open    = 0;
  strcpy(isn->name,hd);
  strcat(isn->name,name);

  return &isn->istream;
}

#else /* #ifndef ISTREAM68_NO_NULL */

/* istream mem must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

#include "file68/istream68_null.h"

istream68_t * istream68_null_create(const char * name)
{
  msg68_error("null68: create -- *NOT SUPPORTED*\n");
  return 0;
}

#endif
