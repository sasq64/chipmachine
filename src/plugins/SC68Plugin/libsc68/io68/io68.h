/**
 * @ingroup   io68_lib
 * @file      io68/io68.h
 * @author    Benjamin Gerard
 * @date      1999/03/20
 * @brief     all io68 headers.
 *
 */

/* Copyright (C) 1998-2011 Benjamin Gerard */

#ifndef _IO68_H_
#define _IO68_H_

#include "io68_api.h"

#include "ym_io.h"
#include "mw_io.h"
#include "shifter_io.h"
#include "mfp_io.h"
#include "paula_io.h"

/**
 *  @defgroup  io68_lib  Chipset emulators.
 *  @ingroup   sc68_lib
 *
 *  @{
 */

/** IO chip init parameters. */
/* typedef struct { */
/*   paula_parms_t paula;          /\**< paula init parms.     *\/ */
/*   mw_parms_t    mw;             /\**< microwire init parms. *\/ */
/*   ym_parms_t    ym;             /\**< ym-2149 init parms.   *\/ */
/*   int          *argc;           /\**< Argument count.       *\/ */
/*   char        **argv;           /\**< Argument array.       *\/ */
/* } io68_init_t; */

IO68_API
/** Initialize the io68 library.
 *
 *     The io68_init() function setup chipset emulator engines.
 *
 *  @param   argc  pointer to argument count.
 *  @param   argv  argument array.
 *  @retval  0 on success
 *  @retval -1 on error
 */
int io68_init(int * argc, char ** argv);

IO68_API
/** Shutdown the io68 library.
 *
 *    The io68_shutdown() function cleanup emulator engines. It should
 *    call the corresponding shutdown function for each existing
 *    chipset emulators. It should be call only after all chipset
 *    instances have been released.
 */
void io68_shutdown(void);

IO68_API
/** Destroy an chipset instance.
 *
 *    The io68_destroy() function calls the io68_t::destroy() callback
 *    function.
 *
 *  @param  io  chipset instance
 */
void io68_destroy(io68_t * const io);

/**
 *  @}
 */


#endif /* #ifndef _IO68_H_ */
