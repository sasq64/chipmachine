/*
 * petcpu.c - Emulation of the main 6502 processor.
 *
 * Written by Olaf Seibert.
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

#include "maincpu.h"
#include "mem.h"
#include "pets.h"
#include "6809.h"

/* ------------------------------------------------------------------------- */

/* MACHINE_STUFF should define/undef

 - NEED_REG_PC
 - TRACE

 The following are optional:

 - PAGE_ZERO
 - PAGE_ONE
 - STORE_IND
 - LOAD_IND
 - DMA_FUNC
 - DMA_ON_RESET

*/

#define DMA_FUNC h6809_mainloop(CPU_INT_STATUS, ALARM_CONTEXT)

#define DMA_ON_RESET                       \
    while (petres.superpet &&              \
        petres.superpet_cpu_switch == SUPERPET_CPU_6809) {  \
        EXPORT_REGISTERS();                \
        cpu6809_reset();                   \
        DMA_FUNC;                          \
        interrupt_ack_dma(CPU_INT_STATUS); \
        IMPORT_REGISTERS();                \
    }

#define HAVE_6809_REGS

#include "../maincpu.c"
