/**
 * @ingroup   emu68_lib
 * @file      emu68/lines68.h
 * @brief     68k instructions header.
 * @date      2009/05/08
 * @author    Benjamin Gerard
 *
 *   The lines68.h header file should include all files required for
 *   compiling instruction emulator files.
 *
 */

/* $Id$ */

#ifndef _EMU68_LINES68_H_
#define _EMU68_LINES68_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "assert68.h"
#include "excep68.h"
#include "emu68.h"
#include "inst68.h"
#include "macro68.h"

#include "inl68_exception.h"
#include "inl68_arithmetic.h"
#include "inl68_bcd.h"
#include "inl68_bitmanip.h"
#include "inl68_datamove.h"
#include "inl68_logic.h"
#include "inl68_progctrl.h"
#include "inl68_shifting.h"
#include "inl68_systctrl.h"

#endif /* #ifndef _EMU68_LINES68_H_ */
