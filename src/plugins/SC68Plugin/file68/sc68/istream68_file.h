/**
 * @ingroup  file68_lib
 * @file     sc68/istream68_file.h
 * @author   Benjamin Gerard
 * @date     2007/08/08
 * @brief    FILE stream header.
 *
 */

/* $Id: istream68_file.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_FILE_H_
#define _FILE68_ISTREAM68_FILE_H_

#include "istream68.h"


/** @name     FILE stream
 *  @ingroup  file68_istream
 *
 *    Implements istream68_t for stdio.h FILE.
 *
 *  @{
 */

FILE68_EXTERN
/** Creates a stream for "C" FILE.
 *
 *  @param  fname    path of file.
 *  @param  mode     bit#0 : read access, bit#1 : write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is internally copied.
 */
istream68_t * istream68_file_create(const char * fname, int mode);

/**
 *  @}
 */

#endif /* #define _FILE68_ISTREAM68_FILE_H_ */
