/**
 * @ingroup  file68_lib
 * @file     sc68/file68_api.h
 * @author   Benjamin Gerard
 * @date     2007-02-25
 * @brief    Symbol exportation header.
 *
 */

/* $Id: file68_api.h 124 2009-07-02 18:51:52Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_FILE68_API_H_
#define _FILE68_FILE68_API_H_

/**
 *  @defgroup file68_lib file68 library
 *  @ingroup  api68
 *
 *  file68 is a library to manipulate sc68 files and access sc68
 *  resources and much more.
 *
 *  @{
 */

#ifndef FILE68_EXTERN
# ifdef __cplusplus
/** Decorate symbols exported locally. */
#   define FILE68_EXTERN extern "C"
# else
#   define FILE68_EXTERN extern
# endif
#endif

#ifndef FILE68_API
/* Building */
# ifdef FILE68_EXPORT
#  if defined(DLL_EXPORT) && defined(HAVE_DECLSPEC)
#   define FILE68_API __declspec(dllexport)
#  elif defined(HAVE_VISIBILITY)
#   define FILE68_API FILE68_EXTERN __attribute__ ((visibility("default")))
#  else
#   define FILE68_API FILE68_EXTERN
#  endif
/* Using */
# else
#  if defined(FILE68_DLL)
#   define FILE68_API __declspec(dllimport)
#  else
#   define FILE68_API FILE68_EXTERN
#  endif
# endif
#endif

/** Decorate symbols exported for public. */
#ifndef FILE68_API
#define FILE68_API FILE68_EXTERN
#error "FILE68_API should be defined"
#endif

/**
 *  @}
 */

#endif /* ifndef _FILE68_FILE68_API_H_ */
