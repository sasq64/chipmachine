/**
 * @ingroup   io68_mfp_devel
 * @file      io68/mfp_io.h
 * @author    Benjamin Gerard
 * @date      1999/03/20
 * @brief     MFP-68901 IO plugin header.
 *
 */

/* $Id: mfp_io.h 126 2009-07-15 08:58:51Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _IO68_MFP_IO_H_
#define _IO68_MFP_IO_H_

#include "io68_api.h"
#include "emu68/struct68.h"

/** @addtogroup  io68_mfp_devel
 *  @{
 */

/** @name MFP-68901 (Atari-ST timers) IO plugin
 *  @{
 */

IO68_EXTERN
/** Initialize MFP IO library. */
int mfpio_init(int * argc, char ** argv);

IO68_EXTERN
/** Shutdown MFP IO library. */
void mfpio_shutdown(void);

IO68_EXTERN
/** MFP-68901 IO plugin instance. */
io68_t * mfpio_create(emu68_t * const emu68);

/** @} */

/**
 *  @}
 */

#endif /* #ifndef _IO68_MFP_IO_H_ */
