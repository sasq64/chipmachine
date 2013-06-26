/**
 * @ingroup   io68_lib
 * @file      io68/mw_io.h
 * @author    Benjamin Gerard
 * @date      1999/03/20
 * @brief     STE sound IO plugin header.
 *
 */

/* $Id: mw_io.h 126 2009-07-15 08:58:51Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _IO68_MW_IO_H_
#define _IO68_MW_IO_H_

#include "io68_api.h"
#include "mwemul.h"

/** @addtogroup  io68_lib_mw
 *  @{
 */

/** @name Microwire/LMC (STE sound) IO plugin.
 *  @{
 */

IO68_EXTERN
/** Init the mwio library.
 *
 *    The mwio_init() function setup the microwire IO plugin and the
 *    microwire emulator library by calling the mw_init() function.
 *
 *  @param argc  pointer to argument count
 *  @param argv  arguemnt array
 *  @return      error status
 *  @retval   0  on success
 *  @retval  -1  on success
 */
int mwio_init(int * argc, char ** argv);

IO68_EXTERN
/** Shutdown mwio library. */
void mwio_shutdown(void);

IO68_EXTERN
/** Create a microwire plugin instance. */
io68_t * mwio_create(emu68_t * emu68, mw_parms_t * const parms);

IO68_EXTERN
/** Get/Set sampling rate.
 *
 * @param  io  MW IO instance
 * @param  hz  @ref mw_hz_e "sampling rate"
 *
 * @return current @ref mw_hz_e "sampling rate"
 *
 */
int mwio_sampling_rate(io68_t * const io, int hz);

IO68_EXTERN
/** Get/Set emulator engine.
 *
 * @param  io      MW IO instance
 * @param  engine  @ref mw_engine_e "MW engine descriptor"
 *
 * @return @ref mw_engine_e "MW engine descriptor"
 */
uint68_t mwio_engine(io68_t * const io, int);

IO68_EXTERN
/** Get microwire emulator instance attached to the mwio plugin. */
mw_t * mwio_emulator(io68_t * const io);

/** @} */

/**
 *  @}
 */

#endif /* #ifndef _IO68_MW_IO_H_ */
