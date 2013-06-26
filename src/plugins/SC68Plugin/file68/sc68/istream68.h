/**
 * @ingroup  file68_lib
 * @file     sc68/istream68.h
 * @author   Benjamin Gerard
 * @date     2003-08-08
 * @brief    Stream interface header.
 *
 */

/* $Id: istream68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_H_
#define _FILE68_ISTREAM68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif


/** @defgroup  file68_istream  I/O streams.
 *  @ingroup   file68_lib
 *
 *    Provides access functions for generic I/O streams..
 *
 *  @{
 */

/* Stream type. */
typedef struct _istream68_t istream68_t;

/** @name Generic access functions.
 *  @{
 */

FILE68_API
/** Get stream name.
 *
 * @param  istream  stream
 *
 * @return stream name
 * @retval 0 Failure.
 */
const char * istream68_filename(istream68_t *istream);

FILE68_API
/** Open stream.
 *
 * @param  istream  stream
 *
 * @return error code
 * @retval 0   Success
 * @retval -1  Failure
 */
int istream68_open(istream68_t *istream);

FILE68_API
/** Close stream.
 *
 * @param  istream  stream
 *
 * @return error code
 * @retval 0   Success
 * @retval -1  Failure
 */
int istream68_close(istream68_t *istream);

FILE68_API
/** Read data from stream.
 *
 * @param  istream  stream
 * @param  data     destination buffer
 * @param  len      number of byte to read
 *
 * @return number of byte read
 * @retval -1 Failure.
 */
int istream68_read(istream68_t *istream, void * data, int len);

FILE68_API
/** Write data into stream.
 *
 * @param  istream  stream
 * @param  data     destination buffer
 * @param  len      number of byte to read
 *
 * @return number of byte written
 * @retval -1 Failure.
 */
int istream68_write(istream68_t *istream, const void * data, int len);

FILE68_API
/** Get stream length.
 *
 * @param  istream  stream
 *
 * @return number of bytes.
 * @retval -1 Failure.
 */
int istream68_length(istream68_t *istream);

FILE68_API
/** Get stream current position.
 *
 * @param  istream  stream
 *
 * @return stream position
 * @retval -1 Failure.
 */
int istream68_tell(istream68_t *istream);

FILE68_API
/** Set stream relative position.
 *
 * @param  istream  stream
 * @param  offset   displacement from current position
 *
 * @return Absolute position after seeking
 * @retval -1 Failure.
 *
 * @see istream68_seek_to()
 */
int istream68_seek(istream68_t *istream, int offset);

FILE68_API
/** Set stream absolute position.
 *
 * @param  istream  stream
 * @param  pos      position to reach
 *
 * @return Absolute position after seeking
 * @retval -1 Failure.
 *
 * @see istream68_seek()
 */
int istream68_seek_to(istream68_t *istream, int pos);

FILE68_API
/** Close and destroy stream.
 *
 * @param  istream  stream
 *
 */
void istream68_destroy(istream68_t *istream);

FILE68_API
/** Read a '\\0' or '\\n' terminated string.
 *
 * @param  istream  stream
 * @param  buffer   destination buffer
 * @param  max      destination buffer size
 *
 * @return number of char read
 * @retval -1  Failure.
 */
int istream68_gets(istream68_t *istream, char * buffer, int max);

FILE68_API
/** Read next character.
 *
 * @param  istream  stream
 *
 * @return char value [0..255]
 * @retval -1  EOF or error.
 */
int istream68_getc(istream68_t *istream);

FILE68_API
/** Write a '\\0' terminated string.
 *
 * @param  istream  stream
 * @param  s        string
 *
 * @return number of char written
 * @retval -1  Failure.
 */
int istream68_puts(istream68_t *istream, const char * s);

FILE68_API
/** Write a character.
 *
 * @param  istream  stream
 * @param  c        char [0..255]
 *
 * @return error code
 * @retval  0  Success
 * @retval -1  Failure
 */
int istream68_putc(istream68_t *istream, const int c);

/** @} */

/**
 *  @}
 */

#endif /* #ifndef _FILE68_ISTREAM68_H_ */
