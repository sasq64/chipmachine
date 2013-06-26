/*
 * @file    paulaemul.c
 * @brief   Paula emulator (Amiga soundchip)
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-27 12:13:51 ben>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_CONFIG_OPTION68_H
# include <config_option68.h>
#else
# include "default_option68.h"
#endif

#include "io68/paulaemul.h"
#include "emu68/assert68.h"
#include <sc68/msg68.h>

#ifndef DEBUG_PL_O
# define DEBUG_PL_O 0
#endif
int pl_cat = msg68_DEFAULT;

/* Define this once to calculate and copy past the volume table. This
 * is required only if PL_VOL_FIX or Tvol[] are modified.
 */
#undef PAULA_CALCUL_TABLE

#ifdef PAULA_CALCUL_TABLE
# include <math.h>
# include <stdio.h>
#endif

#define PL_VOL_FIX   16
#define PL_VOL_MUL   (1<<PL_VOL_FIX)
#define PL_MIX_FIX   (PL_VOL_FIX+1+8-16) /*(PL_VOL_FIX+2+8-16) */

#ifdef PAULA_CALCUL_TABLE

# define LN_10_OVER_10 0.230258509299
# define PL_N_DECIBEL 65

static const s16 Tvol[] = {
  0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16, 18, 20, 21, 23,
  25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 48, 50, 52, 55, 58,
  60, 63, 66, 69, 72, 75, 78, 82, 85, 89, 93, 97, 101, 105, 110,
  115, 120, 126, 132, 138, 145, 153, 161, 170, 181, 192, 206,
  221, 241, 266, 301, 361, 450
};
static uint68_t volume[65];
static uint68_t calc_volume(unsigned int vol, unsigned int mult)
{
  double r;

  if (!vol) {
    return 0;
  }
  r = (double) mult / exp((double) Tvol[64 - vol] / 100.0);
  return (uint68_t) r;
}

static void init_volume(void)
{
  int i;

  for (i = 0; i < 65; i++) {
    volume[i] = calc_volume(i, PL_VOL_MUL);
    fprintf(stderr, "XXX:%08x\n", volume[i]);
  }
}

#else

/* This table has been precalculated with the above piece of code. */
static const uint68_t volume[65] = {
  0x00000,0x006ec,0x00c9e,0x011e8,0x016fe,0x01c15,0x020a0,0x02588,
  0x029e5,0x02ec4,0x0332b,0x0376e,0x03c0c,0x04067,0x04462,0x0489d,
  0x04d1b,0x0510f,0x05537,0x05995,0x05d3d,0x0610b,0x06501,0x06920,
  0x06d6b,0x070c0,0x0755a,0x078ed,0x07c9b,0x08067,0x08450,0x08857,
  0x08c7e,0x08f55,0x093b2,0x09832,0x09b45,0x09e68,0x0a33b,0x0a687,
  0x0a9e4,0x0ad53,0x0b0d3,0x0b466,0x0b80b,0x0bbc3,0x0bf8e,0x0c36c,
  0x0c75f,0x0cb66,0x0cf82,0x0d198,0x0d5d4,0x0da26,0x0dc57,0x0e0ca,
  0x0e30d,0x0e7a3,0x0e9f7,0x0eeb1,0x0f117,0x0f5f6,0x0f86f,0x0fd73,
  0x10000,
};

static void init_volume(void) {}

#endif

/* Filled by paula_init() to avoid field order dependencies */
static paula_parms_t default_parms;

 /* big/little endian compliance */
static int msw_first = 0;

/* ,-----------------------------------------------------------------.
 * |                         Paula init                              |
 * `-----------------------------------------------------------------'
 */

static const u32 tmp = 0x1234;

int paula_init(int * argc, char ** argv)
{
  if (pl_cat == msg68_DEFAULT)
    pl_cat = msg68_cat("paula","amiga sound emulator", DEBUG_PL_O);

  /* Build (or not) volume table. */
  init_volume();

  /* Setup little/big endian swap */
  msw_first = !(*(const u8 *)&tmp);

  /* Set default default */
  default_parms.engine = PAULA_ENGINE_SIMPLE;
  default_parms.clock  = PAULA_CLOCK_PAL;
  default_parms.hz     = SAMPLING_RATE_DEF;

  /* $$$ TODO: parsing options paula options */
  return 0;
}

void paula_shutdown()
{
  msg68_cat_free(pl_cat);
  pl_cat = msg68_DEFAULT;
}

/* ,-----------------------------------------------------------------.
 * |                      Paula Sampling Rate                        |
 * `-----------------------------------------------------------------'
 */

static int set_clock(paula_t * const paula, int clock_type, uint68_t f)
{
  u64 tmp;
  const int ct_fix = paula->ct_fix;
  const int fix    = 40;

  paula->hz    = f;
  paula->clock = clock_type;
  tmp = (clock_type == PAULA_CLOCK_PAL)
    ? PAULA_PAL_FRQ
    : PAULA_NTSC_FRQ
    ;
  tmp <<= fix;
  tmp /= f;

  if ( ct_fix < fix )
    tmp >>= fix - ct_fix;
  else
    tmp <<= ct_fix - fix;
  TRACE68(pl_cat, "paula  : clock -- *%s*\n",
          clock_type == PAULA_CLOCK_PAL ? "PAL" : "NTSC");
  paula->clkperspl = (plct_t) tmp;
  return f;
}

int paula_sampling_rate(paula_t * const paula, int hz)
{
  switch (hz) {
  case PAULA_HZ_QUERY:
    hz = paula ? paula->hz : default_parms.hz;
    break;

  case PAULA_HZ_DEFAULT:
      hz = default_parms.hz;

  default:
    if (hz < SAMPLING_RATE_MIN) {
      hz = SAMPLING_RATE_MIN;
    }
    if (hz > SAMPLING_RATE_MAX) {
      hz = SAMPLING_RATE_MAX;
    }
    if (!paula) {
      default_parms.hz = hz;
    } else {
      set_clock(paula, paula->clock, hz);
    }
    TRACE68(pl_cat,"paula  : %s sampling rate -- *%dhz*\n",
            paula ? "select" : "default", hz);
    break;
  }
  return hz;
}

/* ,-----------------------------------------------------------------.
 * |                   Set/Get emulator engine                       |
 * `-----------------------------------------------------------------'
 */

static
const char * pl_engine_name(const int engine)
{
  switch (engine) {
  case PAULA_ENGINE_SIMPLE: return "SIMPLE";
  case PAULA_ENGINE_LINEAR: return "LINEAR";
  }
  return "INVALID";
}

static
const char * pl_clock_name(const int clock)
{
  switch (clock) {
  case PAULA_CLOCK_PAL:  return "PAL";
  case PAULA_CLOCK_NTSC: return "NTSC";
  }
  return "INVALID";
}

int paula_engine(paula_t * const paula, int engine)
{
  switch (engine) {
  case PAULA_ENGINE_QUERY:
    engine = paula ? paula->engine : default_parms.engine;
    break;

  default:
    msg68_warning("paula  : invalid engine -- %d\n", engine);
  case PAULA_ENGINE_DEFAULT:
    engine = default_parms.engine;
  case PAULA_ENGINE_SIMPLE:
  case PAULA_ENGINE_LINEAR:
    *(paula ? &paula->engine : &default_parms.engine) = engine;
    TRACE68(pl_cat, "paula  : %s engine -- *%s*\n",
            paula ? "select" : "default",
            pl_engine_name(engine));
    break;
  }
  return engine;
}

int paula_clock(paula_t * const paula, int clock)
{
  switch (clock) {
  case PAULA_CLOCK_QUERY:
    clock = paula ? paula->clock : default_parms.clock;
    break;

  default:
    clock = default_parms.clock;
  case PAULA_CLOCK_PAL:
  case PAULA_CLOCK_NTSC:
    if (paula) {
      set_clock(paula, clock, paula->hz);
    } else {
      default_parms.clock = clock;
    }
    break;
  }
  return clock;
}

/* ,-----------------------------------------------------------------.
 * |                         paula reset                             |
 * `-----------------------------------------------------------------'
 */

int paula_reset(paula_t * const paula)
{
  int i;

  /* reset shadow registers */
  for (i=0; i<sizeof(paula->map); i++) {
    paula->map[i] = 0;
  }

  /* reset voices */
  for (i=0; i<4; i++) {
    paula->voice[i].adr   = 0;
    paula->voice[i].start = 0;
    paula->voice[i].end   = 2;
  }

  /* Reset DMACON and INTENA/REQ to something that
   * seems acceptable to me.
   */
  paula->dmacon = 1 << 9;  /* DMA general activate, DMA audio desactivate. */
  paula->intreq = 0;       /* No interrupt request.                        */
  paula->intena = 1 << 14; /* Master interrupt enable, audio int disable.  */
  paula->adkcon = 0;       /* No modulation.                               */

  return 0;
}

void paula_cleanup(paula_t * const paula) {}

static void pl_info(paula_t * const paula)
{
  msg68_notice("paula  : engine -- *%s*\n", pl_engine_name(paula->engine));
  msg68_notice("paula  : clock -- *%s*\n", pl_clock_name(paula->clock));
  msg68_notice("paula  : sampling rate -- *%dhz*\n", (int)paula->hz);
}

int paula_setup(paula_t * const paula,
                paula_setup_t * const setup)
{

  if (!paula || !setup || !setup->mem) {
    return -1;
  }

  /* Default sampling rate */
  if (!setup->parms.hz) {
    setup->parms.hz = default_parms.hz;
  }

  /* Default clock mode */
  if (setup->parms.clock == PAULA_CLOCK_DEFAULT) {
    setup->parms.clock = default_parms.clock;
  }

  paula->mem     = setup->mem;
  paula->log2mem = setup->log2mem;
  paula->ct_fix  = ( sizeof(plct_t) << 3 ) - paula->log2mem;

  setup->parms.engine = paula_engine(paula, setup->parms.engine);
  paula_reset(paula);
  set_clock(paula, setup->parms.clock, setup->parms.hz);

  pl_info(paula);

  return 0;
}

#if 0
static void poll_irq(paula_t * const paula, unsigned int N)
{
  u8 *p = paula->map + PAULA_VOICE(N);
  paulav_t * w = paula->voice+N;

  /* Reload internal when interrupt is DENIED */
  if (
    (paula->intreq
     |
     ~((paula->intena << (8*sizeof(int)-1-14) >> (8*sizeof(int)-1))
       & paula->intena)) & (1 << (N + 7))) {
    uint68_t a,l;

    /* Get sample pointer. */
    a = (uint68_t) ( ((p[1] << 16) | (p[2] << 8) | p[3]) & 0x7FFFE )
      << PAULA_ct_fix;
    w->adr = w->start = a;
    /* Get length */
    l = (p[4] << 8) | p[5];
    l |= (!l) << 16;
    l <<= (1 + PAULA_ct_fix);
    w->end = a + l;
  }
  paula->intreq |= 1 << (N + 7);
}
#endif

/* Mix with laudio channel data (1 char instead of 2) */

static void mix_one(paula_t * const paula,
                    int N, const int shift,
                    s32 * b, int n)
{
  const u8 * const mem = paula->mem;
  paulav_t * const w   = paula->voice+N;
  u8       * const p   = paula->map+PAULA_VOICE(N);
  s16      *       b2  = (s16 *)b + shift;
  const int     ct_fix = paula->ct_fix;
  plct_t adr, stp, readr, reend, end, vol, per;
  u8 last, hasloop;

  /* Mask to get the fractionnal part of the sample counter. Therefore
   * forcing it to 0 will disable linear interpolation by forcing
   * exact sample point.
   */
  const plct_t imask = paula->engine == PAULA_ENGINE_LINEAR
    ? ( ( (plct_t) 1 << ct_fix ) - 1 )
    : 0
    ;
  const signed_plct_t one = (signed_plct_t) 1 << ct_fix;

  hasloop = 0;

 /* $$$ dunno exactly what if volume is not in proper range [0..64] */
  vol = p[9] & 127;
  if (vol >= 64) {
    vol = 64;
  }
  vol = volume[vol];

  per = ( p[6] << 8 ) + p[7];
  if (!per) per = 1;                    /* or is it +1 for all ?? */
  stp = paula->clkperspl / per;

  /* Audio irq disable for this voice :
   * Internal will be reload at end of block
   */
  readr   = ( p[1] << 16 ) | ( p[2] << 8 ) | p[3];
  readr <<= ct_fix;
  reend   = ((p[4] << 8) | p[5]);
  reend  |= (!reend) << 16;           /* 0 is 0x10000 */
  reend <<= (1 + ct_fix);             /* +1 as unit is 16-bit word */
  reend  += readr;
  assert( reend > readr );
  if (reend < readr) {
    /* $$$ ??? dunno why I did this !!! */
    return;
  }

  adr = w->adr;
  end = w->end;
  if (end < adr) {
    return;
  }

  /* mix stereo */
  do {
    int idx;
    signed_plct_t low, v0, v1;

    low = adr & imask;
    idx = adr >> ct_fix;                /* current index         */
    last = mem[idx++];                  /* save last sample read */

    if ( ( (plct_t) idx << ct_fix ) >= end )
      idx = readr >> ct_fix;            /* loop index     */
    v1 = (s8) mem[idx];                 /* next sample    */
    v0 = (s8) last;                     /* current sample */

    /* linear interpolation (or not if imask is zero) */
    v0 = ( v1 * low + v0 * ( one - low ) ) >> ct_fix;

    /* apply volume */
    v0  *= vol;
    v0 >>= PL_MIX_FIX;

    if (v0 < -16384 || v0 >= 16384)
      msg68_critical("paula  : pcm clipping -- %d\n", (int) v0);

    assert(v0 >= -16384);
    assert(v0 <   16384);

    /* Store and advance output buffer */
    *b2 += v0;
    b2  += 2;

    /* Advance */
    adr += stp;
    if (adr >= end) {
      plct_t relen = reend - readr;
      hasloop = 1;
      adr = readr + adr - end;
      end = reend;
      while (adr >= end) {
        adr -= relen;
      }
    }
  } while (--n);

  last &= 0xFF;
  p[0xA] = last + (last << 8);
  w->adr = adr;
  if (hasloop) {
    w->start = readr;
    w->end   = end;
  }
}

/* ,-----------------------------------------------------------------.
 * |                        Paula process                            |
 * `-----------------------------------------------------------------'
 */

static void clear_buffer(s32 * b, int n)
{
  const s32 v = 0;
  if (n & 1) { *b++ = v; }
  if (n & 2) { *b++ = v; *b++ = v; }
  if (n >>= 2, n) do {
      *b++ = v; *b++ = v; *b++ = v; *b++ = v;
    } while (--n);
}

void paula_mix(paula_t * const paula, s32 * splbuf, int n)
{

  if ( n > 0 ) {
    int i;
    clear_buffer(splbuf, n);
    for (i=0; i<4; i++) {
      const int right = (i^msw_first)&1;
      if ((paula->dmacon >> 9) & (paula->dmacon >> i) & 1) {
        mix_one(paula, i, right, splbuf, n);
      }
    }
  }

  /* HaxXx: assuming next mix is next frame reset beam V/H position. */
  paula->vhpos = 0;
}