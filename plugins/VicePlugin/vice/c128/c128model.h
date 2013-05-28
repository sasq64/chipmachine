/*
 * c128model.h - C64 model detection and setting.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_C128MODEL_H
#define VICE_C128MODEL_H

#include "types.h"

#define C128MODEL_C128_PAL      0
#define C128MODEL_C128DCR_PAL   1

#define C128MODEL_C128_NTSC     2
#define C128MODEL_C128DCR_NTSC  3

#define C128MODEL_NUM 4

#define C128MODEL_UNKNOWN 99

extern int c128model_get(void);
extern int c128model_get_temp(int vicii_model, int sid_model, int vdc_revision,
                             int vdc_64k, int cia1_model, int cia2_model);
extern void c128model_set(int model);
extern void c128model_set_temp(int model, int *vicii_model, int *sid_model,
                              int *vdc_revision, int *vdc_64k, int *cia1_model, 
                              int *cia2_model);

#endif
