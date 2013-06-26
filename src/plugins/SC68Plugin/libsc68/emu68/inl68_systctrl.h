/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_systctrl.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k system control operation inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_SYSTCTRL_H_
#define _INL68_SYSTCTRL_H_

static inline
void inl_andtosr68(emu68_t * const emu68, int68_t v)
{
  emu68->reg.sr &= v;
}

static inline
void inl_orrtosr68(emu68_t * const emu68, int68_t v)
{
  emu68->reg.sr |= v;
}

static inline
void inl_eortosr68(emu68_t * const emu68, int68_t v)
{
  emu68->reg.sr ^= v;
}

static inline
void inl_reset68(emu68_t * const emu68)
{
  /* $$$ TODO */
}

static inline
void inl_stop68(emu68_t * const emu68)
{
  u16 sr = (u16) get_nextw();
  if ( emu68->reg.sr & SR_S ) {
    emu68->reg.sr = sr;
    emu68->status = EMU68_STP;
  } else {
    exception68(emu68, PRIVV_VECTOR, -1);
  }
}

static inline
void inl_trapv68(emu68_t * const emu68)
{
  if (REG68.sr & SR_V) {
    inl_exception68(emu68, TRAPV_VECTOR, -1);
  }
}

static inline
void inl_trap68(emu68_t * const emu68, const int n)
{
  inl_exception68(emu68,TRAP_VECTOR(n), -1);
}

static inline
void inl_chk68(emu68_t * const emu68, const int68_t a, const int68_t b)
{
  REG68.sr &= 0xFF00 | (SR_X|SR_N);
  REG68.sr |= !b << SR_Z_BIT;
  if ( b < 0 ) {
    REG68.sr |= SR_N;
    inl_exception68(emu68, CHK_VECTOR, -1);
  } else if ( b > a ) {
    REG68.sr &= ~SR_N;
    inl_exception68(emu68, CHK_VECTOR, -1);
  }
}

static inline
void inl_illegal68(emu68_t * const emu68)
{

  inl_exception68(emu68, ILLEGAL_VECTOR, -1);
}

#endif /* #ifndef _INL68_SYSTCTRL_H_ */
