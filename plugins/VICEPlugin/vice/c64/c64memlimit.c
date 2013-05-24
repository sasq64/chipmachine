/*
 * c64memlimit.c -- Builds the C64 memory limit table.
 *
 * Written by
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

#include "c64memlimit.h"


#define NUM_SEGMENTS 8
#define NUM_CONFIGS 32

static const int mstart[NUM_SEGMENTS] = { 0x00, 0x01, 0x10, 0x80,
                                          0xa0, 0xc0, 0xd0, 0xe0 };

static const int mend[NUM_SEGMENTS] = { 0x00, 0x0f, 0x7f, 0x9f,
                                        0xbf, 0xcf, 0xdf, 0xff };

static const int limit_tab[NUM_SEGMENTS][NUM_CONFIGS] = {
    /* 0000-00ff */
    {     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1 },

    /* 0100-0fff */
    { 0xfffd, 0xcffd, 0xcffd, 0x9ffd, 0xfffd, 0xcffd, 0xcffd, 0x9ffd,
      0xfffd, 0xcffd, 0x9ffd, 0x7ffd, 0xfffd, 0xcffd, 0x9ffd, 0x7ffd,
      0x0ffd, 0x0ffd, 0x0ffd, 0x0ffd, 0x0ffd, 0x0ffd, 0x0ffd, 0x0ffd,
      0xfffd, 0xcffd, 0x9ffd, 0x7ffd, 0xfffd, 0xcffd, 0x9ffd, 0x7ffd },

    /* 1000-7fff */
    { 0xfffd, 0xcffd, 0xcffd, 0x9ffd, 0xfffd, 0xcffd, 0xcffd, 0x9ffd,
      0xfffd, 0xcffd, 0x9ffd, 0x7ffd, 0xfffd, 0xcffd, 0x9ffd, 0x7ffd,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xcffd, 0x9ffd, 0x7ffd, 0xfffd, 0xcffd, 0x9ffd, 0x7ffd },

    /* 8000-9fff */
    { 0xfffd, 0xcffd, 0xcffd, 0x9ffd, 0xfffd, 0xcffd, 0xcffd, 0x9ffd,
      0xfffd, 0xcffd, 0x9ffd,     -1, 0xfffd, 0xcffd, 0x9ffd,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xcffd, 0x9ffd,     -1, 0xfffd, 0xcffd, 0x9ffd,     -1 },

    /* a000-bfff */
    { 0xfffd, 0xcffd, 0xcffd, 0xbffd, 0xfffd, 0xcffd, 0xcffd, 0xbffd,
      0xfffd, 0xcffd,     -1,     -1, 0xfffd, 0xcffd,     -1,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xcffd,     -1,     -1, 0xfffd, 0xcffd,     -1,     -1 },

    /* c000-cfff */
    { 0xfffd, 0xcffd, 0xcffd, 0xcffd, 0xfffd, 0xcffd, 0xcffd, 0xcffd,
      0xfffd, 0xcffd, 0xcffd, 0xcffd, 0xfffd, 0xcffd, 0xcffd, 0xcffd,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xcffd, 0xcffd, 0xcffd, 0xfffd, 0xcffd, 0xcffd, 0xcffd },

    /* d000-dfff */
    { 0xfffd, 0xdffd, 0xdffd, 0xdffd, 0xfffd,     -1,     -1,     -1,
      0xfffd, 0xdffd, 0xdffd, 0xdffd, 0xfffd,     -1,     -1,     -1,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xdffd, 0xdffd, 0xdffd, 0xfffd,     -1,     -1,     -1 },

    /* e000-ffff */
    { 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
      0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
          -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd } };


void mem_limit_init(int mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                mem_read_limit_tab[i][k] = limit_tab[j][i];
            }
        }
        mem_read_limit_tab[i][0x100] = -1;
    }
}

void mem_limit_plus60k_init(int mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                if (k < 0x10) {
                    mem_read_limit_tab[i][k] = limit_tab[j][i];
                } else {
                    mem_read_limit_tab[i][k] = -1;
                }
            }
        }
        mem_read_limit_tab[i][0x100] = -1;
    }
}

void mem_limit_256k_init(int mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                mem_read_limit_tab[i][k] = -1;
            }
        }
        mem_read_limit_tab[i][0x100] = -1;
    }
}
