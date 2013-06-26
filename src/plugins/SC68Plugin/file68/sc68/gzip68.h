/**
 * @ingroup  file68_lib
 * @file     sc68/gzip68.h
 * @author   Benjamin Gerard
 * @date     2003-09-03
 * @brief    Gzip loader header.
 *
 */

/* $Id: gzip68.h 124 2009-07-02 18:51:52Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_GZIP68_H_
#define _FILE68_GZIP68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif

/** @defgroup  file68_gzip  gzip file support.
 *  @ingroup   file68_lib
 *
 *    Provides functions for loading and saving gzip file.
 *
 *  @{
 */

FILE68_EXTERN
/** Test gzip file header magic header.
 *
 *  @param  buffer  Buffer containing at least 3 bytes from gzip header.
 *
 *  @retval  1  buffer may be gzipped.
 *  @retval  0  buffer is not gzipped.
 */
int gzip68_is_magic(const void * buffer);

FILE68_EXTERN
/** Load an optionnally gzipped file.
 *
 *    The gzip68_load() function allocates memory and loads the totality of the
 *    given file. If the file is a gzipped file, it will be inflate.
 *
 * @param  fname    Name of file to load.
 * @param  ulen     Pointer to uncompressed or total size of file.
 *
 * @return A pointer to loaded file buffer.
 * @retval 0 on error.
 */
void * gzip68_load(const char * fname, int * ulen);

/**
 *  @}
 */

#endif /* #ifndef _FILE68_GZIP68_H_ */
