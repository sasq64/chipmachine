/**
 * @ingroup   emu68_lib
 * @file      emu68/assert68.h
 * @brief     assert feature header.
 * @date      2009/06/12
 * @author    Benjamin Gerard
 *
 */

/* $Id$ */

#ifndef _EMU68_ASSERT68_H_
#define _EMU68_ASSERT68_H_

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifndef assert
# error "assert macro must be defined"
#endif
#if defined(NDEBUG_LIBSC68) && !defined(NDEBUG)
# error "Compile libsc68 in release mode with assert enable"
#endif

#ifndef EMU68_BREAK
# ifndef DEBUG
#  define EMU68_BREAK 1                 /* NOT debug mode */
# else 
#  define EMU68_BREAK 0                 /*     debug mode */
# endif
#endif

#endif /* #ifndef _EMU68_ASSERT68_H_ */
