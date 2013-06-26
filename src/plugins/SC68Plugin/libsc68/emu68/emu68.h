/**
 * @ingroup   emu68_lib
 * @file      emu68/emu68.h
 * @author    Benjamin Gerard
 * @date      1999/03/13
 * @brief     68K emulator header.
 *
 */

/* $Id: emu68.h 143 2011-08-10 01:05:27Z benjihan $ */

/* Copyright (C) 1998-2010 Benjamin Gerard */

#ifndef _EMU68_EMU68_H_
#define _EMU68_EMU68_H_

#include "emu68_api.h"

#include "struct68.h"
#include "error68.h"
#include "cc68.h"
#include "srdef68.h"
#include "mem68.h"

/**
 *  @defgroup  emu68_lib  68k emulator library
 *  @ingroup   sc68_lib
 *  @brief     The 68k emulator library.
 */

/**
 *  @defgroup  emu68_lib_core  68k emulator core
 *  @ingroup   emu68_lib
 *  @brief     The core of the 68k emulator.
 */

/**
 *  @addtogroup  emu68_lib
 *  @{
 */

/**
 *  @name  Library init functions
 *  @{
 */

EMU68_API
/**
 *  Init EMU68 library.
 *
 *    The emu68_init() function initializes the EMU68 library. It
 *    should be call once before any other emu68 call. The function
 *    will perform internal setup. EMU68 behaviour can be adjust by
 *    passing arguments in the argv[] array. Parsed arguments are
 *    removed and the remaining number of argument is returned.
 *
 *  @retval  -1     on error
 *  @retval   0     on success
 *
 *  @see emu68_shutdown()
 */
int emu68_init(int * argc, char ** argv);

EMU68_API
/**
 *   Shutdown 68K emulator.
 *
 *     The emu68_shutdown() function shutdown the EM68 library. It
 *     must be call at the end and further more calls are forbidden
 *     except for emu68_init(). All emulator instances created should
 *     have been killed before.
 *
 *  @see emu68_init()
 */
void emu68_shutdown(void);

/** @} */


/** @name EMU68 instance functions
 *  @{
 */

/** Atari ST clock (as written on cristal clock chip). */
#define EMU68_ATARIST_CLOCK (8010613u&~3u)

/**
 *  68k emulator instance creation parameters. This structure have to
 *  be filled before calling the emu68_create() function to customize
 *  the instance. Members set to zero are replaced by default values.
 */
typedef struct {
  const char * name;  /**< Identifier name.                         */
  int log2mem;        /**< Memory amount (value of the power of 2). */
  int clock;          /**< CPU clock frequency (in hz).             */
  int debug;          /**< Run in debug mode (0:off).               */
} emu68_parms_t;

EMU68_API
/**
 *  Create a 68k emulator instance.
 *
 *    The emu68_create() function creates an instance of the 68k
 *    emulator.  The logmem parameter is the size of the 68K memory
 *    expressed in power of 2. Valid values are in the range 17 to 24
 *    (inclusive) respectively 128 Kb to 16 Mb. Members set to zero
 *    will be replaced by the default value; alternatively if null
 *    pointer is passed the whole default parameters is applied (512KB
 *    at 8Mhz).
 *
 *  @param   parms  Creation parameters or null pointer.
 *  @return         emu68 instance
 *  @retval  0      on error
 */
emu68_t * emu68_create(emu68_parms_t * const parms);

EMU68_API
/**
 *  Duplicate a 68k emulator instance.
 *
 *    The emu68_dup() function creates an new instance of the 68k
 *    emulator which is a duplicate of the given emu68 instance.
 *
 *  @param  emu68  emulator instance to duplicate
 *  @param  name   duplicate emulator name [0:auto]
 *
 *  @return        duplicated emu68 instance
 *  @retval  0     on error
 *
 *  @todo Duplicate attached IO is currently not supported since IO
 *        modules do not have a dup() function.
 */
emu68_t * emu68_duplicate(emu68_t * emu68, const char * name);

EMU68_API
/**
 *  Destroy a 68k emulator instance.
 *  @param  emu68  emulator instance
 */
void emu68_destroy(emu68_t * const emu68);

EMU68_API
/**
 *  Hardware Reset.
 *
 *    Perform following operations:
 *    - PC = 0
 *    - SR = 2700
 *    - A7 = end of mem - 4
 *    - All registers cleared
 *    - All IO reseted
 *
 *  @param  emu68  emulator instance
 */
void emu68_reset(emu68_t * const emu68);

/** @} */


/*  /\  ============================================================  /\  */
/* <  > ============================================================ <  > */
/*  \/  ============================================================  \/  */


/**
 *  @name  Exception and Interruption control functions.
 *
 *     EMU68 has a very limited interrupt handler. In fact only one
 *     source of interruption is used which is enought for sc68
 *     needs. The emu68_set_interrupt_io() function selects the given
 *     IO chip as the candidate to interruption.
 *
 *     Exception can be trapped and notified by calling specified
 *     handler function.
 *
 *  @{
 */

EMU68_API
/**
 *  Set new interrupt IO.
 *
 *  @param  emu68  emulator instance
 *  @param  io     pointer to the only io that could possibly interrupt
 *  @return        pointer to previous interrupt IO
 */
io68_t * emu68_set_interrupt_io(emu68_t * const emu68, io68_t * io);

EMU68_API
/**
 *  Set user-data pointer.
 *
 *  @param  emu68   emulator instance
 *  @param  cookie  user-data pointer
 *  @return previous user-data pointer
 */
void * emu68_set_cookie(emu68_t * const emu68, void * cookie);

EMU68_API
/**
 *  Get user-data pointer.
 *
 *  @param  emu68   emulator instance
 *  @return  current user-data pointer
 */
void * emu68_get_cookie(emu68_t * const emu68);

EMU68_API
/**
 *  Set user handler.
 *
 *  @param  emu68  emulator instance
 *  @param  hdl    user handler (0:do not update)
 *  @return previous handler
 */
emu68_handler_t emu68_set_handler(emu68_t * const emu68, emu68_handler_t hdl);

EMU68_API
/**
 *  Get exception name.
 *
 *  @param  vector  Eception vector number
 *  @return exception name
 *  @retval 0  unknown exception (not neccessary invalid)
 */
const char * emu68_exception_name(unsigned int vector);

/** @} */


/**
 *  @name  Cycle counter access functions.
 *  @{
 */

EMU68_API
/**
 *  Set internal cycle counter.
 *
 *  @param  emu68  emulator instance
 */
void emu68_set_cycle(emu68_t * const emu68, cycle68_t cycle);

EMU68_API
/**
 *  Get internal cycle counter.
 *
 *  @param  emu68  emulator instance
 */
cycle68_t emu68_get_cycle(emu68_t * const emu68);

/** @} */


/*  /\  ============================================================  /\  */
/* <  > ============================================================ <  > */
/*  \/  ============================================================  \/  */


/** @name  EMU68 on-board memory access
 *  @{
 */

EMU68_API
/**
 *  Check if a memory block is in 68K on-board memory range.
 *
 *  @param  emu68  emulator instance
 *
 *  @return  Pointer to onboard memory block
 *  @retval  0  Failure
 */
u8 * emu68_memptr(emu68_t * const emu68, addr68_t dest, uint68_t sz);

EMU68_API
/**
 *  Check for a memory access status block.
 *
 *  @param  emu68  emulator instance
 *
 *  @return  Pointer to onboard memory block
 *  @retval  0  Failure
 */
u8 * emu68_chkptr(emu68_t * const emu68, addr68_t dst, uint68_t sz);


EMU68_API
/**
 *  Get byte in 68K onboard memory.
 *
 *  @param  emu68  emulator instance
 *  @param  emu68  emulator instance
 *
 *  @see emu68_poke()
 */
int emu68_peek(emu68_t * const emu68, addr68_t addr);

EMU68_API
/**
 *  Get byte in 68K access control memory.
 *
 *  @param  emu68  emulator instance
 *  @param  emu68  emulator instance
 *
 *  @see emu68_poke()
 */
int emu68_chkpeek(emu68_t * const emu68, addr68_t addr);


EMU68_API
/**
 *  Put a byte in 68K onboard memory.
 *
 *  @param  emu68  emulator instance
 *
 *  @see emu68_peek()
 */
int emu68_poke(emu68_t * const emu68, addr68_t addr, int68_t v);

EMU68_API
/**
 *  Put a byte in 68K access control memory.
 *
 *  @param  emu68  emulator instance
 *
 *  @see emu68_peek()
 */
int emu68_chkpoke(emu68_t * const emu68, addr68_t addr, int68_t v);

EMU68_API
/**
 *  Put a memory block to 68K on-board memory.
 *
 *    The function copy a memory block in 68K on-board memory after verifying
 *    that the operation access valid 68K memory.
 *
 *  @param  emu68  emulator instance
 *
 *  @see emu68_memget()
 *  @see emu68_memvalid()
 */
int emu68_memput(emu68_t * const emu68,
                 addr68_t dst, u8 * src, uint68_t sz);

EMU68_API
/**
 *  Get 68K on-board memory into a memory block.
 *
 *    The function copy a 68K on-board memory to a memory location after
 *    verifying that the operation access valid 68K memory.
 *
 *  @param  emu68  emulator instance
 *
 *  @see emu68_memput()
 *  @see emu68_memvalid()
 */
int emu68_memget(emu68_t * const emu68,
                 u8 * dst, addr68_t src, uint68_t sz);

EMU68_API
/**
 *  Fill a 68k on board memory block with a value.
 *  @param  emu68  emulator instance
 */
int emu68_memset(emu68_t * const emu68, addr68_t dst, u8 val, uint68_t sz);

EMU68_API
/**
 *  Fill a 68k access control memory block with a value.
 *  @param  emu68  emulator instance
 */
int emu68_chkset(emu68_t * const emu68, addr68_t dst, u8 val, uint68_t sz);

EMU68_API
/**
 *  Push 32-bit long word.
 *  @param  emu68  emulator instance
 *  @param  val    value to push.
 */
void emu68_pushl(emu68_t * const emu68, int68_t val);

EMU68_API
/**
 *  Push 16-bit word.
 *  @param  emu68  emulator instance
 *  @param  val    value to push.
 */
void emu68_pushw(emu68_t * const emu68, int68_t val);

EMU68_API
/**
 *  Pop 32-bit long word.
 *  @param  emu68  emulator instance
 *  @return poped 32-bit value
 */
int68_t emu68_popl(emu68_t * emu68);

EMU68_API
/**
 *  Pop 16-bit word.
 *  @param  emu68  emulator instance
 *  @return poped 16-bit value
 */
int68_t emu68_popw(emu68_t * emu68);

EMU68_API
/**
 *  Compute CRC32 of emu68 object (registers + memory).
 *  @param  emu68   emulator instance
 *  @return crc32
 */
uint68_t emu68_crc32(emu68_t * const emu68);

/** @} */


/**
 *  @name  Execution functions
 *  @{
 *
 *  @todo Describe execution loop here ...
 *
 */

/**
 *  Execution status code.
 *
 *    The emu68_status_e:: values
 */
enum emu68_status_e {
  EMU68_ERR  = -1,          /**< Execution failed.            */
  EMU68_NRM  =  0,          /**< Execution finished.          */
  EMU68_STP  =  1,          /**< Execution stopped.           */
  EMU68_BRK  =  2,          /**< Execution breaked.           */
  EMU68_XCT  =  3           /**< Execution in exception.      */
};

EMU68_API
/**
 *  Get status name.
 *
 *   @param  status  one of the emu68_status_e value.
 *   @return status name
 */
const char * emu68_status_name(enum emu68_status_e status);

EMU68_API
/**
 *  Execute one instruction.
 *
 *   @param  emu68  emulator instance
 *   @return @ref emu68_status_e "execution status"
 */
int emu68_step(emu68_t * const emu68);

EMU68_API
/**
 *  Execute until RTS (Return To Subroutine).
 *
 *   @param  emu68  emulator instance
 *   @return @ref emu68_status_e "execution status"
 */
int emu68_finish(emu68_t * const emu68, cycle68_t cycles);

EMU68_API
/**
 *  Continue a breaked execution.
 *
 *   @param  emu68  emulator instance
 *   @return @ref emu68_status_e "execution status"
 */
int emu68_continue(emu68_t * const emu68);


EMU68_API
/**
 *  Execute interruptions with given cycle interval.
 *
 *   @param  emu68   emulator instance
 *   @param  cycles  interval within to excute interruptions
 *   @return @ref emu68_rc_e "execution return code"
 */
int emu68_interrupt(emu68_t * const emu68, cycle68_t cycles);

/** @} */

/**
 * @name  Breakpoint functions.
 *  @{
 */

/**
 * Kill all existing breakpoints.
 */
void emu68_bp_delall(emu68_t * const emu68);

/**
 *  Delete a existing breakpoint.
 *
 *  @param  emu68  emulator instance
 *  @param  id      breakpoint identifier
 */
void emu68_bp_del(emu68_t * const emu68, int id);

/**
 *  Set/Create a breakpoint.
 *
 *  @param  emu68  emulator instance
 *  @param  id      breakpoint identifier (-1:find free breakpooint)
 *  @param  addr    breakpoint address
 *  @param  count   breakpoint countdown
 *  @param  reset   breakpoint countdown reset (0:remove after break)
 *  @return breakpoint identifier
 *  @retval -1 on error
 */
int emu68_bp_set(emu68_t * const emu68, int id,
                 addr68_t addr, uint68_t count, uint68_t reset);

/**
 *  Find breakpoint.
 *
 *  @param  emu68  emulator instance
 *  @param  addr   breakpoint address
 *  @return breakpoint identifier
 *  @retval -1 on error
 */
int emu68_bp_find(emu68_t * const emu68, addr68_t addr);

/** @} */

/** @name  Version checking functions
 *  @{
 */

EMU68_API
/**
 *  Get debug mode.
 *
 *  @param  emu68  emulator instance
 *
 *  @retval  0  normal mode
 *  @retval  1  debug mode
 *  @retval -1  error
 */
int emu68_debugmode(emu68_t * const emu68);

/** @} */


/**
 *  @}
 */

#endif /* #ifndef _EMU68_EMU68_H_ */
