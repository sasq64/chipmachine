#error "don't use anymore"

/*
 * @ingroup   emu68_core_devel
 * @file      emu68/getea68.h
 * @author    Benjamin Gerard
 * @date      1999/03/13
 * @brief     68k effective address calculation function table.
 *
 */

/* $Id: getea68.h 116 2009-06-18 14:09:54Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _EMU68_GETEA68_H_
#define _EMU68_GETEA68_H_

#include "emu68_api.h"
#include "type68.h"

/** @addtogroup  emu68_core_devel
 *
 * @{
 */

/** @name Effective address calculation tables.
 *
 *   The get_ab[bwl] tables are used by EMU68 to calculate operand
 *   effective address. Each of them is indexed by operand addressing
 *   mode. Each entry is a pointer to a function which do everything
 *   neccessary to update processor state (e.g. address register
 *   increment or decrement). reg parameter is register number coded
 *   in the three first bit (0 to 2) of 68k opcode. When the mode is
 *   7, register parameter is used as an index in a second level
 *   function table for extended addressing mode.
 *
 * @{
 */

EMU68_EXTERN
/** Byte operand effective address calculation function table. */
addr68_t (*const get_eab68[8])(emu68_t * const,int reg);

EMU68_EXTERN
/** Word operand effective address calculation function table. */
addr68_t (*const get_eaw68[8])(emu68_t * const,int reg);

EMU68_EXTERN
/** Long operand effective address calculation function table. */
addr68_t (*const get_eal68[8])(emu68_t * const,int reg);


/** @} */

/**
 *  @}
 */

#endif /* #ifndef _EMU68_GETEA68_H_ */
