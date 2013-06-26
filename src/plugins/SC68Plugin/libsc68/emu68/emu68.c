/*
 * @file    emu68.c
 * @brief   emu68 main API
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-28 22:45:13 ben>
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

/* generated config include */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "struct68.h"
#include "srdef68.h"
#include "excep68.h"
#include "assert68.h"

#include "emu68.h"
#include "ioplug68.h"

#include "macro68.h"

#include "inl68_exception.h"
#include <string.h>

EMU68_EXTERN linefunc68_t *line_func[1024];

/* ,-----------------------------------------------------------------.
 * |                     Internal struct access                      |
 * `-----------------------------------------------------------------'
 */

/*  Set new interrupt IO
 *  return previous pointer
 */
io68_t * emu68_set_interrupt_io(emu68_t * emu68, io68_t * io)
{
  io68_t * oldio = 0;
  if (emu68) {
    oldio = emu68->interrupt_io;
    emu68->interrupt_io = io;
  }
  return oldio;
}

void emu68_set_registers(emu68_t * emu68, const reg68_t * r, int mask)
{
  if (emu68 && r) {
    int i;
    if ( mask & ( 1 << REG68_US_IDX ) ) REG68.usp = r->usp;
    if ( mask & ( 1 << REG68_PC_IDX ) ) REG68.pc  = r->pc;
    if ( mask & (1  << REG68_SR_IDX ) ) REG68.sr  = r->sr;
    for ( i=0; i<8; i++ )
      if ( mask & ( 1 << ( REG68_D0_IDX + i ) ) ) REG68.d[i] = r->d[i];
    for ( i=0; i<8; i++)
      if ( mask & ( 1 << ( REG68_A0_IDX + i ) ) ) REG68.a[i] = r->a[i];
  }
}

void emu68_get_registers(const emu68_t * const emu68, reg68_t * r, int mask)
{
  if (emu68 && r) {
    int i;
    if ( mask & ( 1 << REG68_US_IDX ) ) r->usp = REG68.usp;
    if ( mask & ( 1 << REG68_PC_IDX ) ) r->pc  = REG68.pc;
    if ( mask & ( 1 << REG68_SR_IDX ) ) r->sr  = REG68.sr;
    for ( i=0; i<8; i++ )
      if ( mask & ( 1 << ( REG68_D0_IDX + i ) ) ) r->d[i] = REG68.d[i];
    for ( i=0; i<8; i++ )
      if ( mask & ( 1 << ( REG68_A0_IDX + i ) ) ) r->a[i] = REG68.a[i];
  }
}

void emu68_set_cycle(emu68_t * const emu68, cycle68_t cycle)
{
  emu68->cycle = cycle;
}

cycle68_t emu68_get_cycle(emu68_t * const emu68)
{
  return emu68->cycle;
}

void * emu68_set_cookie(emu68_t * const emu68, void * cookie)
{
  void * old = 0;
  if (emu68) {
    old = emu68->cookie;
    emu68->cookie = cookie;
  }
  return old;
}

void * emu68_get_cookie(emu68_t * const emu68)
{
  return emu68
    ? emu68->cookie
    : 0
    ;
}

emu68_handler_t emu68_set_handler(emu68_t * const emu68, emu68_handler_t hdl)
{
  emu68_handler_t old = 0;
  if (emu68) {
    old = emu68->handler;
    emu68->handler = hdl;
  }
  return old;
}

emu68_handler_t emu68_get_handler(emu68_t * const emu68)
{
  return emu68
    ? emu68->handler
    : 0
    ;
}

const char * emu68_exception_name(unsigned int vector)
{
  const char * ret = 0;

  static const char * xtra_names[] = {
    "hw-brkp", "hw-trace", "hw-halt"
  };
  static const char * xcpt_names[] = {
    "reset", "reset_pc", "bus-error", "addr-error",
    "illegal", "0-divide", "chk", "trapv", "privv",
    "trace", "linea", "linef"
  };
  static const char * trap_names[] = {
    "trap#0", "trap#1", "trap#2", "trap#3",
    "trap#4", "trap#5", "trap#6", "trap#7",
    "trap#8", "trap#9", "trap#A", "trap#B",
    "trap#C", "trap#D", "trap#E", "trap#F",
  };
  if (vector >= 0x100) {
    vector -= 0x100;
    if (vector < sizeof(xtra_names)/sizeof(*xtra_names))
      ret = xtra_names[vector];
  } else if (vector < sizeof(xcpt_names)/sizeof(*xcpt_names)) {
    ret = xcpt_names[vector];
  } else if (vector >= TRAP_VECTOR(0) && vector <= TRAP_VECTOR(15)) {
    ret = trap_names[vector-TRAP_VECTOR_0];
  }
  return ret;
}

/* ,-----------------------------------------------------------------.
 * |                      Onboard memory access                      |
 * `-----------------------------------------------------------------'
 */

u8 * emu68_memptr(emu68_t * const emu68, addr68_t dst, uint68_t sz)
{
  u8 * ptr = 0;
  if (emu68) {
    addr68_t end = (dst+sz)&MEMMSK68;
    if (sz > MEMMSK68+1) {
      emu68_error_add(emu68,
                      "Not enough 68K memory ($%X>=$%X)",sz,emu68->memmsk);
    } else if (end < dst) {
      emu68_error_add(emu68,
                      "68K memory overflow :($%X-%X,$%X)",dst,dst+sz,sz);
    } else {
      ptr = emu68->mem + dst;
    }
  }
  return ptr;
}

u8 * emu68_chkptr(emu68_t * const emu68, addr68_t dst, uint68_t sz)
{
  u8 * ptr = emu68_memptr(emu68,dst,sz);
  if (ptr && emu68->chk) {
    ptr = emu68->chk + (ptr - emu68->mem);
  }
  return ptr;
}

/* Peek & Poke */
int emu68_peek(emu68_t * const emu68, addr68_t addr)
{
  return emu68
    ? emu68->mem[addr&MEMMSK68]
    : -1
    ;
}

int emu68_chkpeek(emu68_t * const emu68, addr68_t addr)
{
  return (emu68 && emu68->chk)
    ? emu68->chk[addr&MEMMSK68]
    : -1
    ;
}

int emu68_poke(emu68_t * const emu68, addr68_t addr, int68_t v)
{
  return emu68
    ? (emu68->mem[addr & MEMMSK68] = v)
    : -1
    ;
}

int emu68_chkpoke(emu68_t * const emu68, addr68_t addr, int68_t v)
{
  return (emu68 && emu68->chk)
    ? (emu68->chk[addr & MEMMSK68] = v)
    : -1
    ;
}

/*  Write memory block to 68K on board memory
 */
int emu68_memput(emu68_t * const emu68, addr68_t dst, u8 *src, uint68_t sz)
{
  u8 * ptr = emu68_memptr(emu68,dst,sz);
  if (ptr) {
    memcpy(ptr,src,sz);
    return 0;
  }
  return -1;
}

/*  Read memory block from 68K on board memory
 */
int emu68_memget(emu68_t * const emu68, u8 *dst, addr68_t src, uint68_t sz)
{
  u8 * ptr = emu68_memptr(emu68,src,sz);
  if (ptr) {
    memcpy(dst,ptr,sz);
  }
  return -!ptr;
}

int emu68_memset(emu68_t * const emu68, addr68_t dst, u8 val, uint68_t sz)
{
  u8 * ptr;

  if (!emu68) {
    ptr = 0;
  } else {
    if (!sz) {
      sz = emu68->memmsk + 1 - dst;
    }
    ptr = emu68_memptr(emu68, dst, sz);
    if (ptr) {
      while (sz--) *ptr++ = val;
    }
  }
  return -!ptr;
}

int emu68_chkset(emu68_t * const emu68, addr68_t dst, u8 val, uint68_t sz)
{
  u8 * ptr;

  if (!emu68) {
    ptr = 0;
  } else {
    if (!sz) {
      sz = emu68->memmsk+1-dst;
    }
    ptr = emu68_chkptr(emu68, dst, sz);
    if (ptr) {
      while (sz--) *ptr++ = val;
    }
  }
  return -!ptr;
}

#include "crc32.inc"

uint68_t emu68_crc32(emu68_t * const emu68)
{
  uint68_t crc = 0;
  if ( emu68 ) {
    int i;
    u8 tmp [ 18 * 4 + 2 ];              /* serialize registers */
    s32 * reg32 = emu68->reg.d;
    for ( i = 0; i < 18*4; i += 4 ) {
      unsigned int v = reg32[i>>2];
      tmp[i+0] = v >> 24;
      tmp[i+1] = v >> 16;
      tmp[i+2] = v >> 8;
      tmp[i+3] = v;
    }
    tmp[i++] = emu68->reg.sr >> 8;
    tmp[i++] = emu68->reg.sr;
    assert ( i == sizeof(tmp) );

    crc = crc32(crc, tmp, i);
    crc = crc32(crc, emu68->mem, emu68->memmsk+1);
  }
  return crc;
}

/* Process a single instruction emulation. */
static inline void step68(emu68_t * const emu68)
{
  int line,opw,reg9;
  u8 * mem;

  assert( emu68->status == EMU68_NRM );

  /* TODO: check address valid */
  mem = emu68->mem + (REG68.pc & (MEMMSK68 & ~1));
  REG68.pc += 2;
  opw  = (mem[0]<<8) | mem[1];

 /* 68000 OP-WORD format :
  *  1111 0000 0000 0000 ( LINE  )
  *  0000 0001 1111 1000 ( OP    )
  *  0000 0000 0000 0111 ( REG 0 )
  *  0000 1110 0000 0000 ( REG 9 )
  */

  /*                       LINE RG9 OPCODE  RG0 */
  /*                       -------------------- */
  line = opw & 0170000; /* 1111 000 000-000 000 */
  opw -= line;          /* 0000 111 111-111 111 */
  reg9 = opw &   07000; /* 0000 111 000-000 000 */
  opw -= reg9;          /* 0000 000 111-111 111 */
  reg9 >>= 9;           /* reg9 */
  line |= opw<<3;
  line >>= 6;
  opw  &=  7;           /* 0000 000 000-000 111 */
  (line_func[line])(emu68, reg9, opw);
}

/* $$$ evil duplicated code from mem68.c */
static void chkframe(emu68_t * const emu68, addr68_t addr, const int flags)
{
  int oldchk;

  assert ( ! (addr & 0x800000) );
  addr &= MEMMSK68;
  oldchk = emu68->chk[addr];
  if ( ( oldchk & flags ) != flags ) {
    emu68->framechk |= flags;
    emu68->chk[addr] = oldchk|flags;
  }
}

/* Run a single step emulation with all pogramm controls. */
static inline int controlled_step68(emu68_t * const emu68)
{
  assert( emu68->status == EMU68_NRM );

  /* Running in debug mode. */
  if (emu68->chk) {
     /* HardWare TRACE exception */
    exception68(emu68, HWTRACE_VECTOR, -1);
    if (emu68->status != EMU68_NRM)
      return emu68->status;
     /* HardWare BREAKPOINT exception */
    if (emu68->chk[ REG68.pc & MEMMSK68 ] & EMU68_B) {
      exception68(emu68, HWBREAK_VECTOR, -1);
      if (emu68->status != EMU68_NRM)
        return emu68->status;
    }
    chkframe(emu68, REG68.pc, EMU68_X);
  }

  /* Execute 68K instruction. */
  step68(emu68);

  /* Instruction countdown */
  if ( emu68->instructions && !--emu68->instructions )
    emu68->status = EMU68_BRK;

  return emu68->status;
}

static void loop68(emu68_t * const emu68)
{
  assert( ! (emu68->finish_sp & 1 ) );
  while ( controlled_step68(emu68) == EMU68_NRM &&
          emu68->finish_sp >= (addr68_t) REG68.a[7] )
    ;
}

const char * emu68_status_name(enum emu68_status_e status)
{
  switch (status) {
  case EMU68_ERR: return "error";
  case EMU68_NRM: return "ok";
  case EMU68_STP: return "halt";
  case EMU68_BRK: return "break";
  case EMU68_XCT: return "exception";
  }
  return "unknown";
}

/* Execute one instruction. */
int emu68_step(emu68_t * const emu68)
{
  int rc = EMU68_ERR;
  if (emu68) {
    switch (emu68->status) {
    case EMU68_NRM:
      emu68->framechk = 0;
    case EMU68_BRK:
      emu68->status = EMU68_NRM;
      /* loop68(emu68); */
      controlled_step68(emu68);
    case EMU68_STP:
      rc = emu68->status;
      break;
    default:
      assert(!"unexpected value for emu68_t::status");
      break;
    }
  }
  return rc;
}

/* Continue after a break. */
int emu68_continue(emu68_t * const emu68)
{
  int rc = EMU68_ERR;

  if (emu68) {
    switch (emu68->status) {

    case EMU68_NRM:
      /* Nothing to continue: error */
      break;

    case EMU68_BRK:
      emu68->status = EMU68_NRM;
      controlled_step68(emu68);
    case EMU68_STP:
      rc = emu68->status;
      break;

    default:
      assert(!"unexpected value for emu68_t::status");
      break;
    }
  }
  return rc;
}

int emu68_finish(emu68_t * const emu68, uint68_t instructions)
{
  io68_t *io;

  if (!emu68) {
    return EMU68_ERR;
  }

  emu68->finish_sp    = REG68.a[7];
  emu68->framechk     = 0;
  emu68->instructions = instructions;

  for (io=emu68->iohead; io; io=io->next) {
    io->adjust_cycle(io,emu68->cycle);
  }
  emu68->cycle = 0;

  loop68(emu68);

  return emu68->status;
}

/* Execute one instruction. */
int emu68_interrupt(emu68_t * const emu68, cycle68_t cycleperpass)
{

  if (!emu68) {
    return EMU68_ERR;
  }

  assert ( emu68->status == EMU68_NRM || emu68->status == EMU68_STP );

  /* Get interrupt IO (MFP) if any */
  if (emu68->interrupt_io) {
    for ( ;; ) {
      const int ipl = ( REG68.sr >> SR_I_BIT ) & 7;
      interrupt68_t * t =
        emu68->interrupt_io->interrupt(emu68->interrupt_io, cycleperpass);
      if (!t)
        break;
      emu68->cycle = t->cycle;
      if (t->level > ipl) {
        inl_exception68(emu68, t->vector, t->level);
        emu68->finish_sp = (addr68_t) REG68.a[7];
        loop68(emu68);
      }
    }
  }
  emu68->cycle = cycleperpass;

  return emu68->status;
}

#if 0
/* Run until RTS level0 occurs
 * and play Interruption for 1 pass.
 * ( Specific SC68 !!! )
 */
void emu68_level_and_interrupt(emu68_t * const emu68,
                               cycle68_t cycleperpass)
{
  addr68_t stack, pc;
  cycle68_t cycle;
  io68_t *io;

  if (!emu68) {
    return;
  }
  stack = REG68.a[7];
  pc = REG68.pc;
  REG68.a[7] = stack - 4;

  /* Clear ORed memory access flags ... */
  emu68->framechk = 0;

  /* Adjust internal IO cycle counter */
  for (io=emu68->iohead; io; io=io->next) {
    (io->adjust_cycle)(io,emu68->cycle);
  }
  emu68->cycle = 0;

  /* Do until RTS */
  do {
    step68(emu68);
  } while (stack>(addr68_t)REG68.a[7]);

  cycle=0;

  /* Get interrupt IO (MFP) if any */
  if (emu68->interrupt_io) {
    interrupt68_t * t;
    int ipl = (REG68.sr & (7<<SR_I_BIT)) >> SR_I_BIT;
#ifdef _DEBUG
    int ipl_lost = 0;
    int irq_pass = 0;
#endif
    /* $$$ HACK: mfp interupt at level 5 */
    /* $$$ TEMP: FOR DEBUGIN A MUSIC !! */
    if ( /*1 ||*/ 6 > ipl ) {
      do {

        /* Spool interrupt */
        /* Execute mfp interrupt emulation */
        t = emu68->interrupt_io->interrupt(emu68->interrupt_io, cycleperpass);
        if (t) {
#ifdef DEBUG
          if (t->cycle >= cycleperpass) {
            *(int *)0 = 0xDEADBEEF;
          }
#endif


          emu68->cycle=t->cycle;
          REG68.a[7] = stack; /* $$$ Safety net */
          REG68.pc = 0x12345678;
          inl_exception68(emu68,t->vector,t->level);
          poll_rte(emu68, stack);
#ifdef _DEBUG
          if (REG68.pc != 0x12345678) {
            BREAKPOINT68;
          }
          if (REG68.a[7] != stack) {
            BREAKPOINT68;
          }
          if (!(REG68.sr & 0x2000)) {
            BREAKPOINT68;
          }
#endif
        } else {
          //          BREAKPOINT68;
          break; //$$$ ???
        }
      } while (t);
    } else {
      // ipl masks irq
#ifdef _DEBUG
      // This happens sometime
      //BREAKPOINT68;
      ipl_lost++;
#endif
    }
  }

  /* Restore A7, PC, and new cycle counter */
  REG68.a[7]  = stack;
  REG68.pc    = pc;
  emu68->cycle = cycleperpass;
}

/* Run like a JSR at pc, exec interrupt if cycleperpass > 0.
 */
void emu68_run_rts(emu68_t * const emu68,
                   addr68_t pc, cycle68_t cycleperpass)
{
  addr68_t stack;

  if (!emu68) {
    return;
  }

  /* Push current pc .*/
  stack = REG68.a[7];
  pushl(REG68.pc);
  REG68.pc = pc;

  /* Clear ORed memory access flags ... */
  emu68->framechk = 0;

  /* Adjust internal IO cycle counter & find MFP */
  if (emu68->cycle) {
    io68_t *io;
    for (io=emu68->iohead; io; io=io->next) {
      (io->adjust_cycle)(io, emu68->cycle);
    }
    emu68->cycle = 0;
  }

  /* Do until RTS */
  do {
    step68(emu68);
  } while (stack > (addr68_t)REG68.a[7]);

  /* Get interrupt IO (MFP) if any */
  if (cycleperpass && emu68->interrupt_io) {
    interrupt68_t *t;
    int ipl = (REG68.sr & (7<<SR_I_BIT)) >> SR_I_BIT;
    cycle68_t cycle = 0;

    if (6 > ipl ) {
      cycle68_t fd;

      /* Spool interrupt */
      while (fd =
             emu68->interrupt_io->next_interrupt(emu68->interrupt_io,cycle),
             fd != IO68_NO_INT) {
        /* ... This must not happen ...  */
        if( (int)fd < 0 ) {
          fd = 0;
        }

        /* ... */

        /* TIME travel to next interruption */
        cycle += fd;

        if(cycle >= cycleperpass) {
          /* interrupt after this pass */
          break;
        }

        /* Execute mfp interrupt emulation */
        t = emu68->interrupt_io->interrupt(emu68->interrupt_io,cycle);
        if (t) {
          emu68->cycle=cycle;
          REG68.a[7] = stack; /* $$$ Safety net */
          inl_exception68(emu68,t->vector,t->level);
          poll_rte(emu68,stack);
        }
      }
    }
  }
  emu68->cycle = cycleperpass;
}


/* Run emulation for given number of cycle
 */
void emu68_cycle(emu68_t * const emu68, cycle68_t cycleperpass)
{
  cycle68_t cycle;
  io68_t *io;

  /* Clear ORed memory access flags ... */
  REG68.framechk = 0;

  /* Adjust internal IO cycle counter */
  for (io=emu68->iohead; io; io=io->next) {
    (io->adjust_cycle)(io, emu68->cycle);
  }

  /* Do until RTS */
  cycle = emu68->cycle = 0;
  do
  {
    /* If not stopped, execute an instruction */
    if (1/* !REG68.status */) {
      step68(emu68);
      cycle += 4;
    }
    emu68->cycle = cycle;

    if (emu68->interrupt_io) {
      interrupt68_t *t;
      int ipl;

      ipl = (REG68.sr & (7<<SR_I_BIT)) >> SR_I_BIT;

      /* $$$ HACK : mfp interupt at level 5 */
      if (6 > ipl) {
        t = emu68->interrupt_io->interrupt(emu68->interrupt_io,cycle);
        if (t) {
          inl_exception68(emu68,t->vector,t->level);
          /* $$$ extra cycle should be added here for exception handle */
        }
      }
    }
  } while(cycle < cycleperpass);

  // $$$ This is hack : should be "cycle" and handle later
  cycle = cycleperpass;
  emu68->cycle = cycle;
}

#endif

/* Get debug mode.  */
int emu68_debugmode(emu68_t * const emu68)
{
  return emu68
    ? !!emu68->chk
    : -1
    ;
}

/* ,-----------------------------------------------------------------.
 * |                           Breakpoints                           |
 * `-----------------------------------------------------------------'
 */

static void bp_reset(emu68_t * const emu68)
{
  int id;
  for ( id=0; id<16; ++id ) {
    emu68->breakpoints[id].addr  = 0;
    emu68->breakpoints[id].count = 0;
    emu68->breakpoints[id].reset = 0;
  }
}

void emu68_bp_delall(emu68_t * const emu68)
{
  int id;
  for ( id=0; id<16; ++id ) {
    emu68_bp_del(emu68, id);
  }
}

void emu68_bp_del(emu68_t * const emu68, int id)
{
  if (emu68 && (unsigned int)id<16u) {
    if (emu68->chk && emu68->breakpoints[id].count ) {
      const addr68_t addr = emu68->breakpoints[id].addr & MEMMSK68;
      emu68->chk[addr] &= ~(EMU68_B|EMU68_M);
    }
    emu68->breakpoints[id].addr  = 0;
    emu68->breakpoints[id].count = 0;
    emu68->breakpoints[id].reset = 0;
  }
}

int emu68_bp_set(emu68_t * const emu68, int id,
                 addr68_t addr, uint68_t count, uint68_t reset)
{
  if (!emu68) {
    id = -1;
  } else {
    if ( id == -1 )
      for ( id=0; id<16 && emu68->breakpoints[id].count; ++id)
        ;
    if ( (unsigned int) id < 16u) {
      emu68->breakpoints[id].addr  = addr &= MEMMSK68;
      emu68->breakpoints[id].count = count;
      emu68->breakpoints[id].reset = reset;
      if (emu68->chk)
        emu68->chk[addr] = (emu68->chk[addr] & ~EMU68_M) | EMU68_B | (id<<4);
    } else {
      id = -1;
    }
  }
  return id;
}

int emu68_bp_find(emu68_t * const emu68, addr68_t addr)
{
  int id = -1;
  if (emu68) {
    int i;
    for ( i=0; i<16; ++i ) {
      if (emu68->breakpoints[i].count &&
          !( (emu68->breakpoints[i].addr ^ addr) & MEMMSK68 ) ) {
        id = i;
        break;
      }
    }
  }
  return id;
}


/* ,-----------------------------------------------------------------.
 * |                     Emulator init functions                     |
 * `-----------------------------------------------------------------'
 */

static emu68_parms_t def_parms;

emu68_t * emu68_create(emu68_parms_t * const parms)
{
  emu68_t * emu68 = 0;
  int memsize, membyte;
  emu68_parms_t * const p = parms ? parms : &def_parms;

  if (!p->log2mem) {
    p->log2mem = def_parms.log2mem;
  }
  if (p->log2mem < 16 || p->log2mem > 24) {
    emu68_error_add(emu68,
                    "Invalid requested amount of memory -- 2^%d", p->log2mem);
    goto error;
  }

  if (!p->clock) {
    p->clock = def_parms.clock;
  }
  if (p->clock<500000u || p->clock>60000000u) {
    emu68_error_add(emu68,
                    "Invalid clock frequency -- %u", p->clock);
    goto error;
  }

  memsize = 1 << p->log2mem;
  membyte = sizeof(emu68_t) + (memsize << !!p->debug);
  emu68   = emu68_alloc(membyte);
  if (!emu68) {
    goto error;
  }
  memset(emu68,0,membyte);
  strncpy(emu68->name,p->name?p->name:"emu68",sizeof(emu68->name)-1);
  emu68->clock = p->clock;

  emu68->log2mem = p->log2mem;
  emu68->memmsk  = memsize-1;
  if (p->debug) {
    emu68->chk = emu68->mem + memsize+8;
  }

  emu68_mem_init(emu68);
  emu68_reset(emu68);

error:
  return emu68;
}

emu68_t * emu68_duplicate(emu68_t * emu68src, const char * dupname)
{
  emu68_parms_t parms;
  emu68_t * emu68 = 0;

  if (!emu68src)
    goto error;

  /* Create an instance with the same parameters */
  parms.name    = dupname;
  parms.log2mem = emu68->log2mem;
  parms.clock   = emu68->clock;
  parms.debug   = !!emu68->chk;
  emu68 = emu68_create(&parms);
  if (!emu68)
    goto error;

  /* Copy registers and status */
  emu68->reg          = emu68src->reg;
  emu68->cycle        = emu68src->cycle;
  emu68->framechk     = emu68src->framechk;
  emu68->instructions = emu68src->instructions;
  emu68->finish_sp    = emu68src->finish_sp;
  emu68->status       = emu68src->status;

  /* Copy memory */
  memcpy(emu68->mem, emu68src->mem, 1<<emu68->log2mem);
  if (emu68->chk)
    memcpy(emu68->chk, emu68src->chk, 1<<emu68->log2mem);

  /* Copy breakpoints */
  memcpy(emu68->breakpoints, emu68src->breakpoints, sizeof(emu68->breakpoints));

error:
  return emu68;
}

/* Destroy emulator instance. */
void emu68_destroy(emu68_t * const emu68)
{
  if (emu68) {
    emu68_ioplug_destroy_all(emu68);
    emu68_free(emu68);
  }
}

/* 68000 Hardware Reset
 * - PC = 0
 * - SR = 2700
 * - A7 = end of mem - 4
 * - All registers cleared
 * - All IO reseted
 */
void emu68_reset(emu68_t * const emu68)
{
  if (emu68) {
    io68_t * io;
    for (io=emu68->iohead; io; io=io->next) {
      (io->reset)(io);
    }
    bp_reset(emu68);

    memset(REG68.d,0,sizeof(REG68.d)*2);
    REG68.pc     = 0;
    REG68.sr     = 0x2700;
    REG68.a[7]   = MEMMSK68+1-4;
    REG68.usp    = REG68.a[7];

    emu68->cycle        = 0;
    emu68->framechk     = 0;
    emu68->instructions = 0;
    emu68->finish_sp    = -1;
    emu68->status       = EMU68_NRM;
  }
}

/* ,-----------------------------------------------------------------.
 * |                    Library init functions                       |
 * `-----------------------------------------------------------------'
 */

/* Library initialize. */
int emu68_init(int * argc, char ** argv)
{
  def_parms.name    = 0;
  def_parms.log2mem = 19;            /* log2mem: 2^19 => 512 kB */
  def_parms.clock   = EMU68_ATARIST_CLOCK;
  def_parms.debug   = 0;

  /* $$$ TODO: parse argument */
  return 0;
}

/* Library shutdown. */
void emu68_shutdown(void)
{
}