/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_progctrl.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k program control inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_PROGCTRL_H_
#define _INL68_PROGCTRL_H_

static inline
void inl_tst68(emu68_t * const emu68, const int68_t a)
{
  REG68.sr = ( REG68.sr & ( 0xFF00 | SR_X ) )
    | (  !a << SR_Z_BIT )
    | ( ( a >> ( SIGN_FIX - SR_N_BIT ) ) & SR_N )
    ;
}

static inline
int68_t inl_tas68(emu68_t * const emu68, int68_t d)
{
  inl_tst68(emu68, d << BYTE_FIX);
  return d | 0x80;
}

static inline
void inl_rts68(emu68_t * const emu68)
{
  REG68.pc = popl();
}

static inline
void inl_rte68(emu68_t * const emu68)
{
  REG68.sr = popw();
  REG68.pc = popl();
}

static inline
void inl_rtr68(emu68_t * const emu68)
{
  REG68.sr = ( REG68.sr & 0xFF00 ) | (u8)popw();
  REG68.pc = popl();
}

static inline
void inl_bsr68(emu68_t * const emu68, const addr68_t addr)
{
  pushl(REG68.pc);
  REG68.pc = addr;
  inl_addcycle68(emu68,12);
}

static inline
void inl_jmp68(emu68_t * const emu68, const addr68_t addr)
{
  REG68.pc = addr;
  inl_addcycle68(emu68,4);
}

static inline
void inl_jsr68(emu68_t * const emu68, const addr68_t addr)
{
  pushl(REG68.pc);
  REG68.pc = addr;
  inl_addcycle68(emu68,4);
}

static inline
void inl_nop68(emu68_t * const emu68) { }

#endif /* #ifndef _INL68_PROGCTRL_H_ */
