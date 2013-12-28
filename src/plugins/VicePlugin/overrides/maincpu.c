/*
 * maincpu.c - Emulation of the main 6510 processor.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "6510core.h"
#include "alarm.h"
#include "clkguard.h"
#include "debug.h"
#include "interrupt.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#ifdef C64DTV
#include "mos6510dtv.h"
#else
#include "mos6510.h"
#endif
#include "h6809regs.h"
#include "snapshot.h"
#include "traps.h"
#include "types.h"


/* MACHINE_STUFF should define/undef

 - NEED_REG_PC

 The following are optional:

 - PAGE_ZERO
 - PAGE_ONE
 - STORE_IND
 - LOAD_IND

*/

/* ------------------------------------------------------------------------- */

#define NEED_REG_PC

/* ------------------------------------------------------------------------- */

/* Implement the hack to make opcode fetches faster.  */
#define JUMP(addr)                            \
    do {                                      \
        reg_pc = (unsigned int)(addr);        \
        bank_base = mem_read_base(reg_pc);    \
        bank_limit = mem_read_limit(reg_pc);  \
        mem_old_reg_pc = reg_pc;              \
    } while (0)

/* ------------------------------------------------------------------------- */

#ifndef STORE_ZERO
#define STORE_ZERO(addr, value) \
    (*_mem_write_tab_ptr[0])((WORD)(addr), (BYTE)(value))
#endif

#ifndef LOAD_ZERO
#define LOAD_ZERO(addr) \
    (*_mem_read_tab_ptr[0])((WORD)(addr))
#endif

#ifdef FEATURE_CPUMEMHISTORY
#ifndef C64DTV

/* HACK this is C64 specific */

void memmap_mem_store(unsigned int addr, unsigned int value)
{
    if ((addr >= 0xd000)&&(addr <= 0xdfff)) {
        monitor_memmap_store(addr, MEMMAP_I_O_W);
    } else {
        monitor_memmap_store(addr, MEMMAP_RAM_W);
    }
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

BYTE memmap_mem_read(unsigned int addr)
{
    switch(addr >> 12) {
        case 0xa:
        case 0xb:
        case 0xe:
        case 0xf:
            memmap_state |= MEMMAP_STATE_IGNORE;
            if (LOAD_ZERO(1) & (1 << ((addr>>14) & 1))) {
                monitor_memmap_store(addr, (memmap_state&MEMMAP_STATE_OPCODE)?MEMMAP_ROM_X:(memmap_state&MEMMAP_STATE_INSTR)?0:MEMMAP_ROM_R);
            } else {
                monitor_memmap_store(addr, (memmap_state&MEMMAP_STATE_OPCODE)?MEMMAP_RAM_X:(memmap_state&MEMMAP_STATE_INSTR)?0:MEMMAP_RAM_R);
            }
            memmap_state &= ~(MEMMAP_STATE_IGNORE);
            break;
        case 0xd:
            monitor_memmap_store(addr, MEMMAP_I_O_R);
            break;
        default:
            monitor_memmap_store(addr, (memmap_state&MEMMAP_STATE_OPCODE)?MEMMAP_RAM_X:(memmap_state&MEMMAP_STATE_INSTR)?0:MEMMAP_RAM_R);
            break;
    }
    memmap_state &= ~(MEMMAP_STATE_OPCODE);
    return (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr));
}

#ifndef STORE
#define STORE(addr, value) \
    memmap_mem_store(addr, value)
#endif

#ifndef LOAD
#define LOAD(addr) \
    memmap_mem_read(addr)
#endif

#endif /* C64DTV */
#endif /* FEATURE_CPUMEMHISTORY */

#ifndef STORE
#define STORE(addr, value) \
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value))
#endif

#ifndef LOAD
#define LOAD(addr) \
    (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr))
#endif

#define LOAD_ADDR(addr) \
    ((LOAD((addr) + 1) << 8) | LOAD(addr))

#define LOAD_ZERO_ADDR(addr) \
    ((LOAD_ZERO((addr) + 1) << 8) | LOAD_ZERO(addr))

static BYTE *bank_base;
static int bank_limit;

inline static BYTE *mem_read_base(int addr)
{
    BYTE *p = _mem_read_base_tab_ptr[addr >> 8];

    if (p == NULL)
        return p;

    return p - (addr & 0xff00);
}

inline static int mem_read_limit(int addr)
{
    return mem_read_limit_tab_ptr[addr >> 8];
}

/* Those may be overridden by the machine stuff.  Probably we want them in
   the .def files, but if most of the machines do not use, we might keep it
   here and only override it where needed.  */
#ifndef PAGE_ZERO
#define PAGE_ZERO mem_ram
#endif

#ifndef PAGE_ONE
#define PAGE_ONE (mem_ram + 0x100)
#endif

#ifndef STORE_IND
#define STORE_IND(addr, value) STORE((addr),(value))
#endif

#ifndef LOAD_IND
#define LOAD_IND(addr) LOAD((addr))
#endif

#ifndef DMA_FUNC
static void maincpu_generic_dma(void)
{
    /* Generic DMA hosts can be implemented here.
       For example a very accurate REU emulation. */
}
#define DMA_FUNC maincpu_generic_dma()
#endif

#ifndef DMA_ON_RESET
#define DMA_ON_RESET
#endif

#ifndef CPU_ADDITIONAL_RESET
#define CPU_ADDITIONAL_RESET()
#endif

#ifndef CPU_ADDITIONAL_INIT
#define CPU_ADDITIONAL_INIT()
#endif

/* ------------------------------------------------------------------------- */

struct interrupt_cpu_status_s *maincpu_int_status = NULL;
#ifndef CYCLE_EXACT_ALARM
alarm_context_t *maincpu_alarm_context = NULL;
#endif
clk_guard_t *maincpu_clk_guard = NULL;
monitor_interface_t *maincpu_monitor_interface = NULL;

/* Global clock counter.  */
CLOCK maincpu_clk = 0L;

/* This is flag is set to 1 each time a Read-Modify-Write instructions that
   accesses memory is executed.  We can emulate the RMW behaviour of the 6510
   this way.  VERY important notice: Always assign 1 for true, 0 for false!
   Some functions depend on this to do some optimization.  */
int maincpu_rmw_flag = 0;

/* Information about the last executed opcode.  This is used to know the
   number of write cycles in the last executed opcode and to delay interrupts
   by one more cycle if necessary, as happens with conditional branch opcodes
   when the branch is taken.  */
unsigned int last_opcode_info;

/* Address of the last executed opcode. This is used by watchpoints. */
unsigned int last_opcode_addr;

/* Number of write cycles for each 6510 opcode.  */
const CLOCK maincpu_opcode_write_cycles[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  3, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $00 */
    /* $10 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $10 */
    /* $20 */  2, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $20 */
    /* $30 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $30 */
    /* $40 */  0, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $40 */
    /* $50 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $50 */
    /* $60 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $60 */
    /* $70 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $70 */
    /* $80 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
    /* $90 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
    /* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $A0 */
    /* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $B0 */
    /* $C0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $C0 */
    /* $D0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $D0 */
    /* $E0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $E0 */
    /* $F0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2  /* $F0 */
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
};

/* Public copy of the CPU registers.  As putting the registers into the
   function makes it faster, you have to generate a `TRAP' interrupt to have
   the values copied into this struct.  */
#ifdef C64DTV
mos6510dtv_regs_t maincpu_regs;
#else
mos6510_regs_t maincpu_regs;
#endif

/* ------------------------------------------------------------------------- */

monitor_interface_t *maincpu_monitor_interface_get(void)
{
#ifdef C64DTV
    maincpu_monitor_interface->cpu_regs = NULL;
    maincpu_monitor_interface->dtv_cpu_regs = &maincpu_regs;
#else
    maincpu_monitor_interface->cpu_regs = &maincpu_regs;
    maincpu_monitor_interface->dtv_cpu_regs = NULL;
#endif

#ifdef HAVE_Z80_REGS
    maincpu_monitor_interface->z80_cpu_regs = &z80_regs;
#else
    maincpu_monitor_interface->z80_cpu_regs = NULL;
#endif
#ifdef HAVE_6809_REGS
    maincpu_monitor_interface->h6809_cpu_regs = &h6809_regs;
#else
    maincpu_monitor_interface->h6809_cpu_regs = NULL;
#endif

    maincpu_monitor_interface->int_status = maincpu_int_status;

    maincpu_monitor_interface->clk = &maincpu_clk;

    maincpu_monitor_interface->current_bank = 0;
    maincpu_monitor_interface->mem_bank_list = mem_bank_list;
    maincpu_monitor_interface->mem_bank_from_name = mem_bank_from_name;
    maincpu_monitor_interface->mem_bank_read = mem_bank_read;
    maincpu_monitor_interface->mem_bank_peek = mem_bank_peek;
    maincpu_monitor_interface->mem_bank_write = mem_bank_write;

    maincpu_monitor_interface->mem_ioreg_list_get = mem_ioreg_list_get;

    maincpu_monitor_interface->toggle_watchpoints_func = mem_toggle_watchpoints;

    maincpu_monitor_interface->set_bank_base = NULL;
    maincpu_monitor_interface->get_line_cycle = machine_get_line_cycle;

    return maincpu_monitor_interface;
}

/* ------------------------------------------------------------------------- */

void maincpu_early_init(void)
{
    maincpu_int_status = interrupt_cpu_status_new();
}

void maincpu_init(void)
{
    interrupt_cpu_status_init(maincpu_int_status, &last_opcode_info);

    /* cpu specifix additional init routine */
    CPU_ADDITIONAL_INIT();
}

void maincpu_shutdown(void)
{
    interrupt_cpu_status_destroy(maincpu_int_status);
}

static void cpu_reset(void)
{
    int preserve_monitor;

    preserve_monitor = maincpu_int_status->global_pending_int & IK_MONITOR;

    interrupt_cpu_status_reset(maincpu_int_status);

    if (preserve_monitor)
        interrupt_monitor_trap_on(maincpu_int_status);

    maincpu_clk = 6; /* # of clock cycles needed for RESET.  */

    /* CPU specific extra reset routine, currently only used
       for 8502 fast mode refresh cycle. */
    CPU_ADDITIONAL_RESET();

    /* Do machine-specific initialization.  */
    machine_reset();
}

void maincpu_reset(void)
{
    mem_set_bank_pointer(&bank_base, &bank_limit);
    cpu_reset();
}

/* ------------------------------------------------------------------------- */

/* Return nonzero if a pending NMI should be dispatched now.  This takes
   account for the internal delays of the 6510, but does not actually check
   the status of the NMI line.  */
inline static int interrupt_check_nmi_delay(interrupt_cpu_status_t *cs,
                                            CLOCK cpu_clk)
{
    CLOCK nmi_clk = cs->nmi_clk + INTERRUPT_DELAY;

    /* Branch instructions delay IRQs and NMI by one cycle if branch
       is taken with no page boundary crossing.  */
    if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr))
        nmi_clk++;

    if (cpu_clk >= nmi_clk)
        return 1;

    return 0;
}

/* Return nonzero if a pending IRQ should be dispatched now.  This takes
   account for the internal delays of the 6510, but does not actually check
   the status of the IRQ line.  */
inline static int interrupt_check_irq_delay(interrupt_cpu_status_t *cs,
                                            CLOCK cpu_clk)
{
    CLOCK irq_clk = cs->irq_clk + INTERRUPT_DELAY;

    /* Branch instructions delay IRQs and NMI by one cycle if branch
       is taken with no page boundary crossing.  */
    if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr))
        irq_clk++;

    /* If an opcode changes the I flag from 1 to 0, the 6510 needs
       one more opcode before it triggers the IRQ routine.  */
    if (cpu_clk >= irq_clk) {
        if (!OPINFO_ENABLES_IRQ(*cs->last_opcode_info_ptr)) {
            return 1;
        } else {
            cs->global_pending_int |= IK_IRQPEND;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

#ifdef NEED_REG_PC
unsigned int reg_pc;
#endif

short *psid_sound_buf;
int psid_sound_idx;
int psid_sound_max;

#ifndef C64DTV
    /* Notice that using a struct for these would make it a lot slower (at
       least, on gcc 2.7.2.x).  */
    static BYTE reg_a = 0;
    static BYTE reg_x = 0;
    static BYTE reg_y = 0;
#else
    static int reg_a_read_idx = 0;
    static int reg_a_write_idx = 0;
    static int reg_x_idx = 2;
    static int reg_y_idx = 1;
#define reg_a_write dtv_registers[reg_a_write_idx]
#define reg_a_read dtv_registers[reg_a_read_idx]
#define reg_x dtv_registers[reg_x_idx]
#define reg_y dtv_registers[reg_y_idx]
#endif
    static BYTE reg_p = 0;
    static BYTE reg_sp = 0;
    static BYTE flag_n = 0;
    static BYTE flag_z = 0;
#ifndef NEED_REG_PC
    static unsigned int reg_pc;
#endif

#define CLK maincpu_clk
#define RMW_FLAG maincpu_rmw_flag
#define LAST_OPCODE_INFO last_opcode_info
#define LAST_OPCODE_ADDR last_opcode_addr
#define TRACEFLG debug.maincpu_traceflg

#define CPU_INT_STATUS maincpu_int_status

#define ALARM_CONTEXT maincpu_alarm_context

#define CHECK_PENDING_ALARM() \
   (clk >= next_alarm_clk(maincpu_int_status))

#define CHECK_PENDING_INTERRUPT() \
   check_pending_interrupt(maincpu_int_status)

#define TRAP(addr) \
   maincpu_int_status->trap_func(addr);

#define ROM_TRAP_HANDLER() \
   traps_handler()

#define JAM()                                                         \
    do {                                                              \
        unsigned int tmp;                                             \
                                                                      \
        EXPORT_REGISTERS();                                           \
        tmp = machine_jam("   " CPU_STR ": JAM at $%04X   ", reg_pc); \
        switch (tmp) {                                                \
          case JAM_RESET:                                             \
            DO_INTERRUPT(IK_RESET);                                   \
            break;                                                    \
          case JAM_HARD_RESET:                                        \
            mem_powerup();                                            \
            DO_INTERRUPT(IK_RESET);                                   \
            break;                                                    \
          case JAM_MONITOR:                                           \
            monitor_startup(e_comp_space);                            \
            IMPORT_REGISTERS();                                         \
            break;                                                    \
          default:                                                    \
            CLK++;                                                    \
        }                                                             \
    } while (0)

#define CALLER e_comp_space

#define ROM_TRAP_ALLOWED() mem_rom_trap_allowed((WORD)reg_pc)

#define GLOBAL_REGS maincpu_regs

#include "6510core.c"

void other_opcodes();

static opcode_t opcode;

void psid_play(short *buf, int n)
{
    psid_sound_buf = buf;
    psid_sound_idx = 0;
    psid_sound_max = n;
    while (psid_sound_idx != psid_sound_max) {


/* ------------------------------------------------------------------------ */

/* Here, the CPU is emulated. */
{
    /* handle 8502 fast mode refresh cycles */
    CPU_REFRESH_CLK

    CPU_DELAY_CLK

#ifndef CYCLE_EXACT_ALARM
    while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
        alarm_context_dispatch(ALARM_CONTEXT, CLK);
        CPU_DELAY_CLK
    }
#endif

    {
        enum cpu_int pending_interrupt;

        if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
            && (CPU_INT_STATUS->global_pending_int & IK_IRQPEND)
            && CPU_INT_STATUS->irq_pending_clk <= CLK) {
            interrupt_ack_irq(CPU_INT_STATUS);
        }

        pending_interrupt = CPU_INT_STATUS->global_pending_int;
        if (pending_interrupt != IK_NONE) {
            DO_INTERRUPT(pending_interrupt);
            if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
                && CPU_INT_STATUS->global_pending_int & IK_IRQPEND)
                    CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
            CPU_DELAY_CLK
#ifndef CYCLE_EXACT_ALARM
            while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
                alarm_context_dispatch(ALARM_CONTEXT, CLK);
                CPU_DELAY_CLK
            }
#endif
        }
    }

    {
        //opcode_t opcode;
#ifdef DEBUG
        CLOCK debug_clk;
#ifdef DRIVE_CPU
        debug_clk = CLK;
#else
        debug_clk = maincpu_clk;
#endif
#endif

#ifdef FEATURE_CPUMEMHISTORY
#ifndef DRIVE_CPU
        memmap_state |= (MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif
#endif
        SET_LAST_ADDR(reg_pc);
        FETCH_OPCODE(opcode);

#ifdef FEATURE_CPUMEMHISTORY
#ifndef DRIVE_CPU
#ifndef C64DTV
        /* HACK to cope with FETCH_OPCODE optimization in x64 */
        if (((int)reg_pc) < bank_limit) {
            memmap_mem_read(reg_pc);
        }
#endif
        if (p0 == 0x20) {
            monitor_cpuhistory_store(reg_pc, p0, p1, LOAD(reg_pc+2), reg_a_read, reg_x, reg_y, reg_sp, LOCAL_STATUS());
        } else {
            monitor_cpuhistory_store(reg_pc, p0, p1, p2 >> 8, reg_a_read, reg_x, reg_y, reg_sp, LOCAL_STATUS());
        }
        memmap_state &= ~(MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif
#endif

#ifdef DEBUG
#ifdef DRIVE_CPU
        if (TRACEFLG) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8);

            debug_drive((DWORD)(reg_pc), debug_clk,
                        mon_disassemble_to_string(e_disk8_space,
                                                  reg_pc, op,
                                                  lo, hi, 0, 1, "6502"),
                        reg_a_read, reg_x, reg_y, reg_sp, drv->mynumber + 8);
        }
#else
        if (TRACEFLG) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8); 

            if (op == 0x20) {
               hi = LOAD(reg_pc + 2);
            }

            debug_maincpu((DWORD)(reg_pc), debug_clk,
                          mon_disassemble_to_string(e_comp_space,
                                                    reg_pc, op,
                                                    lo, hi, 0, 1, "6502"),
                          reg_a_read, reg_x, reg_y, reg_sp);
        }
        if (debug.perform_break_into_monitor)
        {
            monitor_startup_trap();
            debug.perform_break_into_monitor = 0;
        }
#endif
#endif

trap_skipped:
        SET_LAST_OPCODE(p0);

        switch (p0) {

          case 0x4c:            /* JMP $nnnn */
            JMP(p2);
            break;

          case 0xf0:            /* BEQ $nnnn */
            BRANCH(LOCAL_ZERO(), p1);
            break;

          case 0xa9:            /* LDA #$nn */
            LDA(p1, 0, 2);
            break;

          case 0xea:            /* NOP */
            NOP();
            break;

          case 0xad:            /* LDA $nnnn */
            LDA(LOAD(p2), 1, 3);
            break;

          case 0xae:            /* LDX $nnnn */
            LDX(LOAD(p2), 1, 3);
            break;

          case 0x9d:            /* STA $nnnn,X */
            STA(p2, 0, CLK_ABS_I_STORE2, 3, STORE_ABS_X);
            break;

          case 0xd0:            /* BNE $nnnn */
            BRANCH(!LOCAL_ZERO(), p1);
            break;

          case 0xa5:            /* LDA $nn */
            LDA(LOAD_ZERO(p1), 1, 2);
            break;

          case 0x8d:            /* STA $nnnn */
            STA(p2, 0, 1, 3, STORE_ABS);
            break;

          case 0xbd:            /* LDA $nnnn,X */
            LDA(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0xbe:            /* LDX $nnnn,Y */
            LDX(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0x7d:            /* ADC $nnnn,X */
            ADC(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0x10:            /* BPL $nnnn */
            BRANCH(!LOCAL_SIGN(), p1);
            break;

          case 0x85:            /* STA $nn */
            STA_ZERO(p1, 1, 2);
            break;

          case 0x99:            /* STA $nnnn,Y */
            STA(p2, 0, CLK_ABS_I_STORE2, 3, STORE_ABS_Y);
            break;

          case 0x88:            /* DEY */
            DEY();
            break;

          case 0x40:            /* RTI */
            RTI();
            break;

          case 0xb9:            /* LDA $nnnn,Y */
            LDA(LOAD_ABS_Y(p2), 1, 3);
            break;

            //// Here for differnt reason
          case 0x02:            /* JAM - also used for traps */
            STATIC_ASSERT(TRAP_OPCODE == 0x02);
            JAM_02();
            break;
            //////////////////////
          default:
            other_opcodes();
            break;
        }
    }
}
    maincpu_int_status->num_dma_per_opcode = 0;
    }
}

void other_opcodes() {

    switch (p0) {

     case 0x00:            /* BRK */
            BRK();
            break;

          case 0x01:            /* ORA ($nn,X) */
            ORA(LOAD_IND_X(p1), 1, 2);
            break;

          case 0x22:            /* JAM */
          case 0x52:            /* JAM */
          case 0x62:            /* JAM */
          case 0x72:            /* JAM */
          case 0x92:            /* JAM */
          case 0xb2:            /* JAM */
          case 0xd2:            /* JAM */
          case 0xf2:            /* JAM */
#ifndef C64DTV
          case 0x12:            /* JAM */
          case 0x32:            /* JAM */
          case 0x42:            /* JAM */
#endif
            REWIND_FETCH_OPCODE(CLK);
            JAM();
            break;

#ifdef C64DTV
          /* These opcodes are defined in c64/c64dtvcpu.c */
          case 0x12:            /* BRA */
            BRANCH(1, p1);
            break;

          case 0x32:            /* SAC */
            SAC(p1);
            break;

          case 0x42:            /* SIR */
            SIR(p1);
            break;
#endif

          case 0x03:            /* SLO ($nn,X) */
            SLO(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0x04:            /* NOOP $nn */
          case 0x44:            /* NOOP $nn */
          case 0x64:            /* NOOP $nn */
            NOOP(1, 2);
            break;

          case 0x05:            /* ORA $nn */
            ORA(LOAD_ZERO(p1), 1, 2);
            break;

          case 0x06:            /* ASL $nn */
            ASL(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x07:            /* SLO $nn */
            SLO(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x08:            /* PHP */
#ifdef DRIVE_CPU
            if (drivecpu_byte_ready())
                LOCAL_SET_OVERFLOW(1);
#endif
            PHP();
            break;

          case 0x09:            /* ORA #$nn */
            ORA(p1, 0, 2);
            break;

          case 0x0a:            /* ASL A */
            ASL_A();
            break;

          case 0x0b:            /* ANC #$nn */
            ANC(p1, 2);
            break;

          case 0x0c:            /* NOOP $nnnn */
            NOOP_ABS();
            break;

          case 0x0d:            /* ORA $nnnn */
            ORA(LOAD(p2), 1, 3);
            break;

          case 0x0e:            /* ASL $nnnn */
            ASL(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x0f:            /* SLO $nnnn */
            SLO(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x11:            /* ORA ($nn),Y */
            ORA(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0x13:            /* SLO ($nn),Y */
            SLO_IND_Y(p1);
            break;

          case 0x14:            /* NOOP $nn,X */
          case 0x34:            /* NOOP $nn,X */
          case 0x54:            /* NOOP $nn,X */
          case 0x74:            /* NOOP $nn,X */
          case 0xd4:            /* NOOP $nn,X */
          case 0xf4:            /* NOOP $nn,X */
            NOOP(CLK_NOOP_ZERO_X, 2);
            break;

          case 0x15:            /* ORA $nn,X */
            ORA(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0x16:            /* ASL $nn,X */
            ASL((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x17:            /* SLO $nn,X */
            SLO((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x18:            /* CLC */
            CLC();
            break;

          case 0x19:            /* ORA $nnnn,Y */
            ORA(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0x1a:            /* NOOP */
          case 0x3a:            /* NOOP */
          case 0x5a:            /* NOOP */
          case 0x7a:            /* NOOP */
          case 0xda:            /* NOOP */
          case 0xfa:            /* NOOP */
            NOOP_IMM(1);
            break;

          case 0x1b:            /* SLO $nnnn,Y */
            SLO(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0x1c:            /* NOOP $nnnn,X */
          case 0x3c:            /* NOOP $nnnn,X */
          case 0x5c:            /* NOOP $nnnn,X */
          case 0x7c:            /* NOOP $nnnn,X */
          case 0xdc:            /* NOOP $nnnn,X */
          case 0xfc:            /* NOOP $nnnn,X */
            NOOP_ABS_X();
            break;

          case 0x1d:            /* ORA $nnnn,X */
            ORA(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0x1e:            /* ASL $nnnn,X */
            ASL(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x1f:            /* SLO $nnnn,X */
            SLO(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x20:            /* JSR $nnnn */
            JSR();
            break;

          case 0x21:            /* AND ($nn,X) */
            AND(LOAD_IND_X(p1), 1, 2);
            break;

          case 0x23:            /* RLA ($nn,X) */
            RLA(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0x24:            /* BIT $nn */
            BIT(LOAD_ZERO(p1), 2);
            break;

          case 0x25:            /* AND $nn */
            AND(LOAD_ZERO(p1), 1, 2);
            break;

          case 0x26:            /* ROL $nn */
            ROL(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x27:            /* RLA $nn */
            RLA(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x28:            /* PLP */
            PLP();
            break;

          case 0x29:            /* AND #$nn */
            AND(p1, 0, 2);
            break;

          case 0x2a:            /* ROL A */
            ROL_A();
            break;

          case 0x2b:            /* ANC #$nn */
            ANC(p1, 2);
            break;

          case 0x2c:            /* BIT $nnnn */
            BIT(LOAD(p2), 3);
            break;

          case 0x2d:            /* AND $nnnn */
            AND(LOAD(p2), 1, 3);
            break;

          case 0x2e:            /* ROL $nnnn */
            ROL(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x2f:            /* RLA $nnnn */
            RLA(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x30:            /* BMI $nnnn */
            BRANCH(LOCAL_SIGN(), p1);
            break;

          case 0x31:            /* AND ($nn),Y */
            AND(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0x33:            /* RLA ($nn),Y */
            RLA_IND_Y(p1);
            break;

          case 0x35:            /* AND $nn,X */
            AND(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0x36:            /* ROL $nn,X */
            ROL((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x37:            /* RLA $nn,X */
            RLA((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x38:            /* SEC */
            SEC();
            break;

          case 0x39:            /* AND $nnnn,Y */
            AND(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0x3b:            /* RLA $nnnn,Y */
            RLA(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0x3d:            /* AND $nnnn,X */
            AND(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0x3e:            /* ROL $nnnn,X */
            ROL(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x3f:            /* RLA $nnnn,X */
            RLA(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x41:            /* EOR ($nn,X) */
            EOR(LOAD_IND_X(p1), 1, 2);
            break;

          case 0x43:            /* SRE ($nn,X) */
            SRE(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0x45:            /* EOR $nn */
            EOR(LOAD_ZERO(p1), 1, 2);
            break;

          case 0x46:            /* LSR $nn */
            LSR(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x47:            /* SRE $nn */
            SRE(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x48:            /* PHA */
            PHA();
            break;

          case 0x49:            /* EOR #$nn */
            EOR(p1, 0, 2);
            break;

          case 0x4a:            /* LSR A */
            LSR_A();
            break;

          case 0x4b:            /* ASR #$nn */
            ASR(p1, 2);
            break;

          case 0x4d:            /* EOR $nnnn */
            EOR(LOAD(p2), 1, 3);
            break;

          case 0x4e:            /* LSR $nnnn */
            LSR(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x4f:            /* SRE $nnnn */
            SRE(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x50:            /* BVC $nnnn */
#ifdef DRIVE_CPU
            CLK_ADD(CLK, -1);
            if (drivecpu_byte_ready())
                LOCAL_SET_OVERFLOW(1);
            CLK_ADD(CLK, 1);
#endif
            BRANCH(!LOCAL_OVERFLOW(), p1);
            break;

          case 0x51:            /* EOR ($nn),Y */
            EOR(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0x53:            /* SRE ($nn),Y */
            SRE_IND_Y(p1);
            break;

          case 0x55:            /* EOR $nn,X */
            EOR(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0x56:            /* LSR $nn,X */
            LSR((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x57:            /* SRE $nn,X */
            SRE((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x58:            /* CLI */
            CLI();
            break;

          case 0x59:            /* EOR $nnnn,Y */
            EOR(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0x5b:            /* SRE $nnnn,Y */
            SRE(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0x5d:            /* EOR $nnnn,X */
            EOR(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0x5e:            /* LSR $nnnn,X */
            LSR(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x5f:            /* SRE $nnnn,X */
            SRE(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x60:            /* RTS */
            RTS();
            break;

          case 0x61:            /* ADC ($nn,X) */
            ADC(LOAD_IND_X(p1), 1, 2);
            break;

          case 0x63:            /* RRA ($nn,X) */
            RRA(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0x65:            /* ADC $nn */
            ADC(LOAD_ZERO(p1), 1, 2);
            break;

          case 0x66:            /* ROR $nn */
            ROR(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x67:            /* RRA $nn */
            RRA(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x68:            /* PLA */
            PLA();
            break;

          case 0x69:            /* ADC #$nn */
            ADC(p1, 0, 2);
            break;

          case 0x6a:            /* ROR A */
            ROR_A();
            break;

          case 0x6b:            /* ARR #$nn */
            ARR(p1, 2);
            break;

          case 0x6c:            /* JMP ($nnnn) */
            JMP_IND();
            break;

          case 0x6d:            /* ADC $nnnn */
            ADC(LOAD(p2), 1, 3);
            break;

          case 0x6e:            /* ROR $nnnn */
            ROR(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x6f:            /* RRA $nnnn */
            RRA(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0x70:            /* BVS $nnnn */
#ifdef DRIVE_CPU
            CLK_ADD(CLK, -1);
            if (drivecpu_byte_ready())
                LOCAL_SET_OVERFLOW(1);
            CLK_ADD(CLK, 1);
#endif
            BRANCH(LOCAL_OVERFLOW(), p1);
            break;

          case 0x71:            /* ADC ($nn),Y */
            ADC(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0x73:            /* RRA ($nn),Y */
            RRA_IND_Y(p1);
            break;

          case 0x75:            /* ADC $nn,X */
            ADC(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0x76:            /* ROR $nn,X */
            ROR((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x77:            /* RRA $nn,X */
            RRA((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0x78:            /* SEI */
            SEI();
            break;

          case 0x79:            /* ADC $nnnn,Y */
            ADC(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0x7b:            /* RRA $nnnn,Y */
            RRA(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0x7e:            /* ROR $nnnn,X */
            ROR(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x7f:            /* RRA $nnnn,X */
            RRA(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0x80:            /* NOOP #$nn */
          case 0x82:            /* NOOP #$nn */
          case 0x89:            /* NOOP #$nn */
          case 0xc2:            /* NOOP #$nn */
          case 0xe2:            /* NOOP #$nn */
            NOOP_IMM(2);
            break;

          case 0x81:            /* STA ($nn,X) */
            STA(LOAD_ZERO_ADDR(p1 + reg_x), 3, 1, 2, STORE_ABS);
            break;

          case 0x83:            /* SAX ($nn,X) */
            SAX(LOAD_ZERO_ADDR(p1 + reg_x), 3, 1, 2);
            break;

          case 0x84:            /* STY $nn */
            STY_ZERO(p1, 1, 2);
            break;

          case 0x86:            /* STX $nn */
            STX_ZERO(p1, 1, 2);
            break;

          case 0x87:            /* SAX $nn */
            SAX_ZERO(p1, 1, 2);
            break;

          case 0x8a:            /* TXA */
            TXA();
            break;

          case 0x8b:            /* ANE #$nn */
            ANE(p1, 2);
            break;

          case 0x8c:            /* STY $nnnn */
            STY(p2, 1, 3);
            break;

          case 0x8e:            /* STX $nnnn */
            STX(p2, 1, 3);
            break;

          case 0x8f:            /* SAX $nnnn */
            SAX(p2, 0, 1, 3);
            break;

          case 0x90:            /* BCC $nnnn */
            BRANCH(!LOCAL_CARRY(), p1);
            break;

          case 0x91:            /* STA ($nn),Y */
            STA_IND_Y(p1);
            break;

          case 0x93:            /* SHA ($nn),Y */
            SHA_IND_Y(p1);
            break;

          case 0x94:            /* STY $nn,X */
            STY_ZERO(p1 + reg_x, CLK_ZERO_I_STORE, 2);
            break;

          case 0x95:            /* STA $nn,X */
            STA_ZERO(p1 + reg_x, CLK_ZERO_I_STORE, 2);
            break;

          case 0x96:            /* STX $nn,Y */
            STX_ZERO(p1 + reg_y, CLK_ZERO_I_STORE, 2);
            break;

          case 0x97:            /* SAX $nn,Y */
            SAX((p1 + reg_y) & 0xff, 0, CLK_ZERO_I_STORE, 2);
            break;

          case 0x98:            /* TYA */
            TYA();
            break;

          case 0x9a:            /* TXS */
            TXS();
            break;

          case 0x9b:            /* SHS $nnnn,Y */
#ifdef C64DTV
            NOOP_ABS_Y();
#else
            SHS_ABS_Y(p2);
#endif
            break;

          case 0x9c:            /* SHY $nnnn,X */
            SHY_ABS_X(p2);
            break;

          case 0x9e:            /* SHX $nnnn,Y */
            SHX_ABS_Y(p2);
            break;

          case 0x9f:            /* SHA $nnnn,Y */
            SHA_ABS_Y(p2);
            break;

          case 0xa0:            /* LDY #$nn */
            LDY(p1, 0, 2);
            break;

          case 0xa1:            /* LDA ($nn,X) */
            LDA(LOAD_IND_X(p1), 1, 2);
            break;

          case 0xa2:            /* LDX #$nn */
            LDX(p1, 0, 2);
            break;

          case 0xa3:            /* LAX ($nn,X) */
            LAX(LOAD_IND_X(p1), 1, 2);
            break;

          case 0xa4:            /* LDY $nn */
            LDY(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xa6:            /* LDX $nn */
            LDX(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xa7:            /* LAX $nn */
            LAX(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xa8:            /* TAY */
            TAY();
            break;

          case 0xa9:            /* LDA #$nn */
            LDA(p1, 0, 2);
            break;

          case 0xaa:            /* TAX */
            TAX();
            break;

          case 0xab:            /* LXA #$nn */
            LXA(p1, 2);
            break;

          case 0xac:            /* LDY $nnnn */
            LDY(LOAD(p2), 1, 3);
            break;

          case 0xaf:            /* LAX $nnnn */
            LAX(LOAD(p2), 1, 3);
            break;

          case 0xb0:            /* BCS $nnnn */
            BRANCH(LOCAL_CARRY(), p1);
            break;

          case 0xb1:            /* LDA ($nn),Y */
            LDA(LOAD_IND_Y_BANK(p1), 1, 2);
            break;

          case 0xb3:            /* LAX ($nn),Y */
            LAX(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0xb4:            /* LDY $nn,X */
            LDY(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0xb5:            /* LDA $nn,X */
            LDA(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0xb6:            /* LDX $nn,Y */
            LDX(LOAD_ZERO_Y(p1), CLK_ZERO_I2, 2);
            break;

          case 0xb7:            /* LAX $nn,Y */
            LAX(LOAD_ZERO_Y(p1), CLK_ZERO_I2, 2);
            break;

          case 0xb8:            /* CLV */
            CLV();
            break;

          case 0xba:            /* TSX */
            TSX();
            break;

          case 0xbb:            /* LAS $nnnn,Y */
            LAS(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0xbc:            /* LDY $nnnn,X */
            LDY(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0xbf:            /* LAX $nnnn,Y */
            LAX(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0xc0:            /* CPY #$nn */
            CPY(p1, 0, 2);
            break;

          case 0xc1:            /* CMP ($nn,X) */
            CMP(LOAD_IND_X(p1), 1, 2);
            break;

          case 0xc3:            /* DCP ($nn,X) */
            DCP(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0xc4:            /* CPY $nn */
            CPY(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xc5:            /* CMP $nn */
            CMP(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xc6:            /* DEC $nn */
            DEC(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xc7:            /* DCP $nn */
            DCP(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xc8:            /* INY */
            INY();
            break;

          case 0xc9:            /* CMP #$nn */
            CMP(p1, 0, 2);
            break;

          case 0xca:            /* DEX */
            DEX();
            break;

          case 0xcb:            /* SBX #$nn */
            SBX(p1, 2);
            break;

          case 0xcc:            /* CPY $nnnn */
            CPY(LOAD(p2), 1, 3);
            break;

          case 0xcd:            /* CMP $nnnn */
            CMP(LOAD(p2), 1, 3);
            break;

          case 0xce:            /* DEC $nnnn */
            DEC(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0xcf:            /* DCP $nnnn */
            DCP(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0xd1:            /* CMP ($nn),Y */
            CMP(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0xd3:            /* DCP ($nn),Y */
            DCP_IND_Y(p1);
            break;

          case 0xd5:            /* CMP $nn,X */
            CMP(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0xd6:            /* DEC $nn,X */
            DEC((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0xd7:            /* DCP $nn,X */
            DCP((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0xd8:            /* CLD */
            CLD();
            break;

          case 0xd9:            /* CMP $nnnn,Y */
            CMP(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0xdb:            /* DCP $nnnn,Y */
            DCP(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0xdd:            /* CMP $nnnn,X */
            CMP(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0xde:            /* DEC $nnnn,X */
            DEC(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0xdf:            /* DCP $nnnn,X */
            DCP(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0xe0:            /* CPX #$nn */
            CPX(p1, 0, 2);
            break;

          case 0xe1:            /* SBC ($nn,X) */
            SBC(LOAD_IND_X(p1), 1, 2);
            break;

          case 0xe3:            /* ISB ($nn,X) */
            ISB(LOAD_ZERO_ADDR(p1 + reg_x), 3, CLK_IND_X_RMW, 2, LOAD_ABS, STORE_ABS);
            break;

          case 0xe4:            /* CPX $nn */
            CPX(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xe5:            /* SBC $nn */
            SBC(LOAD_ZERO(p1), 1, 2);
            break;

          case 0xe6:            /* INC $nn */
            INC(p1, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xe7:            /* ISB $nn */
            ISB(p1, 0, CLK_ZERO_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xe8:            /* INX */
            INX();
            break;

          case 0xe9:            /* SBC #$nn */
            SBC(p1, 0, 2);
            break;

          case 0xea:            /* NOP */
            NOP();
            break;

          case 0xeb:            /* USBC #$nn (same as SBC) */
            SBC(p1, 0, 2);
            break;

          case 0xec:            /* CPX $nnnn */
            CPX(LOAD(p2), 1, 3);
            break;

          case 0xed:            /* SBC $nnnn */
            SBC(LOAD(p2), 1, 3);
            break;

          case 0xee:            /* INC $nnnn */
            INC(p2, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0xef:            /* ISB $nnnn */
            ISB(p2, 0, CLK_ABS_RMW2, 3, LOAD_ABS, STORE_ABS);
            break;

          case 0xf1:            /* SBC ($nn),Y */
            SBC(LOAD_IND_Y(p1), 1, 2);
            break;

          case 0xf3:            /* ISB ($nn),Y */
            ISB_IND_Y(p1);
            break;

          case 0xf5:            /* SBC $nn,X */
            SBC(LOAD_ZERO_X(p1), CLK_ZERO_I2, 2);
            break;

          case 0xf6:            /* INC $nn,X */
            INC((p1 + reg_x) & 0xff, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xf7:            /* ISB $nn,X */
            ISB((p1 + reg_x) & 0xff, 0, CLK_ZERO_I_RMW, 2, LOAD_ZERO, STORE_ABS);
            break;

          case 0xf8:            /* SED */
            SED();
            break;

          case 0xf9:            /* SBC $nnnn,Y */
            SBC(LOAD_ABS_Y(p2), 1, 3);
            break;

          case 0xfb:            /* ISB $nnnn,Y */
            ISB(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_Y_RMW, STORE_ABS_Y_RMW);
            break;

          case 0xfd:            /* SBC $nnnn,X */
            SBC(LOAD_ABS_X(p2), 1, 3);
            break;

          case 0xfe:            /* INC $nnnn,X */
            INC(p2, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;

          case 0xff:            /* ISB $nnnn,X */
            ISB(p2, 0, CLK_ABS_I_RMW2, 3, LOAD_ABS_X_RMW, STORE_ABS_X_RMW);
            break;
        }
}
/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "MAINCPU";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int maincpu_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
                               ((BYTE)SNAP_MINOR));
    if (m == NULL)
        return -1;

#ifdef C64DTV
    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_A(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_X(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_Y(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_SP(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)MOS6510DTV_REGS_GET_PC(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)MOS6510DTV_REGS_GET_STATUS(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R3(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R4(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R5(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R6(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R7(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R8(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R9(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R10(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R11(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R12(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R13(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R14(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R15(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_ACM(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_YXM(&maincpu_regs)) < 0
        || SMW_BA(m, burst_cache, 4) < 0
        || SMW_W(m, burst_addr) < 0
        || SMW_DW(m, dtvclockneg) < 0
        || SMW_DW(m, (DWORD)last_opcode_info) < 0)
        goto fail;
#else
    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_B(m, MOS6510_REGS_GET_A(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_X(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_Y(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_SP(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)MOS6510_REGS_GET_PC(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)MOS6510_REGS_GET_STATUS(&maincpu_regs)) < 0
        || SMW_DW(m, (DWORD)last_opcode_info) < 0)
        goto fail;
#endif

    if (interrupt_write_snapshot(maincpu_int_status, m) < 0)
        goto fail;

    if (interrupt_write_new_snapshot(maincpu_int_status, m) < 0)
        goto fail;

    return snapshot_module_close(m);

fail:
    if (m != NULL)
        snapshot_module_close(m);
    return -1;
}

int maincpu_snapshot_read_module(snapshot_t *s)
{
    BYTE a, x, y, sp, status;
#ifdef C64DTV
    BYTE r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, acm, yxm;
#endif
    WORD pc;
    BYTE major, minor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);
    if (m == NULL)
        return -1;

    /* FIXME: This is a mighty kludge to prevent VIC-II from stealing the
       wrong number of cycles.  */
    maincpu_rmw_flag = 0;

    /* XXX: Assumes `CLOCK' is the same size as a `DWORD'.  */
    if (0
        || SMR_DW(m, &maincpu_clk) < 0
        || SMR_B(m, &a) < 0
        || SMR_B(m, &x) < 0
        || SMR_B(m, &y) < 0
        || SMR_B(m, &sp) < 0
        || SMR_W(m, &pc) < 0
        || SMR_B(m, &status) < 0
#ifdef C64DTV
        || SMR_B(m, &r3) < 0
        || SMR_B(m, &r4) < 0
        || SMR_B(m, &r5) < 0
        || SMR_B(m, &r6) < 0
        || SMR_B(m, &r7) < 0
        || SMR_B(m, &r8) < 0
        || SMR_B(m, &r9) < 0
        || SMR_B(m, &r10) < 0
        || SMR_B(m, &r11) < 0
        || SMR_B(m, &r12) < 0
        || SMR_B(m, &r13) < 0
        || SMR_B(m, &r14) < 0
        || SMR_B(m, &r15) < 0
        || SMR_B(m, &acm) < 0
        || SMR_B(m, &yxm) < 0
        || SMR_BA(m, burst_cache, 4) < 0
        || SMR_W(m, &burst_addr) < 0
        || SMR_DW_INT(m, &dtvclockneg) < 0
#endif
        || SMR_DW_UINT(m, &last_opcode_info) < 0)
        goto fail;

#ifdef C64DTV
    MOS6510DTV_REGS_SET_A(&maincpu_regs, a);
    MOS6510DTV_REGS_SET_X(&maincpu_regs, x);
    MOS6510DTV_REGS_SET_Y(&maincpu_regs, y);
    MOS6510DTV_REGS_SET_SP(&maincpu_regs, sp);
    MOS6510DTV_REGS_SET_PC(&maincpu_regs, pc);
    MOS6510DTV_REGS_SET_STATUS(&maincpu_regs, status);
    MOS6510DTV_REGS_SET_R3(&maincpu_regs, r3);
    MOS6510DTV_REGS_SET_R4(&maincpu_regs, r4);
    MOS6510DTV_REGS_SET_R5(&maincpu_regs, r5);
    MOS6510DTV_REGS_SET_R6(&maincpu_regs, r6);
    MOS6510DTV_REGS_SET_R7(&maincpu_regs, r7);
    MOS6510DTV_REGS_SET_R8(&maincpu_regs, r8);
    MOS6510DTV_REGS_SET_R9(&maincpu_regs, r9);
    MOS6510DTV_REGS_SET_R10(&maincpu_regs, r10);
    MOS6510DTV_REGS_SET_R11(&maincpu_regs, r11);
    MOS6510DTV_REGS_SET_R12(&maincpu_regs, r12);
    MOS6510DTV_REGS_SET_R13(&maincpu_regs, r13);
    MOS6510DTV_REGS_SET_R14(&maincpu_regs, r14);
    MOS6510DTV_REGS_SET_R15(&maincpu_regs, r15);
    MOS6510DTV_REGS_SET_ACM(&maincpu_regs, acm);
    MOS6510DTV_REGS_SET_YXM(&maincpu_regs, yxm);
#else
    MOS6510_REGS_SET_A(&maincpu_regs, a);
    MOS6510_REGS_SET_X(&maincpu_regs, x);
    MOS6510_REGS_SET_Y(&maincpu_regs, y);
    MOS6510_REGS_SET_SP(&maincpu_regs, sp);
    MOS6510_REGS_SET_PC(&maincpu_regs, pc);
    MOS6510_REGS_SET_STATUS(&maincpu_regs, status);
#endif

    if (interrupt_read_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_read_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL)
        snapshot_module_close(m);
    return -1;
}

