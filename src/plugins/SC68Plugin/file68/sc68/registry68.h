/**
 * @ingroup  file68_lib
 * @file     sc68/registry68.h
 * @author   Benjamin Gerard
 * @date     2003-08-11
 * @brief    Windows registry header.
 *
 */

/* $Id: registry68.h 126 2009-07-15 08:58:51Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_REGISTRY68_H_
#define _FILE68_REGISTRY68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif

/** @defgroup  file68_registry  Registry access
 *  @ingroup   file68_lib
 *
 *    Provides Windows registry access functions.
 *
 *  @{
 */

/** Enumerate registry key type. */
/*  *** DO NOT CHANGE ORDER ***  */
enum registry68_key_e {
  REGISTRY68_INK = -1,                  /**< INvalid Key.          */
  REGISTRY68_CRK = 0,                   /**< Classes Root Key.     */
  REGISTRY68_CUK,                       /**< Current User Key.     */
  REGISTRY68_LMK,                       /**< Local Machine Key.    */
  REGISTRY68_USK,                       /**< USers Key.            */
  REGISTRY68_PDK,                       /**< Performance Data Key. */
  REGISTRY68_CCK,                       /**< Current Config Key.   */
  REGISTRY68_DDK,                       /**< Dynamic Data Key.     */

  REGISTRY68_LST                        /**< Last element.         */
};

/** Registry key type (override Microsoft HKEY type) */
typedef void * registry68_key_t;

/* Last error message. */
/* extern char registry68_errorstr[]; */

FILE68_API
/** Check for registry support.
 *
 *  @retval  0  registry not supported
 *  @retval  1  registry supported
 */
int registry68_support(void);


FILE68_API
/** Get key handler for a registry root type.
 *
 *  @param  rootkey  One of the M$ registry key-type/root-key/entry-point.
 *  @return key handler
 *  @retval  0  error (key invalid)
 */
registry68_key_t registry68_rootkey(enum registry68_key_e rootkey);

FILE68_API
/** Open a named hierarchic key.
 *
 *  @param hkey     Opened key handle or one of reserved registry key handles.
 *  @param kname    Hierarchic key name. Slash '/' caractere is interpreted
 *                  as sub-key separator.
 *
 *  @return Registry key handle
 *  @retval registry68InvalidKey Error
 */
registry68_key_t registry68_open(registry68_key_t hkey, const char *kname);

FILE68_API
/** Get value of a named hierarchic string key.
 *
 *  @param hkey     Opened key handle or one of reserved registry key handles.
 *  @param kname    Hierarchic key name. Slash '/' caractere is interpreted
 *                  as sub-key separator.
 *  @param kdata    Returned string storage location
 *  @param kdatasz  Maximum size of kdata buffer.
 *
 *  @return ErrorNo
 *  @retval 0  Success
 *  @retval <0 Error
 */
int registry68_gets(registry68_key_t hkey,
                    const char *kname,
                    char *kdata,
                    int kdatasz);

/**
 *  @}
 */

#endif /* #ifndef _FILE68_REGISTRY68_H_ */
