/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_exception.h
 * @brief     68k exception header.
 * @date      2009/05/08
 * @author    Benjamin Gerard
 *
 */

/* $Id$ */

#ifndef _EMU68_INL_EXCEPTION68_H_
#define _EMU68_INL_EXCEPTION68_H_

EMU68_EXTERN
void exception68(emu68_t * const emu68, const int vector, const int level);

static inline
void inl_exception68(emu68_t * const emu68, const int vector, const int level)
{
  exception68(emu68, vector, level);
}

static inline
void inl_buserror68(emu68_t * const emu68, int addr, int mode)
{
  inl_exception68(emu68, BUSERR_VECTOR, -1);
}

static inline
void inl_linea68(emu68_t * const emu68)
{
  inl_exception68(emu68, LINEA_VECTOR, -1);
}

static inline
void inl_linef68(emu68_t * const emu68)
{
  inl_exception68(emu68, LINEF_VECTOR, -1);
}

#endif /* #ifndef _EMU68_INL_EXCEPTION68_H_ */
