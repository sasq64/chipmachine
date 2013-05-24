/*
 * c64model.h - C64 model detection and setting.
 *
 * Written by
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

#ifndef VICE_C64MODEL_H
#define VICE_C64MODEL_H

#include "types.h"

#define C64MODEL_C64_PAL     0
#define C64MODEL_C64C_PAL    1
#define C64MODEL_C64_OLD_PAL 2

#define C64MODEL_C64_NTSC     3
#define C64MODEL_C64C_NTSC    4
#define C64MODEL_C64_OLD_NTSC 5

#define C64MODEL_C64_PAL_N 6

#define C64MODEL_NUM 7

#define C64MODEL_UNKNOWN 99

extern int c64model_get(void);
extern int c64model_get_temp(int vicii_model, int sid_model, int glue_logic,
                             int cia1_model, int cia2_model, int new_luma);
extern void c64model_set(int model);
extern void c64model_set_temp(int model, int *vicii_model, int *sid_model,
                              int *glue_logic, int *cia1_model, int *cia2_model,
                              int *new_luma);

#endif
