/**
 * @ingroup   emu68_lib
 * @file      emu68/emu68_api.h
 * @author    Benjamin Gerard
 * @date      2009/02/10
 * @brief     emu68 library export header.
 *
 */

/* $Id: emu68.h 91 2009-02-09 17:25:51Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _EMU68_EMU68_API_H_
#define _EMU68_EMU68_API_H_

#ifndef EMU68_API
# ifdef EMU68_EXPORT
#  include "sc68/sc68.h"
#  define EMU68_EXTERN SC68_EXTERN
#  define EMU68_API    SC68_API
# elif defined (__cplusplus)
#  define EMU68_EXTERN extern "C"
#  define EMU68_API    EMU68_EXTERN
# else
#  define EMU68_EXTERN extern
#  define EMU68_API    EMU68_EXTERN
# endif
#endif

#endif /* #ifndef _EMU68_EMU68_API_H_ */
