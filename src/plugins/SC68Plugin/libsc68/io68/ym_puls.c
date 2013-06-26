/*
 * @file    ym_puls.c
 * @brief   YM-2149 emulator - YM-2149 pulse engine
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-27 12:01:21 ben>
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


#include "ymemul.h"
#include "emu68/assert68.h"

#include <sc68/msg68.h>
#include <sc68/string68.h>
#include <sc68/option68.h>

extern int ym_cat;                      /* defined in ymemul.c */

#ifndef INTMSB
# define INTMSB (sizeof(int)*8-1)
#endif

#define YM_PULS_FILTER 1                /* 0:none 1:fast 2:slow */

#define YM_OUT_MSK(C,B,A)                       \
  (((((C)&0x1F)<<10))                           \
   |((((B)&0x1F)<< 5))                          \
   |((((A)&0x1F)    )))

const int ym_smsk_table[8] = {
  /* 000 */ YM_OUT_MSK(00,00,00),
  /* 001 */ YM_OUT_MSK(00,00,-1),
  /* 010 */ YM_OUT_MSK(00,-1,00),
  /* 011 */ YM_OUT_MSK(00,-1,-1),
  /* 100 */ YM_OUT_MSK(-1,00,00),
  /* 101 */ YM_OUT_MSK(-1,00,-1),
  /* 110 */ YM_OUT_MSK(-1,-1,00),
  /* 111 */ YM_OUT_MSK(-1,-1,-1)
};

/*********************/
/* Filters functions */
/*********************/

static void filter_none(ym_t * const);
static void filter_1pole(ym_t * const);
static void filter_2pole(ym_t * const);
static void filter_mixed(ym_t * const);
static void filter_boxcar(ym_t * const);
static struct {
  const char * name;
  ym_puls_filter_t filter;
} filters[] = {
  { "2-poles", filter_2pole },   /* first is default */
  { "none",    filter_none  },
  { "boxcar",  filter_boxcar},
  { "1-pole",  filter_1pole },
  { "mixed",   filter_mixed }
};
static const int n_filters = sizeof(filters) / sizeof(*filters);
static int default_filter = 0;

static int reset(ym_t * const ym, const cycle68_t ymcycle)
{
  ym_puls_t * const puls = &ym->emu.puls;

  /* Reset envelop generator */
  puls->env_bit            = 0;
  puls->env_ct             = 0;

  /* Reset noise generator */
  puls->noise_gen          = 1;
  puls->noise_ct           = 0;

  /* Reset tone generator */
  puls->voice_ctA          = 0;
  puls->voice_ctB          = 0;
  puls->voice_ctC          = 0;
  puls->levels             = 0;

  /* Reset filters */
  puls->hipass_inp1 = 0;
  puls->hipass_out1 = 0;
  puls->lopass_out1 = 0;

  /* Reset butterworth */
  puls->btw.x[0] = puls->btw.x[1] = 0;
  puls->btw.y[0] = puls->btw.y[1] = 0;

  /* Butterworth low-pass cutoff=15.625khz sampling=250khz */
  puls->btw.a[0] =  0x01eac69f; /* fix 30 */
  puls->btw.a[1] =  0x03d58d3f;
  puls->btw.a[2] =  0x01eac69f;
  puls->btw.b[0] = -0x5d1253b0;
  puls->btw.b[1] =  0x24bd6e2f;

  return 0;
}

/* ,-----------------------------------------------------------------.
 * |                          Noise generator                        |
 * `-----------------------------------------------------------------'
 */

/* Perform noise generator for N ym-cycles
 *
 *   The noise generator will use the 16 Less Signifiant Bit of the
 *   32 output buffer for each entry. 16 Most Signifiant Bit are
 *   clear by this function.
 *
 *   **BEFORE** envelop_generator() and tone_generator().
 */
static int noise_generator(ym_t * const ym, int ymcycles)
{
  ym_puls_t * const puls = &ym->emu.puls;
  int rem_cycles;
  int v, noise_gen, ct, per, msk;
  s32 * b;

  rem_cycles = ymcycles & 7;
  if(!(ymcycles >>= 3)) goto finish;

  /* All inits */
  ct        = puls->noise_ct;
  noise_gen = puls->noise_gen;
  per       = ym->reg.name.per_noise & 0x1F;

  per     <<= 1;    /* because the noise generator base frequency is
                       master/16 but we have to match the envelop
                       generator frequency which is master/8.
                    */

  msk       = ym_smsk_table[7 & (ym->reg.name.ctl_mixer >> 3)];
  v         = (u16)(-(noise_gen & 1) | msk);
  b         = puls->noiptr;
  do {
    if (++ct >= per) {
      ct = 0;

      /* *** Based on MAME. Bit have been reversed for optimzation :) ***
       *
       *   The Random Number Generator of the 8910 is a 17-bit shift
       *   register. The input to the shift register is bit0 XOR bit2.
       *   bit0 is the output.
       */

      /* bit 17 := bit 0 ^ bit 2 */
      noise_gen |= ((noise_gen^(noise_gen>>2)) & 1)<<17;
      noise_gen >>= 1;
      v = (u16)(-(noise_gen & 1) | msk);
    }
    *b++ = v;
  } while (--ymcycles);

  /* Save value for next pass */
  puls->noiptr    = b;
  puls->noise_gen = noise_gen;
  puls->noise_ct  = ct;

  finish:
  /* return not mixed cycle */
  return rem_cycles;
}

/* Flush all noise registers write access and perform noise generation
 * until given cycle. The given ymcycle should/must be greater than the
 * latest write access cycle stamp.
 */
static void do_noise(ym_t * const ym, cycle68_t ymcycle)
{
  ym_puls_t * const puls = &ym->emu.puls;
  ym_waccess_t * access;
  ym_waccess_list_t * const regs = &ym->noi_regs;
  cycle68_t lastcycle;

  puls->noiptr = ym->outbuf;
  if (!ymcycle) {
    return;
  }

  for (access=regs->head, lastcycle=0; access; access=access->link) {
    int ymcycles = access->ymcycle-lastcycle;

    assert(access->ymcycle <= ymcycle);

    if (ymcycles) {
      lastcycle = access->ymcycle - noise_generator(ym,ymcycles);
    }
    ym->reg.index[access->reg] = access->val;
  }
  lastcycle = ymcycle - noise_generator(ym, ymcycle-lastcycle);
  regs->head = regs->tail = 0;
}

/* ,-----------------------------------------------------------------.
 * |                         Envelop generator                       |
 * `-----------------------------------------------------------------'
 */

#if YM_ENV_TABLE

#undef V
#define V(X) YM_OUT_MSK(X,X,X)

static const int env_uplo[64] = {
  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037),

  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
};

static const int env_uphi[64] = {
  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037),

  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
};

static const int env_upup[64] = {
  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037),

  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037)
};

static const int env_updw[64] = {
  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037),

  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000)
};

static const int env_dwlo[64] = {
  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000),

  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
  V(000),V(000),V(000),V(000),V(000),V(000),V(000),V(000),
};

static const int env_dwhi[64] = {
  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000),

  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
  V(037),V(037),V(037),V(037),V(037),V(037),V(037),V(037),
};

static const int env_dwup[64] = {
  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000),

  V(000),V(001),V(002),V(003),V(004),V(005),V(006),V(007),
  V(010),V(011),V(012),V(013),V(014),V(015),V(016),V(017),
  V(020),V(021),V(022),V(023),V(024),V(025),V(026),V(027),
  V(030),V(031),V(032),V(033),V(034),V(035),V(036),V(037)
};

static const int env_dwdw[64] = {
  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000),

  V(037),V(036),V(035),V(034),V(033),V(032),V(031),V(030),
  V(027),V(026),V(025),V(024),V(023),V(022),V(021),V(020),
  V(017),V(016),V(015),V(014),V(013),V(012),V(011),V(010),
  V(007),V(006),V(005),V(004),V(003),V(002),V(001),V(000)
};

#undef V

static  const int * waveforms[16] = {
  /*0 \_ */ env_dwlo,
  /*1 \_ */ env_dwlo,
  /*2 \_ */ env_dwlo,
  /*3 \_ */ env_dwlo,
  /*4 /_ */ env_uplo,
  /*5 /_ */ env_uplo,
  /*6 /_ */ env_uplo,
  /*7 /_ */ env_uplo,
  /*8 \\ */ env_dwdw,
  /*9 \_ */ env_dwlo,
  /*A \/ */ env_dwup,
  /*B \- */ env_dwhi,
  /*C // */ env_upup,
  /*D /- */ env_uphi,
  /*E /\ */ env_updw,
  /*F /_ */ env_uplo
};

#endif

/* Perform envelop generator for N ym-cycles
 *
 *   The envelop generator will use the 16 Lost Signifiant Bit of the
 *   32 output buffer for each entry. 16 Mores Signifiant Bit are used
 *   by noise generator and have already been setted by
 *   envelop_generator() function calls so the value must be
 *   preserved.
 *
 *   **AFTER** noise_generator() and **BEFORE** tone_generator().
 *
 *   shape format : [CONTinue | ATTack | ALTernate | HOLD]
 */

static int envelop_generator(ym_t * const ym, int ymcycles)
#if YM_ENV_TABLE
{
  ym_puls_t * const puls = &ym->emu.puls;
  int rem_cycle = ymcycles & 7;

  if((ymcycles >>= 3)) {
    const int shape = ym->reg.name.env_shape & 15;
    const int * const waveform = waveforms[shape];
    /* Do Not Repeat is NOT ([CONT] AND NOT [HOLD]) */
    const int dnr = 1 & ~((shape>>3)&~shape);
    s32 *b  = puls->envptr;
    int ct  = puls->env_ct;
    int bit = puls->env_bit;
    int per = ym->reg.name.per_env_lo | (ym->reg.name.per_env_hi<<8);

    do {
      int t = ++ct >= per;
      bit += t;
      ct &= ~-t;
      bit |= -((bit >> 6) & dnr);
      bit &= 63;
      *b++ |= waveform[bit]<<16;
    } while (--ymcycles);

    /* Save value for next pass */
    puls->envptr     = b;
    puls->env_ct     = ct;
    puls->env_bit    = bit;
  }
  return rem_cycle;
}
#else /* #if YM_ENV_TABLE */
{
  unsigned int ncycle = ymcycles;
  int rem_cycle;
  int *b;
  int ct, per;
  unsigned int bit, bitstp, restp;
  unsigned int cont, recont;
  unsigned int alt, altalt;
  int shape;

#ifdef _DEBUG
  if (ncycle < 0) {
    BREAKPOINT68;
    return 0;
  }
#endif

  rem_cycle = ncycle & 7;
  if(!(ncycle >>= 3)) return rem_cycle;

  b       = ym->envptr;

  /* period */
  ct      = ym->env_ct;
  per     = ym->reg.index[YM_ENVL] | (ym->reg.index[YM_ENVH]<<8);
  per     |= !per;
  shape   = ym->reg.index[YM_ENVTYPE];

  /* bit */
  bit     = ym->env_bit;
  bitstp  = ym->env_bitstp;
  restp   = (shape & 1) ^ 1;

  /* continue */
  cont    = ym->env_cont;
  recont  = (-((shape>>3) & 0x1)) & 0x1F;

  /* alternate */
  alt     = ym->env_alt;
  altalt  = (-((shape ^ (shape>>1)) & 0x1)) & 0x1F;

  do {
    int n;

    n = per - ct;
    if (n <= 0) {
      int prev_bit;
      ct = 0;
      n = per;
      prev_bit = bit;
      bit += bitstp;
      if ((bit^prev_bit) & 32) {
        bitstp = restp;
        cont = recont;
        alt ^= altalt;
      }
    }

    /* 5 bit version */
    int v = (bit ^ alt) & cont;
    v |= v<<5;
    v |= v<<5;
    v <<= 16;

    if (n > ncycle) {
      n = ncycle;
    }
    ncycle -= n;
    ct += n;

    do {
      *b++ |= v;
    } while (--n);

  } while (ncycle);

  ym->envptr     = b;
  ym->env_ct     = ct;
  ym->env_bit    = bit;
  ym->env_bitstp = bitstp;
  ym->env_cont   = cont;
  ym->env_alt    = alt;

  return rem_cycle;
}

#endif /* #if YM_ENV_TABLE */


/*
 * Flush all envelop registers write access and perform envelop
 * generation until given cycle. The given ymcycle should/must be
 * greater than the latest write access cycle stamp.
 */
static void do_envelop(ym_t * const ym, cycle68_t ymcycle)
{
  ym_puls_t * const puls = &ym->emu.puls;
  ym_waccess_t * access;
  ym_waccess_list_t * const regs = &ym->env_regs;

  cycle68_t lastcycle;

  puls->envptr = ym->outbuf;
  if (!ymcycle) {
    return;
  }

  for (access=regs->head, lastcycle=0; access; access=access->link) {
    int ymcycles = access->ymcycle-lastcycle;

    if (access->ymcycle > ymcycle) {
      TRACE68(ym_cat,"%s access reg %X out of frame!! (%u>%u %u)\n",
              regs->name, access->reg, access->ymcycle, ymcycle,
              access->ymcycle/ymcycle);
      break;
    }

    if (ymcycles) {
      lastcycle = access->ymcycle - envelop_generator(ym,ymcycles);
    }

    ym->reg.index[access->reg] = access->val;
    if(access->reg == YM_ENVTYPE) {
#if YM_ENV_TABLE
      puls->env_bit = 0;
      puls->env_ct  = 0; /* $$$ Needs to be verifed. It seems cleaner. */
#else
      int shape = access->val & 15;
      /* Alternate mask start value depend on ATTack bit */
      puls->env_alt    = (~((shape << (INTMSB-2)) >> INTMSB)) & 0x1F;
      puls->env_bit    = 0;
      puls->env_bitstp = 1;
      puls->env_cont   = 0x1f;
      puls->env_ct = 0; /* $$$ Needs to be verifed. It seems cleaner. */
#endif

    }
  }
  envelop_generator(ym, ymcycle-lastcycle);
  regs->head = regs->tail = 0;
}

/* ,-----------------------------------------------------------------.
 * |                  Tone generator and mixer                       |
 * `-----------------------------------------------------------------'
 */

static int tone_generator(ym_t  * const ym, int ymcycles)
{
  ym_puls_t * const puls = &ym->emu.puls;

  int ctA,  ctB,  ctC;
  int perA, perB, perC;
  int smsk, emsk, vols;

  s32 * b;
  int rem_cycles, v;
  int levels;
  int mute;

  rem_cycles = ymcycles & 7;
  ymcycles >>= 3;
  if(!ymcycles) goto finish;

  /* init buffer address */
  b = puls->tonptr;

  mute = ym->voice_mute & YM_OUT_MSK_ALL;
  smsk = ym_smsk_table[7 & ym->reg.name.ctl_mixer];

  /* 3 voices buzz or lvl mask */
  emsk = vols = 0;

  v = ym->reg.name.vol_a & 0x1F;
  if(v&0x10) emsk |= YM_OUT_MSK_A;
  else       vols |= (v<<1)+1;

  v = ym->reg.name.vol_b & 0x1F;
  if(v&0x10) emsk |= YM_OUT_MSK_B;
  else       vols |= (v<<6)+(1<<5);

  v = ym->reg.name.vol_c & 0x1F;
  if(v&0x10) emsk |= YM_OUT_MSK_C;
  else       vols |= (v<<11)+(1<<10);

  /* Mixer steps & couters */
  ctA = puls->voice_ctA;
  ctB = puls->voice_ctB;
  ctC = puls->voice_ctC;

  perA = ym->reg.name.per_a_lo | ((ym->reg.name.per_a_hi&0xF)<<8);
  perB = ym->reg.name.per_b_lo | ((ym->reg.name.per_b_hi&0xF)<<8);
  perC = ym->reg.name.per_c_lo | ((ym->reg.name.per_c_hi&0xF)<<8);

  levels = puls->levels;
  do {
    int sq;

    sq = -(++ctA >= perA);
    levels ^= YM_OUT_MSK_A & sq;
    ctA &= ~sq;

    sq = -(++ctB >= perB);
    levels ^= YM_OUT_MSK_B & sq;
    ctB &= ~sq;

    sq = -(++ctC >= perC);
    levels ^= YM_OUT_MSK_C & sq;
    ctC &= ~sq;

    sq = levels;
    sq |= smsk;
    {
      unsigned int eo = *b; /* EEEENNNN */
      sq &= eo; eo >>= 16;
      sq &= (eo&emsk) | vols;
    }

    sq &= mute;

    sq = (int) ym->ymout5[sq];

    *b++ = sq;

  } while (--ymcycles);

  /* Save value for next pass */
  puls->tonptr    = b;
  puls->voice_ctA = ctA;
  puls->voice_ctB = ctB;
  puls->voice_ctC = ctC;
  puls->levels    = levels;
  finish:
  return rem_cycles;
}

/*
 * Flush all tone registers write access and perform tone generation
 * and mixer until given cycle. The given ymcycle should/must be
 * greater than the latest write access cycle stamp.
 */
static void do_tone_and_mixer(ym_t * const ym, cycle68_t ymcycle)
{
  ym_puls_t * const puls = &ym->emu.puls;

  ym_waccess_t * access;
  ym_waccess_list_t * const regs = &ym->ton_regs;
  cycle68_t lastcycle;

  puls->tonptr = ym->outbuf;
  if (!ymcycle) {
    return;
  }

  for (access=regs->head, lastcycle=0; access; access=access->link) {
    const int ymcycles = access->ymcycle - lastcycle;

    if (access->ymcycle > ymcycle) {
      TRACE68(ym_cat,"%s access reg %X out of frame!! (%u>%u %u)\n",
              regs->name, access->reg, access->ymcycle, ymcycle,
              access->ymcycle/ymcycle);
      break;
    }

    if (ymcycles) {
      lastcycle = access->ymcycle - tone_generator(ym, ymcycles);
    }
    ym->reg.index[access->reg] = access->val;
  }
  tone_generator(ym, ymcycle-lastcycle);
  regs->head = regs->tail = 0;
}

/* ,-----------------------------------------------------------------.
 * |                         Run emulation                           |
 * `-----------------------------------------------------------------'
 */

/******************************************************/
/*                                                    */
/* Recursive single pole low-pass filter              */
/* -------------------------------------              */
/*                                                    */
/*   o[N] = i[N] * A + o[N-1] * B                     */
/*                                                    */
/*   X  = exp(-2.0 * pi * Fc)                         */
/*   A  = 1 - X = 1 - B                               */
/*   B  = X                                           */
/*   Fc = cutoff / rate                               */
/*                                                    */
/*                                                    */
/* Recursive single pole high-pass filter             */
/* --------------------------------------             */
/*                                                    */
/*   o[N] = A0 * i[N] + A1 * i[N-1] + B1 * o[N-1]     */
/*        = A0 * i[N] - A0 * i[N-1] + B1 * o[N-1]     */
/*        = A0 * ( i[N] - i[N-1] )  + B1 * o[N-1]     */
/*                                                    */
/*   X  = exp(-2.0 * pi * Fc)                         */
/*   A0 = (1 + X) / 2                                 */
/*   A1 = -A0                                         */
/*   B1 = X                                           */
/*   Fc = cutoff / rate                               */
/*                                                    */
/*                                                    */
/* Butterworth                                        */
/* -----------                                        */
/*     o[N] = A0 * i[N-0] + A1 * i[N-1] + A2 * i[N-2] */
/*                        - B0 * o[N-1] - B1 * o[N-2] */
/*                                                    */
/*                                                    */
/* Butterworth low-pass                               */
/* --------------------                               */
/*                                                    */
/*   c  = 1 / tan(pi * cutoff / rate)                 */
/*   a0 = 1 / (1 + sqrt(2) * c + c^2)                 */
/*   a1 = 2 * a0                                      */
/*   a2 = a0                                          */
/*   b0 = 2 * (1 - c^2) * a0                          */
/*   b1 = (1 - sqrt(2.0) * c + c^2) * a0              */
/*                                                    */
/*                                                    */
/* Butterworth high-pass                              */
/* ---------------------                              */
/*                                                    */
/*   c  = tan(M_PI * cutoff / rate                    */
/*   a0 = 1 / (1 + sqrt(2) * c + c^2)                 */
/*   a1 = -2 * a0                                     */
/*   a2 = a0                                          */
/*   b0 = 2 * (c^2 - 1) * a0                          */
/*   b1 = (1 - sqrt(2) * c + c^2) * a0                */
/*                                                    */
/******************************************************/


#if 0
/* What it should be ... */
# define REVOL(V) ((V) * _vol >> 6)
# define CLIP3(V,A,B) ( V < A ? A : ( V > B ? B : V ) )
#else
/* What it really is (BLEP does not honnor it anyway) */
# define REVOL(V) ((V) >> 1)
# define CLIP3(V,A,B) ( V < A ? A : ( V > B ? B : V ) )
#endif
#define CLIP(V) CLIP3(V,-32768,32767)


static inline int clip(int o)
{
#ifdef DEBUG
  static int max, min;
  if ( o < min ) {
    min = o;
    msg68(ym_cat,"ym-2149: pulse -- pcm min -- %d\n", o);
  }
  if ( o > max ) {
    max = o;
    msg68(ym_cat,"ym-2149: pulse -- pcm max -- %d\n", o);
  }
  if (o < -32768) {
    msg68(ym_cat,"ym-2149: pulse -- pcm clip -- %d < -32768\n", o);
    o = -32768;
  }
  if (o > 32767) {
    msg68(ym_cat,"ym-2149: pulse -- pcm clip -- %d > 32767\n", o);
    o = 32767;
  }
#endif
  return CLIP(o);
}

/* Resample ``n'' input samples from ``irate'' to ``orate''
 * With volume adjustement [0..64]
 * @warning irate <= 262143 or 32bit overflow
 */
static s32 * resampling(s32 * dst, const int n,
/*                         const int _vol, */
                        const uint68_t irate, const uint68_t orate)
{
  s32   * const src = dst;
  const int68_t stp = (irate << 14) / orate; /* step into source */

  if ( 0 == (stp & ((1<<14)-1)) ) {
    const int istp = stp >> 14;
    const int iend = n;
    int idx        = 0;
    /* forward */
    do {
      int o = REVOL(src[idx]);
      *dst++ = clip(o);
    } while ((idx += istp) < iend);
  } else {
    const int68_t end = n << 14;
    int68_t       idx = 0;

    if (stp >= 1<<14) {
      /* forward */
      do {
        int o = REVOL(src[(int)(idx>>14)]);
        *dst++ = clip(o);
      } while ((idx += stp) < end);
    } else {
      /* backward */
      const int m = (n * orate + irate - 1) / irate; /* output samples */
      dst  = src + m - 1;
      idx  = end;
      do {
        int o = REVOL(src[(int)((idx -= stp)>>14)]);
        *dst = clip(o);
      } while (--dst != src);
      dst = src+m;
    }
  }
  return dst;
}

static void filter_none(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = (puls->tonptr - ym->outbuf);

  if (n > 0) {
    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>3, ym->hz);
  }
}

static void filter_boxcar2(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = (puls->tonptr - ym->outbuf) >> 1;

  if (n > 0) {
    int m = n;
    s32 * src = ym->outbuf, * dst = ym->outbuf;

    do {
      *dst++ = ( src[0] + src[1] ) >> 1;
      src += 2;
    } while (--m);

    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>(3+1), ym->hz);
  }
}

static void filter_boxcar4(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = (puls->tonptr - ym->outbuf) >> 2;

  if (n > 0) {
    int m = n;
    s32 * src = ym->outbuf, * dst = ym->outbuf;

    do {
      *dst++ = ( src[0] + src[1] + src[2] + src[3] ) >> 2;
      src += 4;
    } while (--m);

    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>(3+2), ym->hz);
  }
}

/** Use 2-boxcar or 4-boxcar filter so that boxcar output rate not
    less than output sampling rate. */
static void filter_boxcar(ym_t * const ym) {
  /* Select boxcar width  */
  if (ym->hz > (ym->clock >> (3+2)))
    filter_boxcar2(ym);
  else
    filter_boxcar4(ym);
}


/* - 4-boxcar filter resamples to 62500hz
 * - Empirical lowpass filter (+adjust output level)
 * - 1-pole 25hz hipass filter.
 */
static void filter_mixed(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = (puls->tonptr - ym->outbuf) >> 2; /* Number of block */

  if (n > 0) {
    s32 * src = ym->outbuf, * dst = src;
    int68_t h_i1 = puls->hipass_inp1;
    int68_t h_o1 = puls->hipass_out1;
    int68_t l_o1 = puls->lopass_out1;
    int m = n;

    do {
      int68_t i0,o0;

      /***********************************************************/
      /* 4-tap boxcar filter; lower sampling rate from 250Khz to */
      /* 62.5Khz; emulates half level buzz sounds.               */
      /***********************************************************/
      i0  = ( src[0] + src[1] + src[2] + src[3] ) >> 2;
      src += 4;

      /*****************************************/
      /* Recursive single pole low-pass filter */
      /* - cutoff   : 15.625 Khz               */
      /* - sampling : 62.5 Khz                 */
      /*****************************************/
      if (1) {
        const int68_t B = 0x1a9c; /* 15 bit */
        const int68_t A = (1<<15)-B;
        l_o1 = ( (i0 * A) + l_o1 * B ) >> 15;
      } else {
        l_o1 = i0;
      }

      /******************************************/
      /* Recursive single pole high-pass filter */
      /* - cutoff   : 25 hz                     */
      /* - sampling : 62.5 Khz                  */
      /******************************************/
      if (1) {
        const int A0 = 0x7FD7; /* 15 bit */
        const int B1 = 0x7FAE; /* 15 bit */
        o0 = h_o1 = ( (l_o1 - h_i1) * A0 + (h_o1 * B1) ) >> 15;
        h_i1 = l_o1;
      } else {
        o0 = i0;
      }

      /* store */
      *dst++ = o0;

    } while (--m);

    puls->hipass_inp1 = h_i1;
    puls->hipass_out1 = h_o1;
    puls->lopass_out1 = l_o1;

    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>(3+2), ym->hz);
  }
}

static void filter_1pole(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = puls->tonptr - ym->outbuf;

  if (n > 0) {
    s32 * src = ym->outbuf, * dst = src;

    int68_t h_i1 = puls->hipass_inp1;
    int68_t h_o1 = puls->hipass_out1;
    int68_t l_o1 = puls->lopass_out1;
    int m = n;

    do {
      int68_t i0,o0;

      i0  = *src++;

      /*****************************************/
      /* Recursive single pole low-pass filter */
      /* - cutoff   : 15.625 Khz               */
      /* - sampling : 250 Khz                  */
      /*****************************************/
      {
        const int68_t B = 0x7408; /* 15 bit */
        const int68_t A = (1<<15)-B;
        l_o1 = ( (i0 * A) + l_o1 * B ) >> 15;
      }

      /******************************************/
      /* Recursive single pole high-pass filter */
      /* - cutoff   : 25 hz                     */
      /* - sampling : 250 Khz                   */
      /******************************************/
      {
        const int68_t A0 = 0x7FF6; /* 15 bit */
        const int68_t B1 = 0x7FEB; /* 15 bit */
        o0 = h_o1 = ( (l_o1 - h_i1) * A0 + (h_o1 * B1) ) >> 15;
        h_i1 = l_o1;
      }

      /* store */
      *dst++ = o0;

    } while (--m);

    puls->hipass_inp1 = h_i1;
    puls->hipass_out1 = h_o1;
    puls->lopass_out1 = l_o1;

    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>(3+0), ym->hz);
  }
}


/* Transform 250000Hz buffer to current sampling rate.
 *
 * Using a butterworth passband filter
 *
 */
static void filter_2pole(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  const int n = puls->tonptr - ym->outbuf;

  if (n > 0) {
    s32 * src = ym->outbuf, * dst = src;
    int m = n;

    int68_t h_i1 = puls->hipass_inp1;
    int68_t h_o1 = puls->hipass_out1;

    const int68_t a0 = puls->btw.a[0] >> 15;
    const int68_t a1 = puls->btw.a[1] >> 15;
    const int68_t a2 = puls->btw.a[2] >> 15;
    const int68_t b0 = puls->btw.b[0] >> 15;
    const int68_t b1 = puls->btw.b[1] >> 15;

    int68_t x0 = puls->btw.x[0];
    int68_t x1 = puls->btw.x[1];
    int68_t y0 = puls->btw.y[0];
    int68_t y1 = puls->btw.y[1];

    do {
      int68_t i0,o0;

      i0  = *src++;

      /******************************************/
      /* Recursive single pole high-pass filter */
      /* - cutoff   : 25 hz                     */
      /* - sampling : 250 Khz                   */
      /******************************************/
      {
        const int68_t A0 = 0x7FF6; /* 15 bit */
        const int68_t B1 = 0x7FEB; /* 15 bit */
        h_o1 = (((i0 - h_i1) * A0) + (h_o1 * B1)) >> 15;
        h_i1 = i0;
        i0   = h_o1;
      }

      /* Butterworth low-pass  */
      {
        o0 = (
          a0 * i0 + a1 * x0 + a2 * x1 -
          b0 * y0 - b1 * y1 ) >> 15;
        x1 = x0; x0 = i0;
        y1 = y0; y0 = o0;
      }

      *dst++ = o0;

    } while (--m);

    puls->btw.x[0] = x0;
    puls->btw.x[1] = x1;
    puls->btw.y[0] = y0;
    puls->btw.y[1] = y1;

    puls->hipass_inp1 = h_i1;
    puls->hipass_out1 = h_o1;

    ym->outptr =
      resampling(ym->outbuf, n, /* 64, */ ym->clock>>3, ym->hz);
  }
}

static
int run(ym_t * const ym, s32 * output, const cycle68_t ymcycles)
{
  ym->outbuf = ym->outptr = output;

  do_noise(ym,ymcycles);
  do_envelop(ym,ymcycles);
  do_tone_and_mixer(ym,ymcycles);

  ym->waccess     = ym->static_waccess;
  ym->waccess_nxt = ym->waccess;

  filters[ym->emu.puls.ifilter].filter(ym);

  return ym->outptr - ym->outbuf;
}


static
int buffersize(const ym_t const * ym, const cycle68_t ymcycles)
{
  return ((ymcycles+7u) >> 3);
}

static
void cleanup(ym_t * const ym)
{
}

int ym_puls_setup(ym_t * const ym)
{
  ym_puls_t * const puls = &ym->emu.puls;
  int err = 0;

  /* fill callback functions */
  ym->cb_cleanup       = cleanup;
  ym->cb_reset         = reset;
  ym->cb_run           = run;
  ym->cb_buffersize    = buffersize;
  ym->cb_sampling_rate = 0;

  /* use default filter */
  puls->ifilter        = default_filter;
  msg68_notice("ym-2149: filter -- *%s*\n", filters[puls->ifilter].name);

  return err;
}

/* command line options option */
static const char prefix[] = "sc68-";
static const char engcat[] = "ym-puls";
static option68_t opts[] = {
  { option68_STR, prefix, "ym-filter", engcat,
    "set ym-2149 filter [none|boxcar|mixed|1-pole|2-pole*]" },
};

int ym_puls_options(int argc, char ** argv)
{
  option68_t * opt;
  const int n_opts = sizeof(opts) / sizeof(*opts);

  /* Add local options */
  option68_append(opts, n_opts);

  /* Parse options */
  argc = option68_parse(argc,argv,0);

  /* --sc68-ym-filter= */
  opt = option68_get("ym-filter",1);
  if (opt) {
    int i;
    for (i=0; i<n_filters; ++i) {
      if (!strcmp68(opt->val.str, filters[i].name)) {
        default_filter = i;
        break;
      }
    }
    if (i == n_filters) {
      msg68_warning("ym-2149: invalid filter -- *%s*\n", opt->val.str);
    }
  }
  msg68_notice("ym-2149: default filter -- *%s* \n",
               filters[default_filter].name);

  return argc;
}