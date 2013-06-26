/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_bitmanip.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k bit manipalution instruction inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_BITMANIP_H_
#define _INL68_BITMANIP_H_

static inline
void inl_btst68(emu68_t * const emu68, const int68_t v, const int bit)
{
  REG68.sr = ( REG68.sr & ~SR_Z )
    | ( ( (~v >> bit) & 1 ) << SR_Z_BIT );
}

static inline
int68_t inl_bset68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v | ( 1 << bit );
}

static inline
int68_t inl_bclr68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v & ~( 1 << bit );
}

static inline
int68_t inl_bchg68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v ^ ( 1 << bit );
}

#endif /* #ifndef _INL68_BITMANIP_H_ */
