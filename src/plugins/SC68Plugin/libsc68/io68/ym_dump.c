/*
 *                   sc68 - YM-2149 dump engine
 *            Copyright (C) 2001-2009 Ben(jamin) Gerard
 *           <benjihan -4t- users.sourceforge -d0t- net>
 *
 * This  program is  free  software: you  can  redistribute it  and/or
 * modify  it under the  terms of  the GNU  General Public  License as
 * published by the Free Software  Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 *
 * You should have  received a copy of the  GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* $Id: ym_puls.c 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_CONFIG_OPTION68_H
# include <config_option68.h>
#else
# include "default_option68.h"
#endif


#include "ymemul.h"
#include <sc68/msg68.h>
#include <sc68/string68.h>
#include <sc68/option68.h>

static int reset(ym_t * const ym, const cycle68_t ymcycle)
{
  ym_dump_t * const dump = &ym->emu.dump;
  dump->base_cycle = 0;
  dump->pass       = 0;
  return 0;
}

static ym_waccess_t * advance_list( ym_waccess_t * regs[] )
{
  int j, i;
  ym_waccess_t * w = 0;

  for ( i = 0, j = -1; i < 3; ++i ) {
    if ( ! regs[i] ) continue;
    if ( j < 0 || regs[i]->ymcycle < regs[j]->ymcycle ) {
      j = i;
    }
  }

  if ( j >= 0 ) {
    w = regs[j];
    regs[j] = w->link;
  }

  return w;
}

static
int buffersize(const ym_t * ym, const cycle68_t ymcycles)
{
  return ymcycles * ym->hz / ym->clock;
}

#define CYCLE_DIGITS 10
#define  PASS_DIGITS 6

static
int run(ym_t * const ym, s32 * output, const cycle68_t ymcycles)
{
  static const char hex[16] = {
    '0','1','2','3','4','5','6','7',
    '8','9','A','B','C','D','E','F'
  };

  ym_dump_t * const dump = &ym->emu.dump;
  cycle68_t curcycle;
  u64       longcycle;
  int       i, len, ymreg[16], mute;

  char tmp [128], * buf;
  ym_waccess_t * ptr, * regs[3];

  regs[0] = ym->ton_regs.head;
  regs[1] = ym->noi_regs.head;
  regs[2] = ym->env_regs.head;

  mute
    = ( ( ym->voice_mute >> 0  ) & 1 )
    | ( ( ym->voice_mute >> 5  ) & 2 )
    | ( ( ym->voice_mute >> 10 ) & 4 )
    ;
  mute = ( mute | ( mute << 3 ) ) ^ 077;

  for ( i = 0; i < 16; ++i ) ymreg[i] = -1;

  ptr = advance_list(regs);
  do {
    curcycle  = ptr ? ptr->ymcycle : 0;
    longcycle = dump->base_cycle + (u64) curcycle;

    /* Get all entry at this cycle */
    while (ptr && ptr->ymcycle == curcycle) {
      ymreg[ ptr->reg & 15 ] = ptr->val & 255;
      ptr = advance_list(regs);
    }

    buf = tmp;

    /* dump pass number */
    for ( i = (PASS_DIGITS-1)*4; i >= 0; i -= 4) {
      *buf++ = hex [ 15 & (int)(dump->pass >> i) ];
    }
    *buf++ = ' ';

    /* dump cycle number */
    for ( i = (CYCLE_DIGITS-1)*4; i >= 0; i -= 4) {
      *buf++ = hex [ 15 & (int)(longcycle >> i) ];
    }
    /* dump register list */
    for ( i = 0; i < 14; ++i ) {

      /* disable access for muted voices */
      switch (i) {

      case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5:
        if ( ( mute & ( 1 << ( i >> 1 ) ) ) ) ymreg[i] = -1;
        break;

      case 0x8: case 0x9: case 0xA:
        if ( ( mute & ( 1 << ( i - 8 ) ) ) ) ymreg[i] = -1;
        break;

      case 0x7:
        if ( ymreg[i] >= 0 ) ymreg[i] |= mute;
        break;
      }

      *buf++ = (!i)  ? ' ' : '-';
      if (ymreg[i] < 0){
        *buf++ = '.';
        *buf++ = '.';
      } else {
        *buf++ = hex[ ymreg[i] >> 4 ];
        *buf++ = hex[ ymreg[i] & 15 ];
      }
      ymreg[i] = -1;
    }
    *buf++ = 0;
    /* Dump only if actually active */
    if (dump->active) puts(tmp);
  } while (ptr);

  /* Reset write lists */
  ym->ton_regs.head = ym->ton_regs.tail = 0;
  ym->noi_regs.head = ym->noi_regs.tail = 0;
  ym->env_regs.head = ym->env_regs.tail = 0;
  ym->waccess_nxt = ym->waccess;

  /* null terminated string and align to 32-bit */
  dump->base_cycle += (uint64_t) ymcycles;
  dump->pass++;

  len = buffersize(ym, ymcycles);

  for (i=0; i<len; ++i) {
    output[i] = 0;
  }
  return len;
}


static
void cleanup(ym_t * const ym)
{
}

int ym_dump_active(ym_t * const ym, int val)
{
  int code = -1;
  if (ym) {
    ym_dump_t * const dump = &ym->emu.dump;
    code = dump->active;
    if (val != -1) {
      dump->active = !!val;
    }
  }
  return code;
}

int ym_dump_setup(ym_t * const ym)
{
  ym_dump_t * const dump = &ym->emu.dump;
  int err = 0;

  /* fill callback functions */
  ym->cb_cleanup       = cleanup;
  ym->cb_reset         = reset;
  ym->cb_run           = run;
  ym->cb_buffersize    = buffersize;
  ym->cb_sampling_rate = (void*)0;
  dump->base_cycle     = 0;
  dump->active         = 1;
  dump->pass           = 0;

  return err;
}
