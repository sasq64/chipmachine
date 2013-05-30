/*
 * plus4model.h - Plus4 model detection and setting.
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

#ifndef VICE_PLUS4MODEL_H
#define VICE_PLUS4MODEL_H

#include "types.h"

#define PLUS4MODEL_C16_PAL        0 /* C16/C116 (PAL)*/
#define PLUS4MODEL_C16_NTSC       1 /* C16/C116 (NTSC)*/
#define PLUS4MODEL_PLUS4_PAL      2 /* Plus4 (PAL) */
#define PLUS4MODEL_PLUS4_NTSC     3 /* Plus4/C264 (NTSC) */
#define PLUS4MODEL_V364_NTSC      4 /* V364 (NTSC) */
#define PLUS4MODEL_232_NTSC       5 /* C232 (NTSC) */

#define PLUS4MODEL_NUM 6

#define PLUS4MODEL_UNKNOWN 99

extern int plus4model_get(void);
extern int plus4model_get_temp(int video, int ramsize, int hasspeech, int hasacia);
extern void plus4model_set(int model);
extern void plus4model_set_temp(int model, int *ted_model, int *ramsize, int *hasspeech, int *hasacia);

#endif
