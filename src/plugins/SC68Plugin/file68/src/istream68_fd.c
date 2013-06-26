/*
 * @file    istream68_fd.c
 * @brief   implements istream68 VFS for file descriptor
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-20 13:15:29 ben>
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
#include "istream68_fd.h"

/* define this if you don't want file descriptor support. */
#ifndef ISTREAM68_NO_FD

#include "istream68_def.h"
#include "alloc68.h"

#include <sys/types.h>
#include <sys/stat.h>
#if defined(_MSC_VER)
# include <stdio.h>
# include <io.h>
#else
# ifdef __MINGW32__ /* mingw defines SEEK_CUR and pals in stdio.h */
#  include <stdio.h>
# endif
# include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>

/** istream file structure. */
typedef struct {
  istream68_t istream;            /**< istream function.            */
  int fd;                         /**< File descriptor (-1:closed). */
  int org_fd;                     /**< Original file descriptor.    */
  int mode;                       /**< Open modes.                  */

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[1];                   /**< filename.                    */

} istream68_fd_t;

static const char * ifdname(istream68_t * istream)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;

  return (!isf->name[0])
    ? 0
    : isf->name;
}

static int ifdopen(istream68_t * istream)
{
  int imode;
  istream68_fd_t * isf = (istream68_fd_t *)istream;

  if (!isf->name || isf->fd != -1) {
    return -1;
  }

  if (isf->org_fd != -1) {
    isf->fd = isf->org_fd;
    return 0;
  }

  switch (isf->mode) {
  case ISTREAM68_OPEN_READ:
    imode = O_RDONLY;
    break;
  case ISTREAM68_OPEN_WRITE:
    imode = O_WRONLY | O_CREAT | O_TRUNC;
    break;
  case ISTREAM68_OPEN_READ|ISTREAM68_OPEN_WRITE:
    imode = O_RDWR | O_CREAT;
    break;
  default:
    return -1;
  }
#ifdef _O_BINARY
  imode |= _O_BINARY;
#endif

  isf->fd = open(isf->name, imode);
  return -(isf->fd == -1);
}

static int ifdclose(istream68_t * istream)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;
  int fd;

  if (isf->fd == -1) {
    return -1;
  }
  fd = isf->fd;
  isf->fd = -1;
  /* $$$ Close even if org_fd was given. Is it a good thing ? */
  return close(fd);
}

static int ifdread(istream68_t * istream, void * data, int n)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;
  return  (isf->fd == -1)
    ? -1
    : read(isf->fd, data, n);
}

static int ifdwrite(istream68_t * istream, const void * data, int n)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;
  return isf->fd == -1
    ? -1
    : write(isf->fd, data, n)
    ;
}

#if defined(HAVE_FDATASYNC)
# define MY_FSYNC(fd) fdatasync(fd)
#elif defined(HAVE_FSYNC)
# define MY_FSYNC(fd) fsync(fd)
#else
# define MY_FSYNC(fd) 0
#endif

static int ifdflush(istream68_t * istream)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;
  return isf->fd == -1
    ? -1
    : MY_FSYNC(isf->fd)
    ;
}

/* We could have store the length value at opening, but this way it handles
 * dynamic changes of file size.
 */
static int ifdlength(istream68_t * istream)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;
  off_t pos,len = -1;

  if (isf->fd == -1) {
    return -1;
  }

  /* save current position. */
  pos = lseek(isf->fd,0,SEEK_CUR);
  if (pos != (off_t)-1) {
    /* seek to end of file to get file position (AKA length) */
    len = lseek(isf->fd, 0, SEEK_END);
    /* restore saved position. */
    lseek(isf->fd, pos, SEEK_SET);
  }
  return len;
}

static int ifdtell(istream68_t * istream)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;

  return isf->fd == -1
    ? -1
    : lseek(isf->fd,0,SEEK_CUR)
    ;
}

static int ifdseek(istream68_t * istream, int offset)
{
  istream68_fd_t * isf = (istream68_fd_t *)istream;

  return isf->fd == -1
    ? -1
    : lseek(isf->fd, offset, SEEK_CUR)
    ;
}

static void ifddestroy(istream68_t * istream)
{
  free68(istream);
}

static const istream68_t istream68_fd = {
  ifdname,
  ifdopen, ifdclose,
  ifdread, ifdwrite, ifdflush,
  ifdlength, ifdtell,
  ifdseek, ifdseek,
  ifddestroy
};

istream68_t * istream68_fd_create(const char * fname, int fd, int mode)
{
  istream68_fd_t *isf;
  int len;

  if (fd == -1 && (!fname || !fname[0])) {
    fname = ":fd:";
  }

  if (!fname || !fname[0]) {
    return 0;
  }

  /* Don't need 0, because 1 byte already allocated in the
   * istream68_fd_t::fname.
   */
  len = strlen(fname);
  isf = alloc68(sizeof(istream68_fd_t) + len);
  if (!isf) {
    return 0;
  }

  /* Copy istream functions. */
  memcpy(&isf->istream, &istream68_fd, sizeof(istream68_fd));
  /* Clean init. */
  isf->fd     = -1;
  isf->org_fd = fd;
  isf->mode   = mode & (ISTREAM68_OPEN_READ|ISTREAM68_OPEN_WRITE);

  /* Copy filename. */
  /* $$$ May be later, we should add a check for relative path and add
   * CWD ... or make it URL compatible */
  strcpy(isf->name, fname);
  return &isf->istream;
}

#else /* #ifndef ISTREAM68_NO_FILE */

/* istream fd must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

istream68_t * istream68_fd_create(const char * fname, int fd, int mode)
{
  msg68_error("fd68: create -- *NOT SUPPORTED*");
  return 0;
}

#endif /* #ifndef ISTREAM68_NO_FILE */