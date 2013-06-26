/*
 * @file    gzip68.c
 * @brief   gzip file loader
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-02 16:04:11 ben>
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
#include "gzip68.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_ZLIB_H
# include <zlib.h>
#else
# define Z_DEFLATED   8 /* From zlib.h */
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

/* gzip magic header */
static char gz_magic[] = {0x1f, (char)0x8b, Z_DEFLATED};

int gzip68_is_magic(const void * buffer)
{
  return !memcmp(gz_magic, buffer, sizeof(gz_magic));
}

#ifdef HAVE_ZLIB_H

#include "error68.h"
#include "alloc68.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _MSC_VER
# include <stdio.h>
# include <io.h>
#else
# include <unistd.h>
#endif


static int is_gz(int fd, int len)
{
  char magic[sizeof(gz_magic)];
  int inflate_len = -1;

  /* header + inflate len */
  if (len < 10+4) {
    goto error;
  }

  /* Read and check magic. */
  if (read(fd, magic, sizeof(magic)) != sizeof(magic) ||
      memcmp(magic, gz_magic, sizeof(magic))) {
    goto error;
  }

  /* Get uncompressed size at the end of file.
   */
  if (lseek(fd, -4, SEEK_END) == (off_t) -1) {
    goto error;
  } else {
    unsigned char buffer[4];
    if(read(fd, buffer, 4) != 4) {
      goto error;
    }
    inflate_len =
      buffer[0]
      | (buffer[1] << 8)
      | (buffer[2] << 16)
      | (buffer[3] << 24);
    if (inflate_len < 0) {
      inflate_len = -1;
    }
  }
error:

  /* Rewind */
  lseek(fd, 0, SEEK_SET);
  return inflate_len;
}


/* Windows opens streams in Text mode by default. */
#ifndef _O_BINARY
# ifdef O_BINARY
#  define _O_BINARY O_BINARY
# else
#  define _O_BINARY 0
# endif
#endif

void *gzip68_load(const char *fname, int *ptr_ulen)
{
  int fd, err;
  gzFile f = 0;
  int ulen = 0;
  void * uncompr = 0;
  off_t len;
  const int omode = O_RDONLY | _O_BINARY;

  fd = open(fname, omode);
  if (fd == -1) {
    error68("gzip68: load '%s' -- %s", fname, strerror(errno));
    goto error;
  }

  len = lseek(fd, 0, SEEK_END);
  if (len == (off_t) -1) {
    error68("gzip68: load '%s' -- %s", fname, strerror(errno));
    goto error;
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    error68("gzip68: load '%s' -- %s", fname, strerror(errno));
    goto error;
  }

  ulen = is_gz(fd, len);
  if (ulen == -1) {
    ulen = len;
  }

  f = gzdopen(fd, "rb");
  if (!f) {
    error68("gzip68: load '%s' -- %s", fname, gzerror(f, &err));
    goto error;
  }
  fd = 0; /* $$$ Closed by gzclose(). Verify fdopen() rules. */

  uncompr = alloc68(ulen);
  if (!uncompr) {
    error68("gzip68: load '%s' -- alloc error", fname);
    goto error;
  }
  len = gzread(f, uncompr, ulen);

  if (len != ulen) {
    error68("gzip68: load '%s' -- read error (%s)",fname, gzerror(f, &err));
    goto error;
  }
  goto end;

error:
  if (uncompr) {
    free68(uncompr);
    uncompr = 0;
    ulen = 0;
  }

end:
  if (fd) {
    close(fd);
  }
  if (f) {
    gzclose(f);
  }
  if (ptr_ulen) {
    *ptr_ulen = ulen;
  }

  return uncompr;
}

#else

#include "error68.h"

void *gzip68_load(const char *fname, int *ptr_ulen)
{
  if (ptr_ulen) *ptr_ulen=0;
  error68("gzip68: *NOT SUPPORTED*");
  return 0;
}

#endif /* #ifdef HAVE_ZLIB_H */
