/**
 * @ingroup   emu68_lib
 * @file      emu68/excep68.h
 * @author    Benjamin Gerard
 * @date      1999/13/03
 * @brief     68k exception and interruption definition header.
 *
 */

/* $Id: excep68.h 141 2011-08-07 23:30:20Z benjihan $ */

/* Copyright (C) 1998-2010 Benjamin Gerard */

#ifndef _EMU68_EXCEP68_H_
#define _EMU68_EXCEP68_H_

/** @addtogroup  emu68_lib
 *  @{
 */

/**
 *   @name  68k exceptions and interruptions
 *
 *   68K interruptions are defined by a vector and a level. The
 *   interrupt vector is a long word stored in memory at vector
 *   address. This long word is the location of the interrupt routine
 *   which is loaded in the PC register. The interrupt level is the
 *   value transfered to the IPL field of SR so that no lower leveled
 *   interruption may be triggered.
 *
 * @{
 */

enum {
  HWBREAK_VECTOR  = 0x100, /**< Report Hardware Breakpoint.             */
  HWTRACE_VECTOR  = 0x101, /**< Report Hardware TRACE.                  */
  HWHALT_VECTOR   = 0x102, /**< Report processor halted.                */

  RESET_VECTOR    = 0x000, /**< External (hardware) Reset.              */
  RESET_SP_VECTOR = 0x000, /**< Initial Interrupt Stack Pointer.        */
  RESET_PC_VECTOR = 0x001, /**< Initial Program Counter.                */
  BUSERR_VECTOR   = 0x002, /**< Access fault vector address.            */
  ADRERR_VECTOR   = 0x003, /**< Access error vector address.            */
  ILLEGAL_VECTOR  = 0x004, /**< Illegal instruction vector address.     */
  DIVIDE_VECTOR   = 0x005, /**< Integer divide by zero.                 */
  CHK_VECTOR      = 0x006, /**< Chk/Chk2 instructions vector address.   */
  TRAPV_VECTOR    = 0x007, /**< F/TRAP/V/cc istructions vector address. */
  PRIVV_VECTOR    = 0x008, /**< Privilege Violation vector address.     */
  TRACE_VECTOR    = 0x009, /**< TRACE vector address.                   */
  LINEA_VECTOR    = 0x00A, /**< LINEA vector address.                   */
  LINEF_VECTOR    = 0x00B, /**< LINEF vector address.                   */
  SPURIOUS_VECTOR = 0x018, /**< Spurious Interrupt vector address.      */
  TRAP_VECTOR_0   = 0x020, /**< TRAP #N vector address.                 */
};

/** Nth TRAP vector address. */
#define TRAP_VECTOR(N)   ( ( (N) & 15 ) + TRAP_VECTOR_0 )

/** Nth interrupt auto vector address. */
#define AUTO_VECTOR(N)   ( ( (N) & 07 ) + SPURIOUS_VECTOR )

/** @} */

/**
 *  @}
 */

#endif /* #ifndef _EMU68_EXCEP68_H_ */
