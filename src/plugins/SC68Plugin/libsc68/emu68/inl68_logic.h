/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_logic.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k logical instruction inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_LOGIC_H_
#define _INL68_LOGIC_H_

#include "inl68_progctrl.h"             /* for inl_tst68() */

static inline
int68_t inl_and68(emu68_t * const emu68, const int68_t s, int68_t d)
{
  d &= s;
  inl_tst68(emu68, d);
  return d;
}

static inline
int68_t inl_orr68(emu68_t * const emu68, const int68_t s, int68_t d)
{
  d |= s;
  inl_tst68(emu68, d);
  return d;
}

static inline
int68_t inl_eor68(emu68_t * const emu68, const int68_t s, int68_t d)
{
  d ^= s;
  inl_tst68(emu68, d);
  return d;
}

static inline
int68_t inl_not68(emu68_t * const emu68, int68_t d)
{
  d = ~d;
  inl_tst68(emu68, d);
  return d;
}

#endif /* #ifndef _INL68_LOGIC_H_ */
