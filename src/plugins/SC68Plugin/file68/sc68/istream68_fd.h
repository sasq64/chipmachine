/**
 * @ingroup  file68_lib
 * @file     sc68/istream68_fd.h
 * @author   Benjamin Gerard
 * @date     2003-08-08
 * @brief    File descriptor stream header.
 *
 */

/* $Id: istream68_fd.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2007 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_FD_H_
#define _FILE68_ISTREAM68_FD_H_

#include "istream68.h"

/** @name     File descriptor stream
 *  @ingroup  file68_istream
 *
 *    Implements istream68_t for "unix like" file descriptor.
 *
 *  @{
 */

FILE68_EXTERN
/** Creates a stream for "UNIX" file descriptor.
 *
 *  If fd parameters is not -1, it is used to as file descriptor for
 *  the stream and fname is used for naming the stream. Else the file
 *  is open as a regular file with fname as path.
 *
 *  @param  fname  path of file or 0.
 *  @param  fd     file decriptor or -1.
 *  @param  mode   bit-0: read access, bit-1: write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note     filename is internally copied.
 *  @note     Even if fd is given the istream68_open() must be call.
 *  @warning  When opening a stream with an already opened descriptor the
 *            mode should match the real open mode but since no tests are
 *            performed before calling r/w access, it should not failed in
 *            case of wrong access on given mode.
 */
istream68_t * istream68_fd_create(const char * fname, int fd, int mode);

/**
 *  @}
 */

#endif /* #define _FILE68_ISTREAM68_FD_H_ */
