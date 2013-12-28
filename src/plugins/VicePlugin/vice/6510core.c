/*
 * 6510core.c - MOS6510 emulation core.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * DTV sections written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

/* This file is included by CPU definition files */
/* (maincpu.c, drivecpu.c, ...) */

#ifdef DRIVE_CPU
#define CPU_STR "Drive CPU"
#else
#define CPU_STR "Main CPU"
#endif

#include "traps.h"

#ifndef C64DTV
/* The C64DTV can use different shadow registers for accu read/write. */
/* For standard 6510, this is not the case. */
#define reg_a_write reg_a
#define reg_a_read  reg_a

/* Opcode execution time may vary on the C64DTV. */
#define CLK_RTS 3
#define CLK_RTI 4
#define CLK_BRK 5
#define CLK_ABS_I_STORE2 2
#define CLK_STACK_PUSH 1
#define CLK_STACK_PULL 2
#define CLK_ABS_RMW2 3
#define CLK_ABS_I_RMW2 3
#define CLK_ZERO_I_STORE 2
#define CLK_ZERO_I2 2
#define CLK_ZERO_RMW 3
#define CLK_ZERO_I_RMW 4
#define CLK_IND_X_RMW 3
#define CLK_IND_Y_RMW1 1
#define CLK_IND_Y_RMW2 3
#define CLK_BRANCH2 1
#define CLK_INT_CYCLE 1
#define CLK_JSR_INT_CYCLE 1
#define CLK_IND_Y_W 2
#define CLK_NOOP_ZERO_X 2

#define IRQ_CYCLES      7
#define NMI_CYCLES      7
#endif
#define RESET_CYCLES    6

/* ------------------------------------------------------------------------- */
/* Backup for non-6509 CPUs.  */

#ifndef LOAD_IND
#define LOAD_IND(a)     LOAD(a)
#endif
#ifndef STORE_IND
#define STORE_IND(a,b)  STORE(a,b)
#endif

/* ------------------------------------------------------------------------- */
/* Backup for non-variable cycle CPUs.  */

#ifndef CLK_ADD
#define CLK_ADD(clock, amount) clock+=amount
#endif

#ifndef REWIND_FETCH_OPCODE
#define REWIND_FETCH_OPCODE(clock) clock-=2
#endif

/* ------------------------------------------------------------------------- */
/* Hook for additional delay.  */

#ifndef CPU_DELAY_CLK
#define CPU_DELAY_CLK
#endif

#ifndef CPU_REFRESH_CLK
#define CPU_REFRESH_CLK
#endif

/* ------------------------------------------------------------------------- */

#define LOCAL_SET_NZ(val)        (flag_z = flag_n = (val))

#if defined DRIVE_CPU
#define LOCAL_SET_OVERFLOW(val)               \
    do {                                      \
        if (!(val))                           \
            drivecpu_byte_ready_egde_clear(); \
        if (val)                              \
            reg_p |= P_OVERFLOW;              \
        else                                  \
            reg_p &= ~P_OVERFLOW;             \
    } while (0)
#else
#define LOCAL_SET_OVERFLOW(val)   \
    do {                          \
        if (val)                  \
            reg_p |= P_OVERFLOW;  \
        else                      \
            reg_p &= ~P_OVERFLOW; \
    } while (0)
#endif

#define LOCAL_SET_BREAK(val)   \
    do {                       \
        if (val)               \
            reg_p |= P_BREAK;  \
        else                   \
            reg_p &= ~P_BREAK; \
    } while (0)

#define LOCAL_SET_DECIMAL(val)   \
    do {                         \
        if (val)                 \
            reg_p |= P_DECIMAL;  \
        else                     \
            reg_p &= ~P_DECIMAL; \
    } while (0)

#define LOCAL_SET_INTERRUPT(val)   \
    do {                           \
        if (val)                   \
            reg_p |= P_INTERRUPT;  \
        else                       \
            reg_p &= ~P_INTERRUPT; \
    } while (0)

#define LOCAL_SET_CARRY(val)   \
    do {                       \
        if (val)               \
            reg_p |= P_CARRY;  \
        else                   \
            reg_p &= ~P_CARRY; \
    } while (0)

#define LOCAL_SET_SIGN(val)      (flag_n = (val) ? 0x80 : 0)
#define LOCAL_SET_ZERO(val)      (flag_z = !(val))
#define LOCAL_SET_STATUS(val)    (reg_p = ((val) & ~(P_ZERO | P_SIGN)), \
                                  LOCAL_SET_ZERO((val) & P_ZERO),       \
                                  flag_n = (val))

#define LOCAL_OVERFLOW()         (reg_p & P_OVERFLOW)
#define LOCAL_BREAK()            (reg_p & P_BREAK)
#define LOCAL_DECIMAL()          (reg_p & P_DECIMAL)
#define LOCAL_INTERRUPT()        (reg_p & P_INTERRUPT)
#define LOCAL_CARRY()            (reg_p & P_CARRY)
#define LOCAL_SIGN()             (flag_n & 0x80)
#define LOCAL_ZERO()             (!flag_z)
#define LOCAL_STATUS()           (reg_p | (flag_n & 0x80) | P_UNUSED    \
                                  | (LOCAL_ZERO() ? P_ZERO : 0))

#ifdef LAST_OPCODE_INFO

/* If requested, gather some info about the last executed opcode for timing
   purposes.  */

/* Remember the number of the last opcode.  By default, the opcode does not
   delay interrupt and does not change the I flag.  */
#define SET_LAST_OPCODE(x) \
    OPINFO_SET(LAST_OPCODE_INFO, (x), 0, 0, 0)

/* Remember that the last opcode delayed a pending IRQ or NMI by one cycle.  */
#define OPCODE_DELAYS_INTERRUPT() \
    OPINFO_SET_DELAYS_INTERRUPT(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 0 to 1, so we have
   to dispatch an IRQ even if the I flag is 0 when we check it.  */
#define OPCODE_DISABLES_IRQ() \
    OPINFO_SET_DISABLES_IRQ(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 1 to 0, so we must
   not dispatch an IRQ even if the I flag is 1 when we check it.  */
#define OPCODE_ENABLES_IRQ() \
    OPINFO_SET_ENABLES_IRQ(LAST_OPCODE_INFO, 1)

#else

/* Info about the last opcode is not needed.  */
#define SET_LAST_OPCODE(x)
#define OPCODE_DELAYS_INTERRUPT()
#define OPCODE_DISABLES_IRQ()
#define OPCODE_ENABLES_IRQ()

#endif

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

#ifndef DRIVE_CPU

#ifndef C64DTV
/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()      \
  do {                          \
      GLOBAL_REGS.pc = reg_pc;  \
      GLOBAL_REGS.a = reg_a_read;    \
      GLOBAL_REGS.x = reg_x;    \
      GLOBAL_REGS.y = reg_y;    \
      GLOBAL_REGS.sp = reg_sp;  \
      GLOBAL_REGS.p = reg_p;    \
      GLOBAL_REGS.n = flag_n;   \
      GLOBAL_REGS.z = flag_z;   \
  } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()      \
  do {                          \
      reg_a_write /*TODO*/= GLOBAL_REGS.a;    \
      reg_x = GLOBAL_REGS.x;    \
      reg_y = GLOBAL_REGS.y;    \
      reg_sp = GLOBAL_REGS.sp;  \
      reg_p = GLOBAL_REGS.p;    \
      flag_n = GLOBAL_REGS.n;   \
      flag_z = GLOBAL_REGS.z;   \
      JUMP(GLOBAL_REGS.pc);     \
  } while (0)
#else  /* C64DTV */

/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()      \
  do {                          \
      GLOBAL_REGS.pc = reg_pc;  \
      GLOBAL_REGS.a = dtv_registers[0]; \
      GLOBAL_REGS.x = dtv_registers[2]; \
      GLOBAL_REGS.y = dtv_registers[1]; \
      GLOBAL_REGS.sp = reg_sp;  \
      GLOBAL_REGS.p = reg_p;    \
      GLOBAL_REGS.n = flag_n;   \
      GLOBAL_REGS.z = flag_z;   \
      GLOBAL_REGS.r3 = dtv_registers[3]; \
      GLOBAL_REGS.r4 = dtv_registers[4]; \
      GLOBAL_REGS.r5 = dtv_registers[5]; \
      GLOBAL_REGS.r6 = dtv_registers[6]; \
      GLOBAL_REGS.r7 = dtv_registers[7]; \
      GLOBAL_REGS.r8 = dtv_registers[8]; \
      GLOBAL_REGS.r9 = dtv_registers[9]; \
      GLOBAL_REGS.r10 = dtv_registers[10]; \
      GLOBAL_REGS.r11 = dtv_registers[11]; \
      GLOBAL_REGS.r12 = dtv_registers[12]; \
      GLOBAL_REGS.r13 = dtv_registers[13]; \
      GLOBAL_REGS.r14 = dtv_registers[14]; \
      GLOBAL_REGS.r15 = dtv_registers[15]; \
      GLOBAL_REGS.acm = (reg_a_write_idx << 4) | (reg_a_read_idx); \
      GLOBAL_REGS.yxm = (reg_y_idx << 4) | (reg_x_idx); \
  } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()      \
  do {                          \
      dtv_registers[0] = GLOBAL_REGS.a; \
      dtv_registers[2] = GLOBAL_REGS.x; \
      dtv_registers[1] = GLOBAL_REGS.y; \
      reg_sp = GLOBAL_REGS.sp;  \
      reg_p = GLOBAL_REGS.p;    \
      flag_n = GLOBAL_REGS.n;   \
      flag_z = GLOBAL_REGS.z;   \
      dtv_registers[3] = GLOBAL_REGS.r3; \
      dtv_registers[4] = GLOBAL_REGS.r4; \
      dtv_registers[5] = GLOBAL_REGS.r5; \
      dtv_registers[6] = GLOBAL_REGS.r6; \
      dtv_registers[7] = GLOBAL_REGS.r7; \
      dtv_registers[8] = GLOBAL_REGS.r8; \
      dtv_registers[9] = GLOBAL_REGS.r9; \
      dtv_registers[10] = GLOBAL_REGS.r10; \
      dtv_registers[11] = GLOBAL_REGS.r11; \
      dtv_registers[12] = GLOBAL_REGS.r12; \
      dtv_registers[13] = GLOBAL_REGS.r13; \
      dtv_registers[14] = GLOBAL_REGS.r14; \
      dtv_registers[15] = GLOBAL_REGS.r15; \
      reg_a_write_idx = GLOBAL_REGS.acm >> 4; \
      reg_a_read_idx = GLOBAL_REGS.acm & 0xf; \
      reg_y_idx = GLOBAL_REGS.yxm >> 4; \
      reg_x_idx = GLOBAL_REGS.yxm & 0xf; \
      JUMP(GLOBAL_REGS.pc); \
  } while (0)

#endif /* C64DTV */
#else  /* DRIVE_CPU */
#define IMPORT_REGISTERS()
#define EXPORT_REGISTERS()
#endif /* !DRIVE_CPU */

/* Stack operations. */

#ifndef PUSH
#define PUSH(val) ((PAGE_ONE)[(reg_sp--)] = ((BYTE)(val)))
#endif
#ifndef PULL
#define PULL()    ((PAGE_ONE)[(++reg_sp)])
#endif

#ifdef DEBUG
#define TRACE_NMI() \
    do { if (TRACEFLG) debug_nmi(CPU_INT_STATUS, CLK); } while (0)
#define TRACE_IRQ() \
    do { if (TRACEFLG) debug_irq(CPU_INT_STATUS, CLK); } while (0)
#define TRACE_BRK() do { if (TRACEFLG) debug_text("*** BRK"); } while (0)
#else
#define TRACE_NMI()
#define TRACE_IRQ()
#define TRACE_BRK()
#endif

/* Perform the interrupts in `int_kind'.  If we have both NMI and IRQ,
   execute NMI.  */
/* FIXME: Dummy LOAD() cycles are missing!  */
/* FIXME: Improper BRK handling!  */
/* FIXME: LOCAL_STATUS() should check byte ready first.  */
#define DO_INTERRUPT(int_kind)                                        \
    do {                                                              \
        BYTE ik = (int_kind);                                         \
                                                                      \
        if (ik & (IK_IRQ | IK_IRQPEND | IK_NMI)) {                    \
            if ((ik & IK_NMI)                                         \
                && interrupt_check_nmi_delay(CPU_INT_STATUS, CLK)) {  \
                TRACE_NMI();                                          \
                if (monitor_mask[CALLER] & (MI_STEP)) {               \
                    monitor_check_icount_interrupt();                 \
                }                                                     \
                interrupt_ack_nmi(CPU_INT_STATUS);                    \
                LOCAL_SET_BREAK(0);                                   \
                PUSH(reg_pc >> 8);                                    \
                PUSH(reg_pc & 0xff);                                  \
                PUSH(LOCAL_STATUS());                                 \
                LOCAL_SET_INTERRUPT(1);                               \
                JUMP(LOAD_ADDR(0xfffa));                              \
                SET_LAST_OPCODE(0);                                   \
                CLK_ADD(CLK,NMI_CYCLES);                              \
            } else if ((ik & (IK_IRQ | IK_IRQPEND))                   \
                       && (!LOCAL_INTERRUPT()                         \
                       || OPINFO_DISABLES_IRQ(LAST_OPCODE_INFO))      \
                       && interrupt_check_irq_delay(CPU_INT_STATUS,   \
                                                    CLK)) {           \
                TRACE_IRQ();                                          \
                if (monitor_mask[CALLER] & (MI_STEP)) {               \
                    monitor_check_icount_interrupt();                 \
                }                                                     \
                interrupt_ack_irq(CPU_INT_STATUS);                    \
                LOCAL_SET_BREAK(0);                                   \
                PUSH(reg_pc >> 8);                                    \
                PUSH(reg_pc & 0xff);                                  \
                PUSH(LOCAL_STATUS());                                 \
                LOCAL_SET_INTERRUPT(1);                               \
                JUMP(LOAD_ADDR(0xfffe));                              \
                SET_LAST_OPCODE(0);                                   \
                CLK_ADD(CLK,IRQ_CYCLES);                              \
            }                                                         \
        }                                                             \
        if (ik & (IK_TRAP | IK_RESET)) {                              \
            if (ik & IK_TRAP) {                                       \
                EXPORT_REGISTERS();                                   \
                interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);      \
                IMPORT_REGISTERS();                                   \
                if (CPU_INT_STATUS->global_pending_int & IK_RESET)    \
                    ik |= IK_RESET;                                   \
            }                                                         \
            if (ik & IK_RESET) {                                      \
                interrupt_ack_reset(CPU_INT_STATUS);                  \
                cpu_reset();                                          \
                JUMP(LOAD_ADDR(0xfffc));                              \
                DMA_ON_RESET;                                         \
            }                                                         \
        }                                                             \
        if (ik & (IK_MONITOR | IK_DMA)) {                             \
            if (ik & IK_MONITOR) {                                    \
                if (monitor_force_import(CALLER))                     \
                    IMPORT_REGISTERS();                               \
                if (monitor_mask[CALLER])                             \
                    EXPORT_REGISTERS();                               \
                if (monitor_mask[CALLER] & (MI_BREAK)) {              \
                    if (monitor_check_breakpoints(CALLER,             \
                        (WORD)reg_pc)) {                              \
                        monitor_startup(CALLER);                      \
                        IMPORT_REGISTERS();                           \
                    }                                                 \
                }                                                     \
                if (monitor_mask[CALLER] & (MI_STEP)) {               \
                    monitor_check_icount((WORD)reg_pc);               \
                    IMPORT_REGISTERS();                               \
                }                                                     \
                if (monitor_mask[CALLER] & (MI_WATCH)) {              \
                    monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)reg_pc); \
                    IMPORT_REGISTERS();                               \
                }                                                     \
            }                                                         \
            if (ik & IK_DMA) {                                        \
                EXPORT_REGISTERS();                                   \
                DMA_FUNC;                                             \
                interrupt_ack_dma(CPU_INT_STATUS);                    \
                IMPORT_REGISTERS();                                   \
                JUMP(reg_pc);                                         \
            }                                                         \
        }                                                             \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Addressing modes.  For convenience, page boundary crossing cycles and
   ``idle'' memory reads are handled here as well. */

#define LOAD_ABS(addr)  \
   LOAD(addr)

#define LOAD_ABS_X(addr)                                     \
   ((((addr) & 0xff) + reg_x) > 0xff                         \
    ? (LOAD(((addr) & 0xff00) | (((addr) + reg_x) & 0xff)),  \
       CLK_ADD(CLK,CLK_INT_CYCLE),                           \
       LOAD((addr) + reg_x))                                 \
    : LOAD((addr) + reg_x))

#define LOAD_ABS_X_RMW(addr)                              \
   (LOAD(((addr) & 0xff00) | (((addr) + reg_x) & 0xff)),  \
    CLK_ADD(CLK,CLK_INT_CYCLE),                           \
    LOAD((addr) + reg_x))

#define LOAD_ABS_Y(addr)                                     \
   ((((addr) & 0xff) + reg_y) > 0xff                         \
    ? (LOAD(((addr) & 0xff00) | (((addr) + reg_y) & 0xff)),  \
       CLK_ADD(CLK,CLK_INT_CYCLE),                           \
       LOAD((addr) + reg_y))                                 \
    : LOAD((addr) + reg_y))

#define LOAD_ABS_Y_RMW(addr)                              \
   (LOAD(((addr) & 0xff00) | (((addr) + reg_y) & 0xff)),  \
    CLK_ADD(CLK,CLK_INT_CYCLE),                           \
    LOAD((addr) + reg_y))

#define LOAD_IND_X(addr)  \
   (CLK_ADD(CLK, 3), LOAD(LOAD_ZERO_ADDR((addr) + reg_x)))

#define LOAD_IND_Y(addr)                                               \
   (CLK_ADD(CLK, 2), ((LOAD_ZERO_ADDR((addr)) & 0xff) + reg_y) > 0xff  \
    ? (LOAD((LOAD_ZERO_ADDR((addr)) & 0xff00)                          \
            | ((LOAD_ZERO_ADDR((addr)) + reg_y) & 0xff)),              \
       CLK_ADD(CLK,CLK_INT_CYCLE),                                     \
       LOAD(LOAD_ZERO_ADDR((addr)) + reg_y))                           \
    : LOAD(LOAD_ZERO_ADDR((addr)) + reg_y))

#define LOAD_ZERO_X(addr)  \
   (LOAD_ZERO((addr) + reg_x))

#define LOAD_ZERO_Y(addr)  \
   (LOAD_ZERO((addr) + reg_y))

#define LOAD_IND_Y_BANK(addr)                                          \
   (CLK_ADD(CLK, 2), ((LOAD_ZERO_ADDR((addr)) & 0xff) + reg_y) > 0xff  \
    ? (LOAD_IND((LOAD_ZERO_ADDR((addr)) & 0xff00)                      \
            | ((LOAD_ZERO_ADDR((addr)) + reg_y) & 0xff)),              \
       CLK_ADD(CLK,CLK_INT_CYCLE),                                     \
       LOAD_IND(LOAD_ZERO_ADDR((addr)) + reg_y))                       \
    : LOAD_IND(LOAD_ZERO_ADDR((addr)) + reg_y))

#define STORE_ABS(addr, value, inc)  \
  do {                               \
      CLK_ADD(CLK,(inc));            \
      STORE((addr), (value));        \
  } while (0)

#define STORE_ABS_X(addr, value, inc)                       \
  do {                                                      \
      CLK_ADD(CLK,(inc)-2);                                 \
      LOAD((((addr) + reg_x) & 0xff) | ((addr) & 0xff00));  \
      CLK_ADD(CLK,2);                                       \
      STORE((addr) + reg_x, (value));                       \
  } while (0)

#define STORE_ABS_X_RMW(addr, value, inc)  \
  do {                                     \
      CLK_ADD(CLK,(inc));                  \
      STORE((addr) + reg_x, (value));      \
  } while (0)                              \

#define STORE_ABS_SH_X(addr, value, inc)                    \
  do {                                                      \
      unsigned int tmp2;                                    \
                                                            \
      CLK_ADD(CLK,(inc)-2);                                 \
      LOAD((((addr) + reg_x) & 0xff) | ((addr) & 0xff00));  \
      CLK_ADD(CLK,2);                                       \
      tmp2 = (addr) + reg_x;                                \
      if (((addr) & 0xff) + reg_x > 0xff)                   \
          tmp2 = (tmp2 & 0xff) | ((value) << 8);            \
      STORE(tmp2, (value));                                 \
  } while (0)

#define STORE_ABS_Y(addr, value, inc)                       \
  do {                                                      \
      CLK_ADD(CLK,(inc)-2);                                 \
      LOAD((((addr) + reg_y) & 0xff) | ((addr) & 0xff00));  \
      CLK_ADD(CLK,2);                                       \
      STORE((addr) + reg_y, (value));                       \
  } while (0)

#define STORE_ABS_Y_RMW(addr, value, inc)  \
  do {                                     \
      CLK_ADD(CLK,(inc));                  \
      STORE((addr) + reg_y, (value));      \
  } while (0)

#define STORE_ABS_SH_Y(addr, value, inc)                    \
  do {                                                      \
      unsigned int tmp2;                                    \
                                                            \
      CLK_ADD(CLK,(inc)-2);                                 \
      LOAD((((addr) + reg_y) & 0xff) | ((addr) & 0xff00));  \
      CLK_ADD(CLK,2);                                       \
      tmp2 = (addr) + reg_y;                                \
      if (((addr) & 0xff) + reg_y > 0xff)                   \
          tmp2 = (tmp2 & 0xff) | ((value) << 8);            \
      STORE(tmp2, (value));                                 \
  } while (0)

#define INC_PC(value)   (reg_pc += (value))

/* ------------------------------------------------------------------------- */

/* Opcodes.  */

/*
   A couple of caveats about PC:

   - the VIC-II emulation requires PC to be incremented before the first
     write access (this is not (very) important when writing to the zero
     page);

   - `p0', `p1' and `p2' can only be used *before* incrementing PC: some
     machines (eg. the C128) might depend on this.
*/

#define ADC(value, clk_inc, pc_inc)                                           \
  do {                                                                        \
      unsigned int tmp_value;                                                 \
      unsigned int tmp;                                                       \
                                                                              \
      tmp_value = (value);                                                    \
      CLK_ADD(CLK, (clk_inc));                                                \
                                                                              \
      if (LOCAL_DECIMAL()) {                                                  \
          tmp = (reg_a_read & 0xf) + (tmp_value & 0xf) + (reg_p & 0x1);            \
          if (tmp > 0x9)                                                      \
              tmp += 0x6;                                                     \
          if (tmp <= 0x0f)                                                    \
              tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0);        \
          else                                                                \
              tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0) + 0x10; \
          LOCAL_SET_ZERO(!((reg_a_read + tmp_value + (reg_p & 0x1)) & 0xff));      \
          LOCAL_SET_SIGN(tmp & 0x80);                                         \
          LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80)                           \
                              && !((reg_a_read ^ tmp_value) & 0x80));              \
          if ((tmp & 0x1f0) > 0x90)                                           \
              tmp += 0x60;                                                    \
          LOCAL_SET_CARRY((tmp & 0xff0) > 0xf0);                              \
      } else {                                                                \
          tmp = tmp_value + reg_a_read + (reg_p & P_CARRY);                        \
          LOCAL_SET_NZ(tmp & 0xff);                                           \
          LOCAL_SET_OVERFLOW(!((reg_a_read ^ tmp_value) & 0x80)                    \
                              && ((reg_a_read ^ tmp) & 0x80));                     \
          LOCAL_SET_CARRY(tmp > 0xff);                                        \
      }                                                                       \
      reg_a_write = tmp;                                                            \
      INC_PC(pc_inc);                                                         \
  } while (0)

#define ANC(value, pc_inc)                        \
  do {                                            \
      reg_a_write = (BYTE)(reg_a_read & (value)); \
      LOCAL_SET_NZ(reg_a_read);                   \
      LOCAL_SET_CARRY(LOCAL_SIGN());              \
      INC_PC(pc_inc);                             \
  } while (0)

#define AND(value, clk_inc, pc_inc)               \
  do {                                            \
      reg_a_write = (BYTE)(reg_a_read & (value)); \
      LOCAL_SET_NZ(reg_a_read);                   \
      CLK_ADD(CLK, (clk_inc));                    \
      INC_PC(pc_inc);                             \
  } while (0)

/*
The result of the ANE opcode is A = ((A | CONST) & X & IMM), with CONST apparently
being both chip- and temperature dependent.

The commonly used value for CONST in various documents is 0xee, which is however
not to be taken for granted (as it is unstable). see here:
http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)

as seen in the list, there are several possible values, and its origin is still
kinda unknown. instead of the commonly used 0xee we use 0xff here, since this
will make the only known occurance of this opcode in actual code work. see here:
https://sourceforge.net/tracker/?func=detail&aid=2110948&group_id=223021&atid=1057617

FIXME: in the unlikely event that other code surfaces that depends on another
CONST value, it probably has to be made configureable somehow if no value can
be found that works for both.
*/

#define ANE(value, pc_inc)                                 \
  do {                                                     \
      reg_a_write = ((reg_a_read | 0xff) & reg_x & ((BYTE)(value)));  \
      LOCAL_SET_NZ(reg_a_read);                            \
      INC_PC(pc_inc);                                      \
  } while (0)

/* The fanciest opcode ever... ARR! */
#define ARR(value, pc_inc)                                         \
  do {                                                             \
      unsigned int tmp;                                            \
                                                                   \
      tmp = reg_a_read & (value);                                  \
      if (LOCAL_DECIMAL()) {                                       \
          int tmp_2 = tmp;                                         \
          tmp_2 |= (reg_p & P_CARRY) << 8;                         \
          tmp_2 >>= 1;                                             \
          LOCAL_SET_SIGN(LOCAL_CARRY());                           \
          LOCAL_SET_ZERO(!tmp_2);                                  \
          LOCAL_SET_OVERFLOW((tmp_2 ^ tmp) & 0x40);                \
          if (((tmp & 0xf) + (tmp & 0x1)) > 0x5)                   \
              tmp_2 = (tmp_2 & 0xf0) | ((tmp_2 + 0x6) & 0xf);      \
          if (((tmp & 0xf0) + (tmp & 0x10)) > 0x50) {              \
              tmp_2 = (tmp_2 & 0x0f) | ((tmp_2 + 0x60) & 0xf0);    \
              LOCAL_SET_CARRY(1);                                  \
          } else                                                   \
              LOCAL_SET_CARRY(0);                                  \
          reg_a_write = tmp_2;                                     \
      } else {                                                     \
          tmp |= (reg_p & P_CARRY) << 8;                           \
          tmp >>= 1;                                               \
          LOCAL_SET_NZ(tmp);                                       \
          LOCAL_SET_CARRY(tmp & 0x40);                             \
          LOCAL_SET_OVERFLOW((tmp & 0x40) ^ ((tmp & 0x20) << 1));  \
          reg_a_write = tmp;                                       \
      }                                                            \
      INC_PC(pc_inc);                                              \
  } while (0)

#define ASL(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int tmp_value, tmp_addr;                    \
                                                           \
      tmp_addr = (addr);                                   \
      tmp_value = load_func(tmp_addr);                     \
      LOCAL_SET_CARRY(tmp_value & 0x80);                   \
      tmp_value = (tmp_value << 1) & 0xff;                 \
      LOCAL_SET_NZ(tmp_value);                             \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, tmp_value, clk_inc);            \
      RMW_FLAG = 0;                                        \
  } while (0)

#define ASL_A()                       \
  do {                                \
      LOCAL_SET_CARRY(reg_a_read & 0x80); \
      reg_a_write = reg_a_read << 1;  \
      LOCAL_SET_NZ(reg_a_read);       \
      INC_PC(1);                      \
  } while (0)

#define ASR(value, pc_inc)         \
  do {                             \
      unsigned int tmp;            \
                                   \
      tmp = reg_a_read & (value);  \
      LOCAL_SET_CARRY(tmp & 0x01); \
      reg_a_write = tmp >> 1;      \
      LOCAL_SET_NZ(reg_a_read);    \
      INC_PC(pc_inc);              \
  } while (0)

#define BIT(value, pc_inc)            \
  do {                                \
      unsigned int tmp;               \
                                      \
      tmp = (value);                  \
      CLK_ADD(CLK,1);                 \
      LOCAL_SET_SIGN(tmp & 0x80);     \
      LOCAL_SET_OVERFLOW(tmp & 0x40); \
      LOCAL_SET_ZERO(!(tmp & reg_a_read)); \
      INC_PC(pc_inc);                 \
  } while (0)

#ifndef C64DTV
#define BRANCH(cond, value)                                        \
  do {                                                             \
      INC_PC(2);                                                   \
                                                                   \
      if (cond) {                                                  \
          unsigned int dest_addr = reg_pc + (signed char)(value);  \
                                                                   \
          LOAD(reg_pc);                                            \
          CLK_ADD(CLK,CLK_BRANCH2);                                \
          if ((reg_pc ^ dest_addr) & 0xff00) {                     \
              LOAD((reg_pc & 0xff00) | (dest_addr & 0xff));        \
              CLK_ADD(CLK,CLK_BRANCH2);                            \
          } else {                                                 \
              OPCODE_DELAYS_INTERRUPT();                           \
          }                                                        \
          JUMP(dest_addr & 0xffff);                                \
      }                                                            \
  } while (0)
#endif

#define BRK()                                                    \
  do {                                                           \
      EXPORT_REGISTERS();                                        \
      CLK_ADD(CLK,CLK_BRK);                                      \
      TRACE_BRK();                                               \
      INC_PC(2);                                                 \
      LOCAL_SET_BREAK(1);                                        \
      PUSH(reg_pc >> 8);                                         \
      PUSH(reg_pc & 0xff);                                       \
      PUSH(LOCAL_STATUS());                                      \
      LOCAL_SET_INTERRUPT(1);                                    \
      JUMP(LOAD_ADDR(0xfffe));                                   \
  } while (0)

#define CLC()              \
  do {                     \
      INC_PC(1);           \
      LOCAL_SET_CARRY(0);  \
  } while (0)

#define CLD()                \
  do {                       \
      INC_PC(1);             \
      LOCAL_SET_DECIMAL(0);  \
  } while (0)

#define CLI()                    \
  do {                           \
      INC_PC(1);                 \
      if (LOCAL_INTERRUPT())     \
          OPCODE_ENABLES_IRQ();  \
      LOCAL_SET_INTERRUPT(0);    \
  } while (0)

#define CLV()                 \
  do {                        \
      INC_PC(1);              \
      LOCAL_SET_OVERFLOW(0);  \
  } while (0)

#define CMP(value, clk_inc, pc_inc)  \
  do {                               \
      unsigned int tmp;              \
                                     \
      tmp = reg_a_read - (value);    \
      LOCAL_SET_CARRY(tmp < 0x100);  \
      LOCAL_SET_NZ(tmp & 0xff);      \
      CLK_ADD(CLK, (clk_inc));       \
      INC_PC(pc_inc);                \
  } while (0)

#define CPX(value, clk_inc, pc_inc)  \
  do {                               \
      unsigned int tmp;              \
                                     \
      tmp = reg_x - (value);         \
      LOCAL_SET_CARRY(tmp < 0x100);  \
      LOCAL_SET_NZ(tmp & 0xff);      \
      CLK_ADD(CLK,(clk_inc));        \
      INC_PC(pc_inc);                \
  } while (0)

#define CPY(value, clk_inc, pc_inc)  \
  do {                               \
      unsigned int tmp;              \
                                     \
      tmp = reg_y - (value);         \
      LOCAL_SET_CARRY(tmp < 0x100);  \
      LOCAL_SET_NZ(tmp & 0xff);      \
      CLK_ADD(CLK,(clk_inc));        \
      INC_PC(pc_inc);                \
  } while (0)

#define DCP(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      unsigned int tmp, tmp_addr;                                     \
                                                                      \
      tmp_addr = (addr);                                              \
      CLK_ADD(CLK,(clk_inc1));                                        \
      tmp = load_func(tmp_addr);                                      \
      tmp = (tmp - 1) & 0xff;                                         \
      LOCAL_SET_CARRY(reg_a_read >= tmp);                             \
      LOCAL_SET_NZ((reg_a_read - tmp));                               \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      store_func(tmp_addr, tmp, (clk_inc2));                          \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define DCP_IND_Y(addr)                                         \
  do {                                                          \
      unsigned int tmp;                                         \
      unsigned int tmp_addr = LOAD_ZERO_ADDR(addr);             \
                                                                \
      CLK_ADD(CLK,2);                                           \
      LOAD((tmp_addr & 0xff00) | ((tmp_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                              \
      tmp_addr += reg_y;                                        \
      tmp = LOAD(tmp_addr);                                     \
      tmp = (tmp - 1) & 0xff;                                   \
      LOCAL_SET_CARRY(reg_a_read >= tmp);                       \
      LOCAL_SET_NZ((reg_a_read - tmp));                         \
      RMW_FLAG = 1;                                             \
      INC_PC(2);                                                \
      STORE_ABS(tmp_addr, tmp, CLK_IND_Y_RMW2);                 \
      RMW_FLAG = 0;                                             \
  } while (0)

#define DEC(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int tmp, tmp_addr;                          \
                                                           \
      tmp_addr = (addr);                                   \
      tmp = load_func(tmp_addr);                           \
      tmp = (tmp - 1) & 0xff;                              \
      LOCAL_SET_NZ(tmp);                                   \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, tmp, (clk_inc));                \
      RMW_FLAG = 0;                                        \
  } while (0)

#define DEX()               \
  do {                      \
      reg_x--;              \
      LOCAL_SET_NZ(reg_x);  \
      INC_PC(1);            \
  } while (0)

#define DEY()               \
  do {                      \
      reg_y--;              \
      LOCAL_SET_NZ(reg_y);  \
      INC_PC(1);            \
  } while (0)

#define EOR(value, clk_inc, pc_inc)               \
  do {                                            \
      reg_a_write = (BYTE)(reg_a_read ^ (value)); \
      LOCAL_SET_NZ(reg_a_read);                   \
      CLK_ADD(CLK, (clk_inc));                    \
      INC_PC(pc_inc);                             \
  } while (0)

#define INC(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int tmp, tmp_addr;                          \
                                                           \
      tmp_addr = (addr);                                   \
      tmp = (load_func(tmp_addr) + 1) & 0xff;              \
      LOCAL_SET_NZ(tmp);                                   \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, tmp, (clk_inc));                \
      RMW_FLAG = 0;                                        \
  } while (0)

#define INX()               \
  do {                      \
      reg_x++;              \
      LOCAL_SET_NZ(reg_x);  \
      INC_PC(1);            \
  } while (0)

#define INY()               \
  do {                      \
      reg_y++;              \
      LOCAL_SET_NZ(reg_y);  \
      INC_PC(1);            \
  } while (0)

#define ISB(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      BYTE my_src;                                                    \
      int my_addr = (addr);                                           \
                                                                      \
      CLK_ADD(CLK,(clk_inc1));                                        \
      my_src = load_func(my_addr);                                    \
      my_src = (my_src + 1) & 0xff;                                   \
      SBC(my_src, 0, 0);                                              \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      store_func(my_addr, my_src, clk_inc2);                          \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define ISB_IND_Y(addr)                                       \
  do {                                                        \
      BYTE my_src;                                            \
      int my_addr = LOAD_ZERO_ADDR(addr);                     \
                                                              \
      CLK_ADD(CLK,2);                                         \
      LOAD((my_addr & 0xff00) | ((my_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                            \
      my_addr += reg_y;                                       \
      my_src = LOAD(my_addr);                                 \
      my_src = (my_src + 1) & 0xff;                           \
      SBC(my_src, 0, 0);                                      \
      RMW_FLAG = 1;                                           \
      INC_PC(2);                                              \
      STORE_ABS(my_addr, my_src, CLK_IND_Y_RMW2);             \
      RMW_FLAG = 0;                                           \
  } while (0)

/* The 0x02 JAM opcode is also used to patch the ROM.  The function trap_handler()
   returns nonzero if this is not a patch, but a `real' JAM instruction. */

#define JAM_02()                                                 \
  do {                                                           \
      DWORD trap_result;                                         \
      EXPORT_REGISTERS();                                        \
      if (!ROM_TRAP_ALLOWED()                                    \
          || (trap_result = ROM_TRAP_HANDLER()) == (DWORD)-1) {  \
          REWIND_FETCH_OPCODE(CLK);                              \
          JAM();                                                 \
      } else {                                                   \
          if (trap_result) {                                     \
             REWIND_FETCH_OPCODE(CLK);                           \
             SET_OPCODE(trap_result);                            \
             IMPORT_REGISTERS();                                 \
             goto trap_skipped;                                  \
          } else {                                               \
             IMPORT_REGISTERS();                                 \
          }                                                      \
      }                                                          \
  } while (0)

#define JMP(addr)     \
  do {                \
      JUMP(addr);     \
  } while (0)

#define JMP_IND()                                                   \
  do {                                                              \
      WORD dest_addr;                                               \
      dest_addr = LOAD(p2);                                         \
      CLK_ADD(CLK,1);                                               \
      dest_addr |= (LOAD((p2 & 0xff00) | ((p2 + 1) & 0xff)) << 8);  \
      CLK_ADD(CLK,1);                                               \
      JUMP(dest_addr);                                              \
  } while (0)

#define JSR()                                 \
  do {                                        \
      unsigned int tmp_addr;                  \
                                              \
      CLK_ADD(CLK,1);                         \
      INC_PC(2);                              \
      CLK_ADD(CLK,2);                         \
      PUSH(((reg_pc) >> 8) & 0xff);           \
      PUSH((reg_pc) & 0xff);                  \
      tmp_addr = (p1 | (LOAD(reg_pc) << 8));  \
      CLK_ADD(CLK,CLK_JSR_INT_CYCLE);         \
      JUMP(tmp_addr);                         \
  } while (0)

#define LAS(value, clk_inc, pc_inc)               \
  do {                                            \
      reg_a_write = reg_x = reg_sp = reg_sp & (value);  \
      LOCAL_SET_NZ(reg_a_read);                   \
      CLK_ADD(CLK,(clk_inc));                     \
      INC_PC(pc_inc);                             \
  } while (0)

#define LAX(value, clk_inc, pc_inc)  \
  do {                               \
      reg_a_write = reg_x = (value); \
      LOCAL_SET_NZ(reg_a_read);      \
      CLK_ADD(CLK, (clk_inc));       \
      INC_PC(pc_inc);                \
  } while (0)

#define LDA(value, clk_inc, pc_inc)  \
  do {                               \
      reg_a_write = (BYTE)(value);   \
      CLK_ADD(CLK, (clk_inc));       \
      LOCAL_SET_NZ(reg_a_read);      \
      INC_PC(pc_inc);                \
  } while (0)

#define LDX(value, clk_inc, pc_inc)  \
  do {                               \
      reg_x = (BYTE)(value);         \
      LOCAL_SET_NZ(reg_x);           \
      CLK_ADD(CLK,(clk_inc));        \
      INC_PC(pc_inc);                \
  } while (0)

#define LDY(value, clk_inc, pc_inc)  \
  do {                               \
      reg_y = (BYTE)(value);         \
      LOCAL_SET_NZ(reg_y);           \
      CLK_ADD(CLK,(clk_inc));        \
      INC_PC(pc_inc);                \
  } while (0)

#define LSR(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int tmp, tmp_addr;                          \
                                                           \
      tmp_addr = (addr);                                   \
      tmp = load_func(tmp_addr);                           \
      LOCAL_SET_CARRY(tmp & 0x01);                         \
      tmp >>= 1;                                           \
      LOCAL_SET_NZ(tmp);                                   \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, tmp, clk_inc);                  \
      RMW_FLAG = 0;                                        \
  } while (0)

#define LSR_A()                       \
  do {                                \
      LOCAL_SET_CARRY(reg_a_read & 0x01); \
      reg_a_write = reg_a_read >> 1;  \
      LOCAL_SET_NZ(reg_a_read);       \
      INC_PC(1);                      \
  } while (0)

/* Note: this is not always exact, as this opcode can be quite unstable!
   Moreover, the behavior is different from the one described in 64doc. */
#define LXA(value, pc_inc)                                 \
  do {                                                     \
      reg_a_write = reg_x = ((reg_a_read | 0xee) & ((BYTE)(value)));  \
      LOCAL_SET_NZ(reg_a_read);                            \
      INC_PC(pc_inc);                                      \
  } while (0)

#define ORA(value, clk_inc, pc_inc)               \
  do {                                            \
      reg_a_write = (BYTE)(reg_a_read | (value)); \
      LOCAL_SET_NZ(reg_a_write);                  \
      CLK_ADD(CLK, (clk_inc));                    \
      INC_PC(pc_inc);                             \
  } while (0)

#define NOOP(clk_inc, pc_inc)  \
    (CLK_ADD(CLK,(clk_inc)), INC_PC(pc_inc))

#define NOOP_IMM(pc_inc) INC_PC(pc_inc)

#define NOOP_ABS()    \
  do {                \
      LOAD(p2);       \
      CLK_ADD(CLK,1); \
      INC_PC(3);      \
  } while (0)

#define NOOP_ABS_X()   \
  do {                 \
      LOAD_ABS_X(p2);  \
      CLK_ADD(CLK,1);  \
      INC_PC(3);       \
  } while (0)

#define NOP()  NOOP_IMM(1)

#define PHA()         \
  do {                \
      CLK_ADD(CLK,CLK_STACK_PUSH); \
      PUSH(reg_a_read); \
      INC_PC(1);      \
  } while (0)

#define PHP()                          \
  do {                                 \
      CLK_ADD(CLK,CLK_STACK_PUSH); \
      PUSH(LOCAL_STATUS() | P_BREAK);  \
      INC_PC(1);                       \
  } while (0)

#define PLA()               \
  do {                      \
      CLK_ADD(CLK,CLK_STACK_PULL); \
      reg_a_write = PULL(); \
      LOCAL_SET_NZ(reg_a_read); \
      INC_PC(1);            \
  } while (0)

/* FIXME: Rotate disk before executing LOCAL_SET_STATUS().  */
#define PLP()                                            \
  do {                                                   \
      BYTE s = PULL();                                   \
                                                         \
      if (!(s & P_INTERRUPT) && LOCAL_INTERRUPT())       \
          OPCODE_ENABLES_IRQ();                          \
      else if ((s & P_INTERRUPT) && !LOCAL_INTERRUPT())  \
          OPCODE_DISABLES_IRQ();                         \
      CLK_ADD(CLK,CLK_STACK_PULL);                       \
      LOCAL_SET_STATUS(s);                               \
      INC_PC(1);                                         \
  } while (0)

#define RLA(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      unsigned int tmp, tmp_addr;                                     \
                                                                      \
      tmp_addr = (addr);                                              \
      CLK_ADD(CLK,(clk_inc1));                                        \
      tmp = ((load_func(tmp_addr) << 1) | (reg_p & P_CARRY));         \
      LOCAL_SET_CARRY(tmp & 0x100);                                   \
      reg_a_write = reg_a_read & tmp;                                 \
      LOCAL_SET_NZ(reg_a_read);                                       \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      store_func(tmp_addr, tmp, clk_inc2);                            \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define RLA_IND_Y(addr)                                         \
  do {                                                          \
      unsigned int tmp;                                         \
      unsigned int tmp_addr = LOAD_ZERO_ADDR(addr);             \
                                                                \
      CLK_ADD(CLK,2);                                           \
      LOAD((tmp_addr & 0xff00) | ((tmp_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                              \
      tmp_addr += reg_y;                                        \
      tmp = ((LOAD(tmp_addr) << 1) | (reg_p & P_CARRY));        \
      LOCAL_SET_CARRY(tmp & 0x100);                             \
      reg_a_write = reg_a_read & tmp;                           \
      LOCAL_SET_NZ(reg_a_read);                                 \
      RMW_FLAG = 1;                                             \
      INC_PC(2);                                                \
      STORE_ABS(tmp_addr, tmp, CLK_IND_Y_RMW2);                 \
      RMW_FLAG = 0;                                             \
  } while (0)

#define ROL(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int tmp, tmp_addr;                          \
                                                           \
      tmp_addr = (addr);                                   \
      tmp = load_func(tmp_addr);                           \
      tmp = (tmp << 1) | (reg_p & P_CARRY);                \
      LOCAL_SET_CARRY(tmp & 0x100);                        \
      LOCAL_SET_NZ(tmp & 0xff);                            \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, tmp, clk_inc);                  \
      RMW_FLAG = 0;                                        \
  } while (0)

#define ROL_A()                         \
  do {                                  \
      unsigned int tmp = reg_a_read << 1; \
                                        \
      reg_a_write = tmp | (reg_p & P_CARRY);  \
      LOCAL_SET_CARRY(tmp & 0x100);     \
      LOCAL_SET_NZ(reg_a_read);         \
      INC_PC(1);                        \
  } while (0)

#define ROR(addr, clk_inc, pc_inc, load_func, store_func)  \
  do {                                                     \
      unsigned int src, tmp_addr;                          \
                                                           \
      tmp_addr = (addr);                                   \
      src = load_func(tmp_addr);                           \
      if (reg_p & P_CARRY)                                 \
          src |= 0x100;                                    \
      LOCAL_SET_CARRY(src & 0x01);                         \
      src >>= 1;                                           \
      LOCAL_SET_NZ(src);                                   \
      RMW_FLAG = 1;                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp_addr, src, (clk_inc));                \
      RMW_FLAG = 0;                                        \
  } while (0)

#define ROR_A()                             \
  do {                                      \
      BYTE tmp = reg_a_read;                \
                                            \
      reg_a_write = (reg_a_read >> 1) | (reg_p << 7);  \
      LOCAL_SET_CARRY(tmp & 0x01);          \
      LOCAL_SET_NZ(reg_a_read);             \
      INC_PC(1);                            \
  } while (0)

#define RRA(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      BYTE src;                                                       \
      unsigned int my_temp, tmp_addr;                                 \
                                                                      \
      CLK_ADD(CLK,(clk_inc1));                                        \
      tmp_addr = (addr);                                              \
      src = load_func(tmp_addr);                                      \
      my_temp = src >> 1;                                             \
      if (reg_p & P_CARRY)                                            \
          my_temp |= 0x80;                                            \
      LOCAL_SET_CARRY(src & 0x1);                                     \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      ADC(my_temp, 0, 0);                                             \
      store_func(tmp_addr, my_temp, clk_inc2);                        \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define RRA_IND_Y(addr)                                               \
  do {                                                                \
      BYTE src;                                                       \
      unsigned int my_tmp_addr;                                       \
      unsigned int my_temp;                                           \
                                                                      \
      CLK_ADD(CLK,2);                                                 \
      my_tmp_addr = LOAD_ZERO_ADDR(addr);                             \
      LOAD((my_tmp_addr & 0xff00) | ((my_tmp_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                                    \
      my_tmp_addr += reg_y;                                           \
      src = LOAD(my_tmp_addr);                                        \
      RMW_FLAG = 1;                                                   \
      INC_PC(2);                                                      \
      my_temp = src >> 1;                                             \
      if (reg_p & P_CARRY)                                            \
          my_temp |= 0x80;                                            \
      LOCAL_SET_CARRY(src & 0x1);                                     \
      ADC(my_temp, 0, 0);                                             \
      STORE_ABS(my_tmp_addr, my_temp, CLK_IND_Y_RMW2);                \
      RMW_FLAG = 0;                                                   \
  } while (0)

/* RTI does must not use `OPCODE_ENABLES_IRQ()' even if the I flag changes
   from 1 to 0 because the value of I is set 3 cycles before the end of the
   opcode, and thus the 6510 has enough time to call the interrupt routine as
   soon as the opcode ends, if necessary.  */
/* FIXME: Rotate disk before executing LOCAL_SET_STATUS().  */
#define RTI()                       \
  do {                              \
      WORD tmp;                     \
                                    \
      CLK_ADD(CLK,CLK_RTI);         \
      tmp = (WORD)PULL();           \
      LOCAL_SET_STATUS((BYTE)tmp);  \
      tmp = (WORD)PULL();           \
      tmp |= (WORD)PULL() << 8;     \
      JUMP(tmp);                    \
  } while (0)

#define RTS()                     \
  do {                            \
      WORD tmp;                   \
                                  \
      CLK_ADD(CLK,CLK_RTS);       \
      tmp = PULL();               \
      tmp = tmp | (PULL() << 8);  \
      LOAD(tmp);                  \
      CLK_ADD(CLK,CLK_INT_CYCLE); \
      tmp++;                      \
      JUMP(tmp);                  \
  } while (0)

#define SAX(addr, clk_inc1, clk_inc2, pc_inc)  \
  do {                                         \
      unsigned int tmp;                        \
                                               \
      CLK_ADD(CLK,(clk_inc1));                 \
      tmp = (addr);                            \
      CLK_ADD(CLK,(clk_inc2));                 \
      INC_PC(pc_inc);                          \
      STORE(tmp, reg_a_read & reg_x);          \
  } while (0)

#define SAX_ZERO(addr, clk_inc, pc_inc)   \
  do {                                    \
      CLK_ADD(CLK,(clk_inc));             \
      STORE_ZERO((addr), reg_a_read & reg_x);  \
      INC_PC(pc_inc);                     \
  } while (0)

#define SBC(value, clk_inc, pc_inc)                                          \
  do {                                                                       \
      WORD src, tmp;                                                         \
                                                                             \
      src = (WORD)(value);                                                   \
      CLK_ADD(CLK, (clk_inc));                                               \
      tmp = reg_a_read - src - ((reg_p & P_CARRY) ? 0 : 1);                  \
      if (reg_p & P_DECIMAL) {                                               \
          unsigned int tmp_a;                                                \
          tmp_a = (reg_a_read & 0xf) - (src & 0xf) - ((reg_p & P_CARRY) ? 0 : 1); \
          if (tmp_a & 0x10)                                                  \
              tmp_a = ((tmp_a - 6) & 0xf)                                    \
                       | ((reg_a_read & 0xf0) - (src & 0xf0) - 0x10);        \
          else                                                               \
              tmp_a = (tmp_a & 0xf) | ((reg_a_read & 0xf0) - (src & 0xf0));  \
          if (tmp_a & 0x100)                                                 \
              tmp_a -= 0x60;                                                 \
          LOCAL_SET_CARRY(tmp < 0x100);                                      \
          LOCAL_SET_NZ(tmp & 0xff);                                          \
          LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80)                     \
                             && ((reg_a_read ^ src) & 0x80));                \
          reg_a_write = (BYTE) tmp_a;                                        \
      } else {                                                               \
          LOCAL_SET_NZ(tmp & 0xff);                                          \
          LOCAL_SET_CARRY(tmp < 0x100);                                      \
          LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80)                     \
                             && ((reg_a_read ^ src) & 0x80));                \
          reg_a_write = (BYTE) tmp;                                          \
      }                                                                      \
      INC_PC(pc_inc);                                                        \
    }                                                                        \
  while (0)

#define SBX(value, pc_inc)          \
  do {                              \
      unsigned int tmp;             \
                                    \
      tmp = (value);                \
      INC_PC(pc_inc);               \
      tmp = (reg_a_read & reg_x) - tmp; \
      LOCAL_SET_CARRY(tmp < 0x100); \
      reg_x = tmp & 0xff;           \
      LOCAL_SET_NZ(reg_x);          \
  } while (0)

#undef SEC    /* defined in time.h on SunOS. */
#define SEC()              \
  do {                     \
      LOCAL_SET_CARRY(1);  \
      INC_PC(1);           \
  } while (0)

#define SED()                \
  do {                       \
      LOCAL_SET_DECIMAL(1);  \
      INC_PC(1);             \
  } while (0)

#define SEI()                     \
  do {                            \
      if (!LOCAL_INTERRUPT())     \
          OPCODE_DISABLES_IRQ();  \
      LOCAL_SET_INTERRUPT(1);     \
      INC_PC(1);                  \
  } while (0)

#define SHA_ABS_Y(addr)                                                   \
  do {                                                                    \
      unsigned int tmp;                                                   \
                                                                          \
      tmp = (addr);                                                       \
      INC_PC(3);                                                          \
      STORE_ABS_SH_Y(tmp, reg_a_read & reg_x & ((tmp >> 8) + 1), CLK_ABS_I_STORE2);  \
  } while (0)

#define SHA_IND_Y(addr)                               \
  do {                                                \
      unsigned int tmp;                               \
      BYTE val;                                       \
                                                      \
      CLK_ADD(CLK,2);                                 \
      tmp = LOAD_ZERO_ADDR(addr);                     \
      LOAD((tmp & 0xff00) | ((tmp + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_W);                       \
      val = reg_a_read & reg_x & ((tmp >> 8) + 1);    \
      if ((tmp & 0xff) + reg_y > 0xff)                \
          tmp = ((tmp + reg_y) & 0xff) | (val << 8);  \
      else                                            \
          tmp += reg_y;                               \
      INC_PC(2);                                      \
      STORE(tmp, val);                                \
  } while (0)

#define SHX_ABS_Y(addr)                                            \
  do {                                                             \
      unsigned int tmp;                                            \
                                                                   \
      tmp = (addr);                                                \
      INC_PC(3);                                                   \
      STORE_ABS_SH_Y(tmp, reg_x & ((tmp >> 8) + 1), CLK_ABS_I_STORE2); \
  } while (0)

#define SHY_ABS_X(addr)                                            \
  do {                                                             \
      unsigned int tmp;                                            \
                                                                   \
      tmp = (addr);                                                \
      INC_PC(3);                                                   \
      STORE_ABS_SH_X(tmp, reg_y & ((tmp >> 8) + 1), CLK_ABS_I_STORE2); \
  } while (0)

#define SHS_ABS_Y(addr)                                                    \
  do {                                                                     \
      int tmp = (addr);                                                    \
                                                                           \
      INC_PC(3);                                                           \
      STORE_ABS_SH_Y(tmp, reg_a_read & reg_x & ((tmp >> 8) + 1), CLK_ABS_I_STORE2); \
      reg_sp = reg_a_read & reg_x;                                         \
  } while (0)

#define SLO(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      BYTE tmp_value;                                                 \
      int tmp_addr;                                                   \
                                                                      \
      CLK_ADD(CLK,(clk_inc1));                                        \
      tmp_addr = (addr);                                              \
      tmp_value = load_func(tmp_addr);                                \
      LOCAL_SET_CARRY(tmp_value & 0x80);                              \
      tmp_value <<= 1;                                                \
      reg_a_write = reg_a_read | tmp_value;                           \
      LOCAL_SET_NZ(reg_a_read);                                       \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      store_func(tmp_addr, tmp_value, clk_inc2);                      \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define SLO_IND_Y(addr)                                         \
  do {                                                          \
      BYTE tmp_value;                                           \
      unsigned int tmp_addr;                                    \
                                                                \
      CLK_ADD(CLK,2);                                           \
      tmp_addr = LOAD_ZERO_ADDR(addr);                          \
      LOAD((tmp_addr & 0xff00) | ((tmp_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                              \
      tmp_addr += reg_y;                                        \
      tmp_value = LOAD(tmp_addr);                               \
      LOCAL_SET_CARRY(tmp_value & 0x80);                        \
      tmp_value <<= 1;                                          \
      reg_a_write = reg_a_read | tmp_value;                     \
      LOCAL_SET_NZ(reg_a_read);                                 \
      RMW_FLAG = 1;                                             \
      INC_PC(2);                                                \
      STORE_ABS(tmp_addr, tmp_value, CLK_IND_Y_RMW2);           \
      RMW_FLAG = 0;                                             \
  } while (0)

#define SRE(addr, clk_inc1, clk_inc2, pc_inc, load_func, store_func)  \
  do {                                                                \
      BYTE tmp;                                                       \
      unsigned int tmp_addr;                                          \
                                                                      \
      CLK_ADD(CLK,(clk_inc1));                                        \
      tmp_addr = (addr);                                              \
      tmp = load_func(tmp_addr);                                      \
      LOCAL_SET_CARRY(tmp & 0x01);                                    \
      tmp >>= 1;                                                      \
      reg_a_write = reg_a_read ^ tmp;                                 \
      LOCAL_SET_NZ(reg_a_read);                                       \
      RMW_FLAG = 1;                                                   \
      INC_PC(pc_inc);                                                 \
      store_func(tmp_addr, tmp, clk_inc2);                            \
      RMW_FLAG = 0;                                                   \
  } while (0)

#define SRE_IND_Y(addr)                                         \
  do {                                                          \
      BYTE tmp;                                                 \
      unsigned int tmp_addr = LOAD_ZERO_ADDR(addr);             \
                                                                \
      CLK_ADD(CLK,2);                                           \
      LOAD((tmp_addr & 0xff00) | ((tmp_addr + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_RMW1);                              \
      tmp_addr += reg_y;                                        \
      tmp = LOAD(tmp_addr);                                     \
      LOCAL_SET_CARRY(tmp & 0x01);                              \
      tmp >>= 1;                                                \
      reg_a_write = reg_a_read ^ tmp;                           \
      LOCAL_SET_NZ(reg_a_read);                                 \
      RMW_FLAG = 1;                                             \
      INC_PC(2);                                                \
      STORE_ABS(tmp_addr, tmp, CLK_IND_Y_RMW2);                 \
      RMW_FLAG = 0;                                             \
  } while (0)

#define STA(addr, clk_inc1, clk_inc2, pc_inc, store_func)  \
  do {                                                     \
      unsigned int tmp;                                    \
                                                           \
      CLK_ADD(CLK,(clk_inc1));                             \
      tmp = (addr);                                        \
      INC_PC(pc_inc);                                      \
      store_func(tmp, reg_a_read, clk_inc2);               \
  } while (0)

#define STA_ZERO(addr, clk_inc, pc_inc)  \
  do {                                   \
      CLK_ADD(CLK,(clk_inc));            \
      STORE_ZERO((addr), reg_a_read);    \
      INC_PC(pc_inc);                    \
  } while (0)

#define STA_IND_Y(addr)                                   \
  do {                                                    \
      unsigned int tmp;                                   \
                                                          \
      CLK_ADD(CLK,2);                                     \
      tmp = LOAD_ZERO_ADDR(addr);                         \
      LOAD_IND((tmp & 0xff00) | ((tmp + reg_y) & 0xff));  \
      CLK_ADD(CLK,CLK_IND_Y_W);                           \
      INC_PC(2);                                          \
      STORE_IND(tmp + reg_y, reg_a_read);                 \
  } while (0)

#define STX(addr, clk_inc, pc_inc)  \
  do {                              \
      unsigned int tmp;             \
                                    \
      tmp = (addr);                 \
      CLK_ADD(CLK,(clk_inc));       \
      INC_PC(pc_inc);               \
      STORE(tmp, reg_x);            \
  } while (0)

#define STX_ZERO(addr, clk_inc, pc_inc)  \
  do {                                   \
      CLK_ADD(CLK,(clk_inc));            \
      STORE_ZERO((addr), reg_x);         \
      INC_PC(pc_inc);                    \
  } while (0)

#define STY(addr, clk_inc, pc_inc)  \
  do {                              \
      unsigned int tmp;             \
                                    \
      tmp = (addr);                 \
      CLK_ADD(CLK,(clk_inc));       \
      INC_PC(pc_inc);               \
      STORE(tmp, reg_y);            \
  } while (0)

#define STY_ZERO(addr, clk_inc, pc_inc)  \
  do {                                   \
      CLK_ADD(CLK,(clk_inc));            \
      STORE_ZERO((addr), reg_y);         \
      INC_PC(pc_inc);                    \
  } while (0)

#define TAX()               \
  do {                      \
      reg_x = reg_a_read;   \
      LOCAL_SET_NZ(reg_a_read); \
      INC_PC(1);            \
  } while (0)

#define TAY()               \
  do {                      \
      reg_y = reg_a_read;   \
      LOCAL_SET_NZ(reg_a_read); \
      INC_PC(1);            \
  } while (0)

#define TSX()                \
  do {                       \
      reg_x = reg_sp;        \
      LOCAL_SET_NZ(reg_sp);  \
      INC_PC(1);             \
  } while (0)

#define TXA()               \
  do {                      \
      reg_a_write = reg_x;  \
      LOCAL_SET_NZ(reg_a_read); \
      INC_PC(1);            \
  } while (0)

#define TXS()          \
  do {                 \
      reg_sp = reg_x;  \
      INC_PC(1);       \
  } while (0)

#define TYA()               \
  do {                      \
      reg_a_write = reg_y;  \
      LOCAL_SET_NZ(reg_a_read); \
      INC_PC(1);            \
  } while (0)


/* ------------------------------------------------------------------------- */

static const BYTE fetch_tab[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $00 */
    /* $10 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $10 */
    /* $20 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $20 */
    /* $30 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $30 */
    /* $40 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $40 */
    /* $50 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $50 */
    /* $60 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $60 */
    /* $70 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $70 */
    /* $80 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
    /* $90 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
    /* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $A0 */
    /* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $B0 */
    /* $C0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $C0 */
    /* $D0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $D0 */
    /* $E0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $E0 */
    /* $F0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1  /* $F0 */
};

#ifndef C64DTV  /* C64DTV opcode_t & fetch are defined in c64dtvcpu.c */

#ifdef CPU_8502  /* 8502 specific opcode fetch */

static const BYTE rewind_fetch_tab[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $00 */
    /* $10 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $10 */
    /* $20 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $20 */
    /* $30 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $30 */
    /* $40 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $40 */
    /* $50 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $50 */
    /* $60 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $60 */
    /* $70 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $70 */
    /* $80 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $80 */
    /* $90 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $90 */
    /* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $A0 */
    /* $B0 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $B0 */
    /* $C0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $C0 */
    /* $D0 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $D0 */
    /* $E0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $E0 */
    /* $F0 */  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $F0 */
};

#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS

#define opcode_t DWORD

#define FETCH_OPCODE(o)                                                \
    do {                                                               \
        if (((int)reg_pc) < bank_limit) {                              \
            o = (*((DWORD *)(bank_base + reg_pc)) & 0xffffff);         \
            if (rewind_fetch_tab[o & 0xff])                            \
            {                                                          \
                opcode_cycle[0] = vicii_check_memory_refresh(CLK);     \
                CLK_ADD(CLK,1);                                        \
                opcode_cycle[1] = vicii_check_memory_refresh(CLK);     \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
            else                                                       \
            {                                                          \
                opcode_cycle[0] = 0;                                   \
                opcode_cycle[1] = 0;                                   \
                CLK_ADD(CLK,2);                                        \
            }                                                          \
            if (fetch_tab[o & 0xff]) {                                 \
                 CLK_ADD(CLK,1);                                       \
            }                                                          \
        } else {                                                       \
            maincpu_stretch = 0;                                       \
            o = LOAD(reg_pc);                                          \
            if (rewind_fetch_tab[o & 0xff])                            \
            {                                                          \
                opcode_cycle[0] = maincpu_stretch;                     \
                if (opcode_cycle[0] == 0)                              \
                    opcode_cycle[0] = vicii_check_memory_refresh(CLK); \
                CLK_ADD(CLK,1);                                        \
                maincpu_stretch = 0;                                   \
                o |= LOAD(reg_pc + 1) << 8;                            \
                opcode_cycle[1] = maincpu_stretch;                     \
                if (opcode_cycle[1] == 0)                              \
                    opcode_cycle[1] = vicii_check_memory_refresh(CLK); \
               CLK_ADD(CLK,1);                                         \
            }                                                          \
            else                                                       \
            {                                                          \
                CLK_ADD(CLK,1);                                        \
                o |= LOAD(reg_pc + 1) << 8;                            \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
            if (fetch_tab[o & 0xff]) {                                 \
                 o |= (LOAD(reg_pc + 2) << 16);                        \
                 CLK_ADD(CLK,1);                                       \
            }                                                          \
        }                                                              \
    } while (0)

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 (opcode >> 8)

#else /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

#define opcode_t          \
    struct {              \
        BYTE ins;         \
        union {           \
            BYTE op8[2];  \
            WORD op16;    \
        } op;             \
    }

#define FETCH_OPCODE(o)                                                \
    do {                                                               \
        if (((int)reg_pc) < bank_limit) {                              \
            (o).ins = *(bank_base + reg_pc);                           \
            (o).op.op16 = (*(bank_base + reg_pc + 1)                   \
                          | (*(bank_base + reg_pc + 2) << 8));         \
            if (rewind_fetch_tab[(o).ins])                             \
            {                                                          \
                opcode_cycle[0] = vicii_check_memory_refresh(CLK);     \
                CLK_ADD(CLK,1);                                        \
                opcode_cycle[1] = vicii_check_memory_refresh(CLK);     \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
            else                                                       \
            {                                                          \
                opcode_cycle[0] = 0;                                   \
                opcode_cycle[1] = 0;                                   \
                CLK_ADD(CLK,2);                                        \
            }                                                          \
            if (fetch_table[(o).ins)) {                                \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
        } else {                                                       \
            maincpu_stretch = 0;                                       \
            (o).ins = LOAD(reg_pc);                                    \
            if (rewind_fetch_tab[(o).ins])                             \
            {                                                          \
                opcode_cycle[0] = maincpu_stretch;                     \
                if (opcode_cycle[0] == 0)                              \
                    opcode_cycle[0] = vicii_check_memory_refresh(CLK); \
                CLK_ADD(CLK,1);                                        \
                maincpu_stretch = 0;                                   \
                (o).op.op16 = LOAD(reg_pc + 1);                        \
                opcode_cycle[1] = maincpu_stretch;                     \
                if (opcode_cycle[1] == 0)                              \
                    opcode_cycle[1] = vicii_check_memory_refresh(CLK); \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
            else                                                       \
            {                                                          \
                CLK_ADD(CLK,1);                                        \
                (o).op.op16 = LOAD(reg_pc + 1);                        \
                CLK_ADD(CLK,1);                                        \
            }                                                          \
            if (fetch_tab[(o).ins]) {                                  \
                 (o).op.op16 |= (LOAD(reg_pc + 2) << 8);               \
                 CLK_ADD(CLK,1);                                       \
            }                                                          \
        }                                                              \
    } while (0)

#define p0 (opcode.ins)
#define p2 (opcode.op.op16)

#ifdef WORDS_BIGENDIAN
#  define p1 (opcode.op.op8[1])
#else
#  define p1 (opcode.op.op8[0])
#endif

#endif /* !WORDS_BIGENDIAN */

#else /* !CPU_8502 */

#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS

#define opcode_t DWORD

#define FETCH_OPCODE(o)                                         \
    do {                                                        \
        if (((int)reg_pc) < bank_limit) {                       \
            o = (*((DWORD *)(bank_base + reg_pc)) & 0xffffff);  \
            CLK_ADD(CLK,2);                                     \
            if (fetch_tab[o & 0xff]) {                          \
                CLK_ADD(CLK,1);                                 \
            }                                                   \
        } else {                                                \
            o = LOAD(reg_pc);                                   \
            CLK_ADD(CLK,1);                                     \
            o |= LOAD(reg_pc + 1) << 8;                         \
            CLK_ADD(CLK,1);                                     \
            if (fetch_tab[o & 0xff]) {                          \
                 o |= (LOAD(reg_pc + 2) << 16);                 \
                 CLK_ADD(CLK,1);                                \
            }                                                   \
        }                                                       \
    } while (0)

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 (opcode >> 8)

#else /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

#define opcode_t          \
    struct {              \
        BYTE ins;         \
        union {           \
            BYTE op8[2];  \
            WORD op16;    \
        } op;             \
    }

#define FETCH_OPCODE(o)                                         \
    do {                                                        \
        if (((int)reg_pc) < bank_limit) {                       \
            (o).ins = *(bank_base + reg_pc);                    \
            (o).op.op16 = (*(bank_base + reg_pc + 1)            \
                          | (*(bank_base + reg_pc + 2) << 8));  \
            CLK_ADD(CLK,2);                                     \
            if (fetch_tab[(o).ins]) {                           \
                CLK_ADD(CLK,1);                                 \
            }                                                   \
        } else {                                                \
            (o).ins = LOAD(reg_pc);                             \
            CLK_ADD(CLK,1);                                     \
            (o).op.op16 = LOAD(reg_pc + 1);                     \
            CLK_ADD(CLK,1);                                     \
            if (fetch_tab[(o).ins]) {                           \
                 (o).op.op16 |= (LOAD(reg_pc + 2) << 8);        \
                 CLK_ADD(CLK,1);                                \
            }                                                   \
        }                                                       \
    } while (0)

#define p0 (opcode.ins)
#define p2 (opcode.op.op16)

#ifdef WORDS_BIGENDIAN
#  define p1 (opcode.op.op8[1])
#else
#  define p1 (opcode.op.op8[0])
#endif

#endif /* !WORDS_BIGENDIAN */
#endif

/*  SET_OPCODE for traps */
#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS
#define SET_OPCODE(o) (opcode) = o
#else
#if !defined WORDS_BIGENDIAN
#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[0] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[1] = ((o) >> 16) & 0xff; \
    } while (0)
#else
#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[1] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[0] = ((o) >> 16) & 0xff; \
    } while (0)
#endif

#endif

#endif /* !C64DTV */


