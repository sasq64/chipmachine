/**
 * @ingroup  file68_lib
 * @file     sc68/error68.h
 * @author   Benjamin Gerard
 * @date     2003-08-08
 * @brief    Error message header.
 *
 */

/* $Id: error68.h 126 2009-07-15 08:58:51Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_ERROR68_H_
#define _FILE68_ERROR68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif
#include <stdarg.h>


/** @defgroup  file68_error  Error message
 *  @ingroup   file68_lib
 *
 *    Provides error message facilities.
 *
 *  @{
 */

FILE68_API
/** Print a formatted error message.
 *
 *    The error68() function prints an error message via
 *    the msg68_error() function.
 *
 *  @param  format   printf() like format string
 *
 *  @return error-code
 *  @retval -1
 */
int error68(const char * format, ... );

FILE68_API
int error68x(void * cookie, const char * format, ... );

FILE68_API
/** Print a formatted error message (variable argument version).
 *
 *  @see error68()
 */
int error68_va(const char * format, va_list list);

FILE68_API
int error68x_va(void * cookie, const char * format, va_list list);

/**
 *  @}
 */

#endif /* #ifndef _FILE68_ERROR68_H_ */
