/**
 * @ingroup  file68_lib
 * @file     sc68/istream68_null.h
 * @author   Benjamin Gerard
 * @date     2003-10-10
 * @brief    Null stream header.
 *
 */

/* $Id: istream68_null.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_NULL_H_
#define _FILE68_ISTREAM68_NULL_H_

#include "istream68.h"

/** @name     Null stream
 *  @ingroup  file68_istream
 *
 *    Implements a null istream68_t.
 *
 *    Null stream does nothing but checking some trivial errors (like
 *    access without opening) and dealing with a virtual stream position.
 *    The null stream length is the position the highest byte that
 *    has been either read or write. The length is resetted at open.
 *    It implies that stream length can be retrieved by the istream68_length()
 *    function after istream68_close() call.
 *
 *  @{
 */

FILE68_EXTERN
/** Creates a null stream.
 *
 *  @param  name     Optionnal name *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is prefixed by "null://".
 */
istream68_t * istream68_null_create(const char * name);

/** @} */


#endif /* #define _FILE68_ISTREAM68_NULL_H_ */
