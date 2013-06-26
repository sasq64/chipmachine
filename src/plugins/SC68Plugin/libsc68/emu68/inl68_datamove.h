/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_datamove.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k program control inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_DATAMOVE_H_
#define _INL68_DATAMOVE_H_

static inline
void inl_link68(emu68_t * const emu68, const int reg)
{
  pushl(REG68.a[reg]);
  REG68.a[reg] = REG68.a[7];
  REG68.a[7]   = (u32)(REG68.a[7]+get_nextw());
}

static inline
void inl_unlk68(emu68_t * const emu68, const int reg)
{
  REG68.a[7]   = REG68.a[reg];
  REG68.a[reg] = (u32)popl();
}

static inline
void inl_exg68(emu68_t * const emu68, const int reg0, const int reg9)
{
  s32 * const reg = REG68.d, tmp = reg[reg0];
  reg[reg0] = reg[reg9];
  reg[reg9] = tmp;
}

static inline
addr68_t inl_lea68(emu68_t * const emu68, const  int mode, const int reg)
{
  return get_eal68[mode](emu68,reg);
}

static inline
addr68_t inl_pea68(emu68_t * const emu68, const  int mode, const int reg)
{
  const addr68_t ea = get_eal68[mode](emu68,reg);
  pushl(ea);
  return ea;
}

#endif /* #ifndef _INL68_DATAMOVE_H_ */
