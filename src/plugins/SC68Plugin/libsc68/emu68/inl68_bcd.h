/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_bcd.h
 * @author    Benjamin Gerard
 * @date      2009/05/18
 * @brief     68k binary coded decimal arithmetic inlines.
 *
 */

/* $Id: inst68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _INL68_BCD_H_
#define _INL68_BCD_H_

static inline
int inl_abcd68(emu68_t * const emu68, int a, int b)
{
  int x = ( emu68->reg.sr >> SR_X_BIT ) & 1;
  a += b + x;                           /* unadjusted result */
  b  = a;                               /* store unadjusted result */
  x  = REG68.sr & (SR_Z);               /* use x as ccr */
  if ( (a&15) > 9 )
    a += 6;
  if ( a > 0x90 ) {
    a += 0x60;
    x |= SR_C|SR_X;
  }
  a = (u8)a;
  if (a) x &= ~SR_Z;
  x |= (a&0x80) >> (7-SR_N_BIT);
  x |= ((~b)&a&0x80) >> (7-SR_V_BIT);
  SET_CCR(REG68.sr,x);

  return a;
}

static inline
int inl_sbcd68(emu68_t * const emu68, int a, int b)
{
  int x = ( emu68->reg.sr >> SR_X_BIT ) & 1;
  int r = a - b - x;
  if ( (b&0xF) + x > (a&15) )
    r -= 6;

  b  = a;                               /* store unadjusted result */
  x  = REG68.sr & (SR_Z);               /* use x as ccr */

  if ( r & 0x80 ) {
    r -= 0x60;
    x |= SR_C|SR_X;
  }    

  a = (u8)r;
  if (a) x &= ~SR_Z;
  x |= (a&0x80) >> (7-SR_N_BIT);
  x |= (b&(~a)&0x80) >> (7-SR_V_BIT);
  
  SET_CCR(REG68.sr,x);

  return a;
}

static inline
int inl_nbcd68(emu68_t * const emu68, int a)
{
  return inl_sbcd68(emu68,0,a);
}

#endif /* #ifndef _INL68_BCD_H_ */
