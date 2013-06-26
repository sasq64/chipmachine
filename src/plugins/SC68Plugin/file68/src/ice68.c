/*
 * @file    ice68.c
 * @brief   ICE! file loader
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-02 16:04:06 ben>
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
#include "ice68.h"
#include "istream68_def.h"

#include "error68.h"

#ifdef USE_UNICE68

#include "msg68.h"
#include "alloc68.h"
#include "istream68_file.h"
#include "unice68.h"

#include <string.h>

int ice68_version(void)
{
  return unice68_ice_version();
}

int ice68_is_magic(const void * buffer)
{
  return unice68_get_depacked_size(buffer,0) > 0;
}

void * ice68_load(istream68_t *is, int *ulen)
{
  char header[12], *inbuf = 0, * outbuf = 0;
  int dsize, csize;
  const char * fname;

  fname = istream68_filename(is);

  if (istream68_read(is,header,12) != 12) {
    error68("ice68: load '%s' [no header]", fname);
    goto error;
  }

  csize = 0;
  dsize = unice68_get_depacked_size(header, &csize);

  if (dsize < 0) {
    error68("ice68: load '%s' [not ICE!]", fname);
    goto error;
  }

  inbuf = alloc68(csize+12);

  if (!inbuf) {
    error68("ice68: load '%s' [alloc input buffer failed]", fname);
    goto error;
  }

  memcpy(inbuf,header,12);
  if (istream68_read(is,inbuf+12,csize) != csize) {
    error68("ice68: load '%s' [read failed]", fname);
    goto error;
  }

  outbuf = alloc68(dsize);

  if (!outbuf) {
    error68("ice68: load '%s' [alloc output buffer failed]", fname);
    goto error;
  }

  if (!unice68_depacker(outbuf, inbuf)) {
    goto success;
  }

error:

  free68(outbuf);
  outbuf = 0;
  dsize = 0;
success:
  free68(inbuf);
  if (ulen) {
    *ulen = dsize;
  }
  return outbuf;
}

void * ice68_load_file(const char * fname, int * ulen)
{
  void * ret = 0;
  istream68_t * is;

  is = istream68_file_create(fname, 1);
  if (istream68_open(is) != -1) {
    ret = ice68_load(is, ulen);
    istream68_close(is);
  }
  istream68_destroy(is);

  return ret;
}

#else /* #ifdef USE_UNICE68 */

int ice68_version(void)
{
  return 0;
}

int ice68_is_magic(const void * buffer)
{
  return buffer
    && 0[(char *)buffer] == 'I'
    && 1[(char *)buffer] == 'C'
    && 2[(char *)buffer] == 'E'
    && 3[(char *)buffer] == '!';
}

void * ice68_load(istream68_t * is, int * ulen)
{
  const char * fname = istream68_filename(is);
  error68("ice68: *NOT SUPPORTED*");
  return 0;
}

void * ice68_load_file(const char * fname, int * ulen)
{
  error68("ice68: *NOT SUPPORTED*");
  return 0;
}

#endif /* #ifdef USE_UNICE68 */
