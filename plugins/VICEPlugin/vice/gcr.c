/*
 * gcr.c - GCR handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
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

/* #define DEBUG_GCR */

#ifdef DEBUG_GCR
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gcr.h"
#include "lib.h"
#include "log.h"
#include "types.h"


static const BYTE GCR_conv_data[16] =
    { 0x0a, 0x0b, 0x12, 0x13,
      0x0e, 0x0f, 0x16, 0x17,
      0x09, 0x19, 0x1a, 0x1b,
      0x0d, 0x1d, 0x1e, 0x15 };

static const BYTE From_GCR_conv_data[32] =
    { 0,  0,  0,  0,  0,  0,  0,  0,
      0,  8,  0,  1,  0, 12,  4,  5,
      0,  0,  2,  3,  0, 15,  6,  7,
      0,  9, 10, 11,  0, 13, 14,  0 };


static void gcr_convert_4bytes_to_GCR(BYTE *source, BYTE *dest)
{
    int i;
    register unsigned int tdest = 0;    /* at least 16 bits for overflow shifting */

    for (i = 2; i < 10; i += 2, source++, dest++)
    {
        tdest <<= 5;  /* make room for the upper nybble */
        tdest  |= GCR_conv_data[(*source) >> 4];

        tdest <<= 5;  /* make room for the lower nybble */
        tdest  |= GCR_conv_data[(*source) & 0x0f];

        *dest   = (BYTE)(tdest >> i);
    }

    *dest   = (BYTE)tdest;
}

static void gcr_convert_GCR_to_4bytes(BYTE *source, BYTE *dest)
{
    int i;
        /* at least 24 bits for shifting into bits 16...20 */
    register DWORD tdest = *source;

    tdest <<= 13;

    for (i = 5; i < 13; i += 2, dest++)
    {
        source++;
        tdest  |= ((DWORD)(*source)) << i;

            /*  "tdest >> 16" could be optimized to a word
             *  aligned access, hopefully the compiler does
             *  this for us (in a portable way)
             */
        *dest   = From_GCR_conv_data[ (tdest >> 16) & 0x1f ] << 4;
        tdest <<= 5;

        *dest  |= From_GCR_conv_data[ (tdest >> 16) & 0x1f ];
        tdest <<= 5;
    }
}

void gcr_convert_sector_to_GCR(BYTE *buffer, BYTE *ptr, unsigned int track,
                               unsigned int sector, BYTE diskID1, BYTE diskID2,
                               BYTE error_code)
{
    int i;
    BYTE buf[4], header_id1;

    header_id1 = (error_code == 29) ? diskID1 ^ 0xff : diskID1;

    memset(ptr, 0xff, 5);       /* Sync */
    ptr += 5;

    buf[0] = (error_code == 20) ? 0xff : 0x08;
    buf[1] = sector ^ track ^ diskID2 ^ header_id1;
    buf[2] = sector;
    buf[3] = track;

    if (error_code == 27)
        buf[1] ^= 0xff;

    gcr_convert_4bytes_to_GCR(buf, ptr);
    ptr += 5;

    buf[0] = diskID2;
    buf[1] = header_id1;
    buf[2] = buf[3] = 0x0f;
    gcr_convert_4bytes_to_GCR(buf, ptr);
    ptr += 5;

    ptr += 9;

    memset(ptr, 0xff, 5);       /* Sync */
    ptr += 5;

    for (i = 0; i < 65; i++) {
        gcr_convert_4bytes_to_GCR(buffer, ptr);
        buffer += 4;
        ptr += 5;
    }
}

void gcr_convert_GCR_to_sector(BYTE *buffer, BYTE *ptr,
                               BYTE *GCR_track_start_ptr,
                               unsigned int GCR_current_track_size)
{
    BYTE *offset = ptr;
    BYTE *GCR_track_end = GCR_track_start_ptr + GCR_current_track_size;
    BYTE GCR_header[6];
    int i, j, s, shift;

    /* additional 1 bits are part of the previous sync and must
       be shifted out. so check/count these here */
    shift = 0;
    i = *(offset);
    while (i & 0x80) {
        i <<= 1;
        shift++;
    }

    for (i = 0; i < 65; i++) {
        /* get 5 bytes of gcr data */
        for (j = 0; j < 5; j++) {
            GCR_header[j] = *(offset++);
            if (offset >= GCR_track_end) {
                offset = GCR_track_start_ptr;
            }
        }
        /* if the gcr data is not aligned, shift accordingly */
        if (shift) {
            GCR_header[5] = *(offset);
            for (s = 0; s < shift; s++) {
                for (j = 0; j < 5; j++) {
                    GCR_header[j] <<= 1;
                    if (GCR_header[j + 1] & 0x80) {
                        GCR_header[j] |= 1;
                    }
                }
                GCR_header[5] <<= 1;
            }
        }
        gcr_convert_GCR_to_4bytes(GCR_header, buffer);
        buffer += 4;
    }
}

BYTE *gcr_find_sector_header(unsigned int track, unsigned int sector,
                             BYTE *gcr_track_start_ptr,
                             unsigned int gcr_current_track_size)
{
    BYTE *offset = gcr_track_start_ptr;
    BYTE *GCR_track_end = gcr_track_start_ptr + gcr_current_track_size;
    BYTE GCR_header[6], header_data[4];
    int i, wrap_over = 0, shift;
    unsigned int sync_count;

    sync_count = 0;

    while ((offset < GCR_track_end) && !wrap_over) {
        /* find next sync start */
        while (*offset != 0xff) {
            offset++;
            if (offset >= GCR_track_end)
                return NULL;
        }
        /* skip to sync end */
        while (*offset == 0xff) {
            offset++;
            if (offset == GCR_track_end) {
                offset = gcr_track_start_ptr;
                wrap_over = 1;
            }
            /* Check for killer tracks.  */
            if ((++sync_count) >= gcr_current_track_size)
                return NULL;
        }
        /* get next 5(+1) gcr bytes, which are the header */
        for (i = 0; i < 5; i++) {
            GCR_header[i] = *(offset++);
            if (offset >= GCR_track_end) {
                offset = gcr_track_start_ptr;
                wrap_over = 1;
            }
        }
        GCR_header[5] = *(offset);
        /* shift out additional 1 bits, which are part of the sync */
        shift = 0;
        while (GCR_header[0] & 0x80) {
            for (i = 0; i < 5; i++) {
                GCR_header[i] <<= 1;
                if (GCR_header[i + 1] & 0x80) {
                    GCR_header[i] |= 1;
                }
            }
            GCR_header[5] <<= 1;
            shift++;
        }

        gcr_convert_GCR_to_4bytes(GCR_header, header_data);
        if (header_data[0] == 0x08) {
            /* FIXME: Add some sanity checks here.  */
            if (header_data[2] == sector && header_data[3] == track) {
                DBG(("GCR: shift: %d hdr: %02x %02x sec:%02d trk:%02d", shift, header_data[0], header_data[1], header_data[2], header_data[3]));
                if (shift) {
                    log_warning(LOG_DEFAULT,"GCR data is not byte aligned (trk %d sec %d)", header_data[3], header_data[2]);
                }
                return offset;
            }
        }
    }
    return NULL;
}

BYTE *gcr_find_sector_data(BYTE *offset,
                           BYTE *gcr_track_start_ptr,
                           unsigned int gcr_current_track_size)
{
    BYTE *GCR_track_end = gcr_track_start_ptr + gcr_current_track_size;
    int header = 0;

    while (*offset != 0xff) {
        offset++;
        if (offset >= GCR_track_end)
            offset = gcr_track_start_ptr;
        header++;
        if (header >= 500)
            return NULL;
    }

    while (*offset == 0xff) {
        offset++;
        if (offset == GCR_track_end)
            offset = gcr_track_start_ptr;
    }
    return offset;
}

int gcr_read_sector(BYTE *gcr_track_start_ptr,
                    unsigned int gcr_current_track_size, BYTE *readdata,
                    unsigned int track, unsigned int sector)
{
    BYTE buffer[260], *offset;

    offset = gcr_find_sector_header(track, sector,
                                    gcr_track_start_ptr,
                                    gcr_current_track_size);
    if (offset == NULL)
        return -1;

    offset = gcr_find_sector_data(offset, gcr_track_start_ptr,
                                  gcr_current_track_size);
    if (offset == NULL)
        return -1;

    gcr_convert_GCR_to_sector(buffer, offset, gcr_track_start_ptr,
                              gcr_current_track_size);
    if (buffer[0] != 0x7)
        return -1;

    memcpy(readdata, &buffer[1], 256);
    return 0;
}

int gcr_write_sector(BYTE *gcr_track_start_ptr,
                     unsigned int gcr_current_track_size, BYTE *writedata,
                     unsigned int track, unsigned int sector)
{
    BYTE buffer[260], gcr_buffer[325], *offset, *buf, *gcr_data;
    BYTE chksum;
    int i;

    offset = gcr_find_sector_header(track, sector,
                                    gcr_track_start_ptr,
                                    gcr_current_track_size);
    if (offset == NULL)
        return -1;
    offset = gcr_find_sector_data(offset, gcr_track_start_ptr,
                                  gcr_current_track_size);
    if (offset == NULL)
        return -1;
    buffer[0] = 0x7;
    memcpy(&buffer[1], writedata, 256);
    chksum = buffer[1];
    for (i = 2; i < 257; i++)
        chksum ^= buffer[i];
    buffer[257] = chksum;
    buffer[258] = buffer[259] = 0;

    buf = buffer;
    gcr_data = gcr_buffer;

    for (i = 0; i < 65; i++) {
        gcr_convert_4bytes_to_GCR(buf, gcr_data);
        buf += 4;
        gcr_data += 5;
    }

    for (i = 0; i < 325; i++) {
        *offset = gcr_buffer[i];
        offset++;
        if (offset == gcr_track_start_ptr + gcr_current_track_size)
            offset = gcr_track_start_ptr;
    }
    return 0;
}

gcr_t *gcr_create_image(void)
{
    return (gcr_t *)lib_calloc(1, sizeof(gcr_t));
}

void gcr_destroy_image(gcr_t *gcr)
{
    lib_free(gcr);
    return;
}

