/**
 * @ingroup  file68_lib
 * @file     sc68/istream68_mem.h
 * @author   Benjamin Gerard
 * @date     2003-08-08
 * @brief    Memory stream header.
 *
 */

/* $Id: istream68_mem.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_MEM_H_
#define _FILE68_ISTREAM68_MEM_H_

#include "istream68.h"


/** @name     Memory stream
 *  @ingroup  file68_istream
 *
 *    Implements istream68_t for memory buffer.
 *
 *  @{
 */

FILE68_EXTERN
/** Creates a stream for memory buffer.
 *
 *  @param  addr     Buffer base address.
 *  @param  len      Buffer length.
 *  @param  mode     Allowed open mode.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is build with memory range.
 */
istream68_t * istream68_mem_create(const void * addr, int len, int mode);

/** @} */


#endif /* #define _FILE68_ISTREAM68_MEM_H_ */
