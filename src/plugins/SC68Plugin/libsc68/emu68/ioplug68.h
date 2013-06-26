/**
 * @ingroup   emu68_lib
 * @file      emu68/ioplug68.h
 * @date      1999/03/13
 * @brief     68k IO plugger header.
 * @author    Benjamin Gerard
 *
 */

/* Copyright (C) 1998-2011 Benjamin Gerard */

#ifndef _EMU68_IOPLUG68_H_
#define _EMU68_IOPLUG68_H_

#include "emu68_api.h"
#include "struct68.h"

/**
 * @defgroup  emu68_lib_ioplug  68k IO plugger
 * @ingroup   emu68_lib_api
 * @brief     IO plugging and mapping facilities.
 *
 *   Provide functions for warm plugging/unplugging of IO
 *   chipset. Limitations are explained in @ref emu68_mem68_devel "68k
 *   memory and IO manager" detailed description.
 *
 * @{
 */

EMU68_API
/** Unplug all IO.
 *
 *    Process emu68_ioplug_unplug() function for all pluged IO.
 *
 */
void emu68_ioplug_unplug_all(emu68_t * const emu68);

EMU68_API
/** Unplug and destroy all IO.
 *
 *    Process emu68_ioplug_unplug() function for all pluged IO and
 *    destroy each io by calling its io68_t::destroy function.
 *
 */
void emu68_ioplug_destroy_all(emu68_t * const emu68);

EMU68_API
/** Unplug an IO.
 *
 *    The emu68_ioplug_unplug() function removes an IO from pluged IO
 *    list and reset memory access handler for its area.
 *
 *  @param   io  Address of IO structure to unplug.
 *
 *  @return   error-code
 *  @retval   0   Success
 *  @retval   <0  Error (probably no matching IO)
 */
int emu68_ioplug_unplug(emu68_t * const emu68, io68_t * const io);

EMU68_API
/** Plug an IO.
 *
 *    The emu68_ioplug() function add an IO to pluged IO list and add
 *    suitable memory access handlers.
 *
 *  @param  io  Address of IO structure to plug.
 */
void emu68_ioplug(emu68_t * const emu68, io68_t * const io);

/**
 *  @}
 */

#endif /* #ifndef _EMU68_IOPLUG68_H_ */