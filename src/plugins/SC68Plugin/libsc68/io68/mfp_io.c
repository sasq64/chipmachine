/*
 * @file    mfp_io.c
 * @brief   MFP-68901 I/O plugin
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-23 16:28:16 ben>
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

#include "mfp_io.h"
#include "mfpemul.h"

#ifdef DEBUG
# include <sc68/msg68.h>

static const char * const regnames[] = {
  "GPIP", /* 01 - General Purpose I/O Interrupt port     */
  "AER",  /* 03 - Active Edge Register                   */
  "DDR",  /* 05 - Data Direction Register                */
  "IERA", /* 07 - IERA (Interrupt Enable Register A)     */
  "IERB", /* 09 - IERB (Interrupt Enable Register B)     */
  "IPRA", /* 0B - IPRA (Interrupt Pending Register A)    */
  "IPRB", /* 0D - IPRB (Interrupt Pending Register B)    */
  "ISRA", /* 0F - ISRA (Interrupt In Service Register A) */
  "ISRB", /* 11 - ISRB (Interrupt In Service Register B) */
  "IMRA", /* 13 - IMRA (Interrupt Mask Register A)       */
  "IMRB", /* 15 - IMRB (Interrupt Mask Register B)       */
  "VR",   /* 17 - VR (Vector Register)                   */
  "TACR", /* 19 - TACR  (Timer A Control Register)       */
  "TBCR", /* 1B - TBCR  (Timer B Control Register)       */
  "TCDCR",/* 1D - TCDCR (Timer C/D Control Register)     */
  "TADR", /* 1F - TADR  (Timer A Data Register)          */
  "TBDR", /* 21 - TBDR  (Timer B Data Register)          */
  "TCDR", /* 23 - TCDR  (Timer C Data Register)          */
  "TDDR", /* 25 - TDDR  (Timer D Data Register)          */
  "SCR",  /* 27 - SCR (Synchronous Character Register)   */
  "UCR",  /* 29 - UCR,USART (Control Register)           */
  "RSR",  /* 2B - RSR (Receiver Status Register)         */
  "TSR",  /* 2D - TSR (Tranmitter Status Register)       */
  "UDR",  /* 2F - UDR,USART (DataRegister)               */
};

static const char * regname(int reg)
{
  if (! (reg & 1) )
    return "!ODD";                      /* not even (odd) */
  reg >>= 1;
  if (reg < 0 || reg >= sizeof(regnames)/sizeof(*regnames))
    return "!OOR";                      /* out-of-rand */
  return regnames[reg];
}

extern int mfp_cat;
# define REPORTR(N)   TRACE68(mfp_cat, "mfp: R [%02x] -> $%02x (%s)\n", N, mfp->map[N], regname(N))
# define REPORTW(N,V) TRACE68(mfp_cat, "mfp: W [%02x] <- $%02x (%s)\n", N, V, regname(N))
#else
# define REPORTR(N)
# define REPORTW(N,V)
#endif

typedef struct {
  io68_t io;
  mfp_t  mfp;
} mfp_io68_t;

/* $$$ Currently hardcoded for 8Mhz cpu */
#define cpu2bogo(mfpio,cycle) ((bogoc68_t)((cycle)*192u))
#define bogo2cpu(mfpio,bogoc) ((cycle68_t)((bogoc)/* +191u */)/192u)

/* 0  GPIP   General purpose I/O */
static int68_t mfpr_01(mfp_t * const mfp, const bogoc68_t bogoc) {
  REPORTR(GPIP);
  return mfp->map[GPIP];
}
static void mfpw_01(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  REPORTW(GPIP,v);
  mfp->map[GPIP] = v;
}

/* 1  AER    Active edge register */
static int68_t mfpr_03(mfp_t * const mfp, const bogoc68_t bogoc) {
  REPORTR(AER);
  return mfp->map[AER];
}
static void mfpw_03(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  /* $$$ Writing AER might trigger an interrupt */
  REPORTW(AER,v);
  mfp->map[AER]=v;
}

/* 2  DDR    Data direction register */
static int68_t mfpr_05(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[DDR];
}
static void mfpw_05(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[DDR] = v;
}

/* 3  IERA   Interrupt enable register A */
static int68_t mfpr_07(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IERA];
}
static void mfpw_07(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  /* Disabling a line clears pending interrupt on that line */
  mfp->map[IPRA] &= v;
  mfp->map[IERA]  = v;
}

/* 4  IERB   Interrupt enable register B */
static int68_t mfpr_09(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IERB];
}
static void mfpw_09(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  /* Disabling a line clears pending interrupt on that line */
  mfp->map[IPRB] &= v;
  mfp->map[IERB]  = v;
}

/* 5  IPRA   Interrupt pending register A */
static int68_t mfpr_0B(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IPRA];
}
static void mfpw_0B(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  /* Writing IP register only clears pending interrupts. */
  mfp->map[IPRA] &= v;
}

/* 6  IPRB   Interrupt pending register B */
static int68_t mfpr_0D(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IPRB];
}
static void mfpw_0D(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  /* Writing IP register only clears pending interrupts. */
  mfp->map[IPRB] &=v;
}

/* 7  ISRA   Interrupt in-service register A */
static int68_t mfpr_0F(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[ISRA];
}
static void mfpw_0F(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[ISRA] = v;
}

/* 8  ISRB   Interrupt in-service register B */
static int68_t mfpr_11(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[ISRB];
}
static void mfpw_11(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[ISRB] = v;
}

/* 9  IMRA   Interrupt mask register A */
static int68_t mfpr_13(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IMRA];
}
static void mfpw_13(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[IMRA] = v;
}

/* A  IMRB   Interrupt mask register B */
static int68_t mfpr_15(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[IMRB];
}
static void mfpw_15(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[IMRB] = v;
}

/* B  VR     Vector register */
static int68_t mfpr_17(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[VR];
}
static void mfpw_17(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[VR] = v;
}

/* C  TACR   Timer A control register */
static int68_t mfpr_19(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[TACR];
}

/* D  TBCR   Timer B control register */
static int68_t mfpr_1B(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[TBCR];
}
/* E  TCDCR  Timers C and D control registers */
static int68_t mfpr_1D(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[TCDCR];
}

/* F  TADR   Timer A data register */
static int68_t mfpr_1F(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp_get_tdr(mfp, TIMER_A, bogoc);
}

/* 10 TBDR   Timer B data register */
static int68_t mfpr_21(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp_get_tdr(mfp, TIMER_B, bogoc);
}
/* 11 TCDR   Timer C data register */
static int68_t mfpr_23(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp_get_tdr(mfp, TIMER_C, bogoc);
}
/* 12 TDDR   Timer D data register */
static int68_t mfpr_25(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp_get_tdr(mfp, TIMER_D, bogoc);
}

/* 13 SCR    Sync character register */
static int68_t mfpr_27(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[SCR];
}
static void mfpw_27(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc) {
  mfp->map[SCR] = v;
}



/* 14 UCR    USART control register */
static int68_t mfpr_29(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x29];
}
/* 15 RSR    Receiver status register */
static int68_t mfpr_2B(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x2B];
}
/* 16 TSR    Transmitter status register */
static int68_t mfpr_2D(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x2D];
}
/* 17 UDR    USART data register */
static int68_t mfpr_2F(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x2F];
}

static int68_t mfpr_31(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x31];
}
static int68_t mfpr_33(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x33];
}
static int68_t mfpr_35(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x35];
}
static int68_t mfpr_37(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x37];
}
static int68_t mfpr_39(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x39];
}
static int68_t mfpr_3B(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x3B];
}
static int68_t mfpr_3D(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x3D];
}
static int68_t mfpr_3F(mfp_t * const mfp, const bogoc68_t bogoc) {
  return mfp->map[0x3F];
}

static void mfpw_19(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tcr(mfp, TIMER_A, v, bogoc); }

static void mfpw_1B(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tcr(mfp, TIMER_B, v, bogoc); }

static void mfpw_1D(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tcr(mfp, TIMER_C, v, bogoc); }

static void mfpw_1F(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tdr(mfp, TIMER_A, v, bogoc); }

static void mfpw_21(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tdr(mfp, TIMER_B, v, bogoc); }

static void mfpw_23(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tdr(mfp, TIMER_C, v, bogoc); }

static void mfpw_25(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp_put_tdr(mfp, TIMER_D, v, bogoc); }


static void mfpw_29(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x29]=v; }
static void mfpw_2B(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x2B]=v; }
static void mfpw_2D(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x2D]=v; }
static void mfpw_2F(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x2F]=v; }
static void mfpw_31(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x31]=v; }
static void mfpw_33(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x33]=v; }
static void mfpw_35(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x35]=v; }
static void mfpw_37(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x37]=v; }
static void mfpw_39(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x39]=v; }
static void mfpw_3B(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x3B]=v; }
static void mfpw_3D(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x3D]=v; }
static void mfpw_3F(mfp_t * const mfp, const int68_t v, const bogoc68_t bogoc)
{ mfp->map[0x3F]=v; }

/* MFP read jump table */
static int68_t (* const mfpr_func[32])
(mfp_t * const, const bogoc68_t) =
{
  mfpr_01,mfpr_03,mfpr_05,mfpr_07,
  mfpr_09,mfpr_0B,mfpr_0D,mfpr_0F,
  mfpr_11,mfpr_13,mfpr_15,mfpr_17,
  mfpr_19,mfpr_1B,mfpr_1D,mfpr_1F,
  mfpr_21,mfpr_23,mfpr_25,mfpr_27,
  mfpr_29,mfpr_2B,mfpr_2D,mfpr_2F,
  mfpr_31,mfpr_33,mfpr_35,mfpr_37,
  mfpr_39,mfpr_3B,mfpr_3D,mfpr_3F,
};

/* MFP write jump table */
static void (* const mfpw_func[32])
(mfp_t * const, const int68_t, const bogoc68_t) =
{
  mfpw_01,mfpw_03,mfpw_05,mfpw_07,
  mfpw_09,mfpw_0B,mfpw_0D,mfpw_0F,
  mfpw_11,mfpw_13,mfpw_15,mfpw_17,
  mfpw_19,mfpw_1B,mfpw_1D,mfpw_1F,
  mfpw_21,mfpw_23,mfpw_25,mfpw_27,
  mfpw_29,mfpw_2B,mfpw_2D,mfpw_2F,
  mfpw_31,mfpw_33,mfpw_35,mfpw_37,
  mfpw_39,mfpw_3B,mfpw_3D,mfpw_3F,
};

static int68_t _mfp_readB(mfp_io68_t * const mfpio, const int addr,
                          const bogoc68_t bogoc)
{
  return !(addr&1)
    ? 0
    : mfpr_func[(addr>>1)&0x1f](&mfpio->mfp, bogoc)
    ;
}

static void mfpio_readB(io68_t * const io)
{
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  io->emu68->bus_data =
    _mfp_readB((mfp_io68_t *)io,
               io->emu68->bus_addr, bogoc);
}

static void mfpio_readW(io68_t * const io)
{
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  /* Expected EVEN addr for 16 bit access */
  io->emu68->bus_data =
    _mfp_readB((mfp_io68_t *)io,
               io->emu68->bus_addr+1, bogoc);
}

static void mfpio_readL(io68_t * const io)
{
  /* Expected EVEN addr for 16 bit access */
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  const int addr = io->emu68->bus_addr;

  io->emu68->bus_data =
    (_mfp_readB((mfp_io68_t *)io, addr+1, bogoc)<<16)
    |_mfp_readB((mfp_io68_t *)io, addr+3, bogoc);
}

static inline void _mfp_writeB(mfp_io68_t * const mfpio, const int addr,
                               const int68_t v, const bogoc68_t bogoc)
{
  if (!(addr&1)) return;
  mfpw_func[(addr>>1)&0x1f](&mfpio->mfp, (u8)v, bogoc);
}

static void mfpio_writeB(io68_t * const io)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  _mfp_writeB(mfpio, io->emu68->bus_addr, io->emu68->bus_data, bogoc);
}

/* Expected EVEN addr for 16 bit access */
static void mfpio_writeW(io68_t * const io)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  _mfp_writeB(mfpio, io->emu68->bus_addr+1, io->emu68->bus_data, bogoc);
}

/* Expected EVEN addr for 32 bit access */
static void mfpio_writeL(io68_t * const io)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  _mfp_writeB(mfpio,
              io->emu68->bus_addr+1, io->emu68->bus_data>>16, bogoc);
  _mfp_writeB((mfp_io68_t *)io,
              io->emu68->bus_addr+3, io->emu68->bus_data    ,bogoc);
}

static int mfpio_reset(io68_t * const io)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io,io->emu68->cycle);
  mfp_reset(&mfpio->mfp,bogoc);
  return 0;
}

static interrupt68_t * mfpio_interrupt(io68_t * const io,
                                       const cycle68_t cycle)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io,cycle);
  interrupt68_t * const inter =
    mfp_interrupt(&mfpio->mfp, bogoc);
  if (inter) {
#ifdef DEBUG
    if (inter->cycle >= bogoc) {
      *(int *)0 = 0xDEADBEEF;
    }
#endif

    inter->cycle = bogo2cpu(io,inter->cycle);

#ifdef DEBUG
    if (inter->cycle >= cycle) {
      *(int *)0 = 0xDEADBEEF;
    }

    /* Happen in 'Wave is my Passion.sc68' with the version +191u

       Register dump:
       CS:0073 SS:007b DS:007b ES:007b FS:0033 GS:003b
       EIP:0040b38c ESP:008dfd70 EBP:008dfd88 EFLAGS:00010246(- 00 -RIZP1)
       EAX:deadbeef EBX:01d58000 ECX:00164c88 EDX:00027200
       ESI:00027200 EDI:0007fff0

       Stack dump:
       0x008dfd70:  00164bec 01d58000 00000005 004071a1
       0x008dfd80:  00164c54 00166618 008dfda8 004078f5
       0x008dfd90:  00164b90 00027200 00008008 00164408
       0x008dfda0:  00164408 00000000 008dfe08 004042cb
       0x008dfdb0:  00166618 00027200 00000095 64703380
       0x008dfdc0:  00008000 001fcec8 00000020 00000001

       Backtrace:
       =>1 0x0040b38c mfpio_interrupt+0x4c(io=0x164b90, cycle=0x27200)
       [./libsc68/io68/mfp_io.c:348]
       in sc68 (0x008dfd88)

       2 0x004078f5 emu68_level_and_interrupt+0x165(emu68=0x166618,
       cycleperpass=0x27200)
       [./libsc68/emu68/emu68.c:341]
       in sc68 (0x008dfda8)

       0x0040b38c mfpio_interrupt+0x4c [./libsc68/io68/mfp_io.c:348]
       in sc68: movl %eax,0x00000000 348 *(int *)0 = 0xDEADBEEF;
       Modules:
    */

#endif

  }
  return inter;
}

/* static cycle68_t mfpio_nextinterrupt(io68_t * const io, */
/*                                   const cycle68_t cycle) */
/* { */
/*   mfp_io68_t * const mfpio = (mfp_io68_t * const)io; */
/*   cycle68_t const ret = mfp_nextinterrupt(&mfpio->mfp); */
/*   return ret == IO68_NO_INT */
/*     ? IO68_NO_INT */
/*     : bogo2cpu(io,ret); */
/* } */

static void mfpio_adjust_cycle(io68_t * const io,
                               const cycle68_t cycle)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  const bogoc68_t bogoc = cpu2bogo(io, cycle);
  mfp_adjust_bogoc(&mfpio->mfp, bogoc);
}

void mfpio_destroy(io68_t * const io)
{
  mfp_io68_t * const mfpio = (mfp_io68_t * const)io;
  if (io) {
    mfp_cleanup(&mfpio->mfp);
    emu68_free(io);
  }
}

static io68_t mfp_io =
{
  0,
  "MK-68901",
  0xFFFFFA00, 0xFFFFFA2F,
  mfpio_readB,mfpio_readW,mfpio_readL,
  mfpio_writeB,mfpio_writeW,mfpio_writeL,
  mfpio_interrupt,/* mfpio_nextinterrupt */0,
  mfpio_adjust_cycle,
  mfpio_reset,
  mfpio_destroy
};

int mfpio_init(int * argc, char ** argv)
{
  //argc = argc;
  //argv = argv;
  return mfp_init();
}

void mfpio_shutdown(void)
{
  mfp_shutdown();
}

io68_t * mfpio_create(emu68_t * const emu68)
{
  mfp_io68_t * mfpio = 0;

  if (emu68) {
    mfpio = emu68_alloc(sizeof(*mfpio));
    if (mfpio) {
      mfpio->io = mfp_io;
      mfp_setup(&mfpio->mfp);
    }
  }
  return &mfpio->io;
}