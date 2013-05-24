/*
 * sounddummy.c - Implementation of the dummy sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "sound.h"

/* from our maincpu.c */
extern short *psid_sound_buf; /* sound buffer, stereo, 44100, 16 bit, big-endian */
extern int psid_sound_idx; /* current */
extern int psid_sound_max; /* max permitted */

/* Keep some overflow area to deal with filled buffer. */
#define OVERFLOW_SIZE 4096
static short overflow[OVERFLOW_SIZE];
static int overflow_max = 0;

static int channels;

static int dummy_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels_arg) {
	channels = *channels_arg;
	return 0;
}

static int dummy_write(SWORD *pbuf, size_t nr)
{
    /* Move data left over from previous overflow to start of new buffer. */
    int overflow_idx = 0;
    while (overflow_idx != overflow_max &&
		psid_sound_idx != psid_sound_max) {
	if (channels == 1) {
		psid_sound_buf[psid_sound_idx ++] = overflow[overflow_idx];
	}
	psid_sound_buf[psid_sound_idx ++] = overflow[overflow_idx ++];
    }
    /* This would happen if VICE generates larger buffer at once than
     * caller is requesting at any time. Therefore, every attempt to
     * enter the mainloop results in overflow. We let sound fail in
     * that case. */
    if (overflow_idx != overflow_max) {
	return overflow_max - overflow_idx + nr;
    }
    /* All copied (or none). */
    overflow_max = 0;

    /* Copy nr buffers to our buffer, or overflow if all doesn't fit */
    int pbuf_idx = 0;
    while (nr != 0 && psid_sound_idx != psid_sound_max) {
	if (channels == 1) {
		psid_sound_buf[psid_sound_idx ++] = pbuf[pbuf_idx];
	}
	psid_sound_buf[psid_sound_idx ++] = pbuf[pbuf_idx ++];
	nr --;
    }
    /* Copy to overflow. */
    while (nr != 0 && overflow_max != OVERFLOW_SIZE) {
	overflow[overflow_max ++] = pbuf[pbuf_idx ++];
	nr --;
    }

    /* Should be 0, or fail. */
    return nr;
}

static sound_device_t dummy_device =
{
    "dummy",
    dummy_init,
    dummy_write,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0
};

int sound_init_dummy_device(void)
{
    return sound_register_device(&dummy_device);
}
