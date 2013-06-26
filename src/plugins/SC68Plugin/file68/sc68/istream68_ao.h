/**
 * @ingroup  file68_lib
 * @file     sc68/istream68_ao.h
 * @author   Benjamin Gerard
 * @date     2007-03-08
 * @brief    AO stream header.
 *
 */

/* $Id: istream68_ao.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ISTREAM68_AO_H_
#define _FILE68_ISTREAM68_AO_H_

#include "istream68.h"

/** @name     AO stream
 *  @ingroup  file68_istream
 *
 *    Implements istream68_t for XIPH libao (audio output).
 *
 *  @{
 */

FILE68_EXTERN
/** Creates a stream for libao.
 *
 *  @param  fname    path of file.
 *  @param  mode     bit#0 : read access, bit#1 : write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note    filename is internally copied.
 *  @warning write mode only.
 */
istream68_t * istream68_ao_create(const char * fname, int mode);

/**
 *  @}
 */

#endif /* #define _FILE68_ISTREAM68_AO_H_ */
