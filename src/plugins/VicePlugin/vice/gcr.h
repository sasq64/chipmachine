/*
 * grc.h - GCR handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 * Additional changes by
 *  Robert McIntyre <rjmcinty@hotmail.com>
 *  Benjamin 'BeRo' Rosseaux <benjamin@rosseaux.com>
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

#ifndef VICE_GCR_H
#define VICE_GCR_H

#include "types.h"

/* Number of bytes in one raw track. For usage with D64/D71 */
#define NUM_MAX_BYTES_TRACK 7928

/* Number of bytes in one raw track in memory. 64k big to avoid buffer overrun, because
 * the G64 track size field is a 16-bit word */
#ifdef GCR_LOW_MEM
#define NUM_MAX_MEM_BYTES_TRACK 10240
#else
#define NUM_MAX_MEM_BYTES_TRACK 65536
#endif

/* Number of tracks we emulate. 84 for 1541, 140 for 1571 */
#define MAX_GCR_TRACKS 140

typedef struct gcr_s {
    /* Raw GCR image of the disk.  */
    /* RJM: This is a hack.  Need to dynamically allocate it */
    BYTE data[(MAX_GCR_TRACKS + 1) * NUM_MAX_MEM_BYTES_TRACK];

    /* Speed zone image of the disk.  */
    /* RJM: This is a hack.  Need to dynamically allocate it */
    BYTE speed_zone[(MAX_GCR_TRACKS + 1) * NUM_MAX_MEM_BYTES_TRACK];

    /* Size of the GCR data of each track.  */
    unsigned int track_size[MAX_GCR_TRACKS];

    /* Size of the largest track, set from the file header */
    unsigned int max_track_size;

} gcr_t;

extern void gcr_convert_sector_to_GCR(BYTE *buffer, BYTE *ptr,
                                      unsigned int track, unsigned int sector,
                                      BYTE diskID1, BYTE diskID2,
                                      BYTE error_code);
extern void gcr_convert_GCR_to_sector(BYTE *buffer, BYTE *ptr,
                                      BYTE *GCR_track_start_ptr,
                                      unsigned int GCR_current_track_size);

extern BYTE *gcr_find_sector_header(unsigned int track, unsigned int sector,
                                    BYTE *gcr_track_start_ptr,
                                    unsigned int gcr_current_track_size);
extern BYTE *gcr_find_sector_data(BYTE *offset,
                                  BYTE *gcr_track_start_ptr,
                                  unsigned int gcr_current_track_size);
extern int gcr_read_sector(BYTE *gcr_track_start_ptr,
                           unsigned int gcr_current_track_size, BYTE *readdata,
                           unsigned int track, unsigned int sector);
extern int gcr_write_sector(BYTE *gcr_track_start_ptr,
                            unsigned int gcr_current_track_size,
                            BYTE *writedata,
                            unsigned int track, unsigned int sector);

extern gcr_t *gcr_create_image(void);
extern void gcr_destroy_image(gcr_t *gcr);
#endif

