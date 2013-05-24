/*
 * fsimage-gcr.c
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

#include <stdio.h>
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-gcr.h"
#include "fsimage.h"
#include "gcr.h"
#include "log.h"
#include "lib.h"
#include "types.h"
#include "util.h"


static log_t fsimage_gcr_log = LOG_ERR;
static const BYTE gcr_image_header_expected[] =
    { 0x47, 0x43, 0x52, 0x2D, 0x31, 0x35, 0x34, 0x31 };
/* Hardcoded/expected values VICE works with:
 * 0x54   -   84: Maximum container size for (half) track pointers
 * 0x00   -    0: GCR image file version number
 */
static const BYTE gcr_container_sizes_expected [] =
{ 0x00 , 0x54 };

/*-----------------------------------------------------------------------*/
/* Intial GCR buffer setup.  */

int fsimage_read_gcr_image(disk_image_t *image)
{
    unsigned int half_track, num_half_tracks, max_track_length;
    DWORD gcr_track_p[MAX_TRACKS_1541 * 2];
    DWORD gcr_speed_p[MAX_TRACKS_1541 * 2];
    BYTE gcr_image_header[ sizeof( gcr_image_header_expected ) ];
    BYTE gcr_container_sizes[ sizeof(gcr_container_sizes_expected)];
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    num_half_tracks = image->half_tracks;

    /* Do G64 image file sanity checks, current VICE implementation
     * does only support image file version 0 with a fixed track map
     * container size of 84
     */
    fseek(fsimage->fd, 0, SEEK_SET);
    if (fread(gcr_image_header, 1, sizeof( gcr_image_header_expected ), fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }
    if (memcmp( gcr_image_header_expected, gcr_image_header,
                sizeof( gcr_image_header_expected ) ) != 0) {
        log_error(fsimage_gcr_log,
                  "Unexpected GCR header found." );
        return -1;
    }

    fseek(fsimage->fd, 8, SEEK_SET);
    if (fread(gcr_container_sizes, 1, sizeof( gcr_container_sizes_expected ), fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }

    if (memcmp( gcr_container_sizes_expected, gcr_container_sizes,
            sizeof( gcr_container_sizes_expected ) ) != 0) {
        log_error(fsimage_gcr_log,
                  "Unexpected GCR image file constants found, VICE is unable to work with." );
        return -1;
    }

    max_track_length = fsimage_gcr_set_max_track_length(image);
    if (max_track_length < 0) {
        log_error(fsimage_gcr_log, "Could not get max track length.");
        return -1;
    }
    image->gcr->max_track_size = max_track_length;

#ifdef GCR_LOW_MEM
    if (max_track_length > NUM_MAX_MEM_BYTES_TRACK) {
        log_error(fsimage_gcr_log, "Too large max track length.");
        return -1;
    }
#endif

    fseek(fsimage->fd, 12, SEEK_SET);
    if (util_dword_read(fsimage->fd, gcr_track_p, num_half_tracks) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }

    fseek(fsimage->fd, 12 + num_half_tracks * 4, SEEK_SET);
    if (util_dword_read(fsimage->fd, gcr_speed_p, num_half_tracks) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }

    memset(image->gcr->data, 0x00, sizeof(image->gcr->data));
    memset(image->gcr->speed_zone, 0x00, sizeof(image->gcr->speed_zone));

    for (half_track = 0; half_track < MAX_TRACKS_1541 * 2; half_track++) {
        BYTE *track_data, *zone_data;

        track_data = image->gcr->data + half_track * max_track_length;
        zone_data = image->gcr->speed_zone + half_track * max_track_length;
        image->gcr->track_size[half_track] = 6250;

        if (half_track <= num_half_tracks && gcr_track_p[half_track] != 0) {
            BYTE len[2];
            long offset;
            size_t track_len;
            unsigned int zone_len;

            offset = gcr_track_p[half_track];

            if (offset != 0) {
                fseek(fsimage->fd, offset, SEEK_SET);
                if (fread(len, 2, 1, fsimage->fd) < 1) {
                    log_error(fsimage_gcr_log, "Could not read GCR disk image.");
                    return -1;
                }

                track_len = len[0] + len[1] * 256;

                if (track_len > max_track_length) {
                    log_error(fsimage_gcr_log, "Could not read GCR disk image.");
                    return -1;
                }

                image->gcr->track_size[half_track] = (unsigned int)track_len;

                fseek(fsimage->fd, offset + 2, SEEK_SET);
                if (fread(track_data, track_len, 1, fsimage->fd) < 1) {
                    log_error(fsimage_gcr_log, "Could not read GCR disk image.");
                    return -1;
                }

                zone_len = (unsigned int)((track_len + 3) / 4);

                if (gcr_speed_p[half_track] > 3) {
                    unsigned int i;
                    BYTE *comp_speed = (BYTE*) lib_malloc(max_track_length / 4);

                    offset = gcr_speed_p[half_track];

                    fseek(fsimage->fd, offset, SEEK_SET);
                    if (fread(comp_speed, zone_len, 1, fsimage->fd) < 1) {
                        log_error(fsimage_gcr_log,
                                "Could not read GCR disk image.");
                        return -1;
                    }
                    for (i = 0; i < zone_len; i++) {
                        zone_data[i * 4 + 3] = comp_speed[i] & 3;
                        zone_data[i * 4 + 2] = (comp_speed[i] >> 2) & 3;
                        zone_data[i * 4 + 1] = (comp_speed[i] >> 4) & 3;
                        zone_data[i * 4 ] = (comp_speed[i] >> 6) & 3;
                    }

                    lib_free(comp_speed);
                } else {
                    memset(zone_data, gcr_speed_p[half_track], max_track_length);
                }
            }
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Read an entire GCR track from the disk image.  */

int fsimage_gcr_read_half_track(disk_image_t *image, unsigned int half_track,
                                BYTE *gcr_data, int *gcr_track_size)
{
    unsigned int track_len;
    BYTE len[2];
    DWORD gcr_track_p;
    long offset;
    fsimage_t *fsimage;
    unsigned int max_track_length;

    fsimage = image->media.fsimage;

    max_track_length=fsimage_gcr_set_max_track_length(image);
    if (max_track_length < 0) {
        log_error(fsimage_gcr_log, "Could not get max track length.");
        return -1;
    }
#ifdef GCR_LOW_MEM
    if (max_track_length > NUM_MAX_MEM_BYTES_TRACK) {
        log_error(fsimage_gcr_log, "Too large max track length.");
        return -1;
    }
#endif

    if (fsimage->fd == NULL) {
        log_error(fsimage_gcr_log, "Attempt to read without disk image.");
        return -1;
    }

    fseek(fsimage->fd, 12 + (half_track - 2) * 4, SEEK_SET);
    if (util_dword_read(fsimage->fd, &gcr_track_p, 1) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }

    memset(gcr_data, 0xff, max_track_length);
    *gcr_track_size = 6250;

    if (gcr_track_p != 0) {

        offset = gcr_track_p;

        fseek(fsimage->fd, offset, SEEK_SET);
        if (fread(len, 2, 1, fsimage->fd) < 1) {
            log_error(fsimage_gcr_log, "Could not read GCR disk image.");
            return -1;
        }

        track_len = len[0] + len[1] * 256;

        if ((track_len < 1) || (track_len > max_track_length)) {
            log_error(fsimage_gcr_log,
                      "Track field length %i is not supported.",
                      track_len);
            return -1;
        }

        *gcr_track_size = track_len;

        fseek(fsimage->fd, offset + 2, SEEK_SET);
        if (fread(gcr_data, track_len, 1, fsimage->fd) < 1) {
            log_error(fsimage_gcr_log, "Could not read GCR disk image.");
            return -1;
        }
    }
    return 0;
}

int fsimage_gcr_read_track(disk_image_t *image, unsigned int track,
                           BYTE *gcr_data, int *gcr_track_size)
{
    return fsimage_gcr_read_half_track(image, track << 1, gcr_data, gcr_track_size);
}

/*-----------------------------------------------------------------------*/
/* Write an entire GCR track to the disk image.  */

int fsimage_gcr_write_half_track(disk_image_t *image, unsigned int half_track,
                                 int gcr_track_size, BYTE *gcr_speed_zone,
                                 BYTE *gcr_track_start_ptr)
{
    int gap;
	unsigned int i, num_half_tracks, max_track_length;
    BYTE len[2];
    DWORD gcr_track_p[MAX_TRACKS_1541 * 2];
    DWORD gcr_speed_p[MAX_TRACKS_1541 * 2];
    int offset;
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_gcr_log, "Attempt to write without disk image.");
        return -1;
    }

    if (image->read_only != 0) {
        log_error(fsimage_gcr_log,
                  "Attempt to write to read-only disk image.");
        return -1;
    }

    max_track_length = fsimage_gcr_set_max_track_length(image);
    if (max_track_length < 0) {
        log_error(fsimage_gcr_log, "Could not get max track length.");
        return -1;
    }
    image->gcr->max_track_size = max_track_length;

#ifdef GCR_LOW_MEM
    if (max_track_length > NUM_MAX_MEM_BYTES_TRACK) {
        log_error(fsimage_gcr_log, "Too large max track length.");
        return -1;
    }
#endif

    num_half_tracks = image->half_tracks;

    fseek(fsimage->fd, 12, SEEK_SET);
    if (util_dword_read(fsimage->fd, gcr_track_p, num_half_tracks) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image header.");
        return -1;
    }

    fseek(fsimage->fd, 12 + num_half_tracks * 4, SEEK_SET);
    if (util_dword_read(fsimage->fd, gcr_speed_p, num_half_tracks) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image header.");
        return -1;
    }

    if (gcr_track_p[half_track - 2] == 0) {
        offset = fseek(fsimage->fd, 0, SEEK_END);
        if (offset < 0) {
            log_error(fsimage_gcr_log, "Could not extend GCR disk image.");
            return -1;
        }
        gcr_track_p[half_track - 2] = offset;
    }

    offset = gcr_track_p[half_track - 2];

    len[0] = gcr_track_size % 256;
    len[1] = gcr_track_size / 256;

    if (fseek(fsimage->fd, offset, SEEK_SET) < 0
        || fwrite(len, 2, 1, fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not write GCR disk image.");
        return -1;
    }

    /* Clear gap between the end of the actual track and the start of
       the next track.  */
    gap = image->gcr->max_track_size - gcr_track_size;
    if (gap > 0)
        memset(gcr_track_start_ptr + gcr_track_size, 0, gap);

    if (fseek(fsimage->fd, offset + 2, SEEK_SET) < 0
        || fwrite(gcr_track_start_ptr, image->gcr->max_track_size, 1,
        fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not write GCR disk image.");
        return -1;
    }

    if (gcr_speed_zone != NULL) {
        for (i = 0; (gcr_speed_zone[(half_track - 2) * max_track_length]
            == gcr_speed_zone[(half_track - 2) * max_track_length + i])
            && i < max_track_length; i++);

        if (i < (unsigned int)gcr_track_size) {
            /* This will change soon.  */
            log_error(fsimage_gcr_log,
                      "Saving different speed zones is not supported yet.");
            return -1;
        }

        if (gcr_speed_p[half_track - 2] >= 4) {
            /* This will change soon.  */
            log_error(fsimage_gcr_log,
                      "Adding new speed zones is not supported yet.");
            return -1;
        }

        offset = 12 + (num_half_tracks * 4) + ((half_track - 2) * 4);
        if (fseek(fsimage->fd, offset, SEEK_SET) < 0
            || util_dword_write(fsimage->fd, &gcr_speed_p[half_track - 2], 1)
            < 0) {
            log_error(fsimage_gcr_log, "Could not write GCR disk image.");
            return -1;
        }
    }

#if 0  /* We do not support writing different speeds yet.  */
    for (i = 0; i < (max_track_length / 4); i++)
        zone_len = (gcr_track_size + 3) / 4;
    zone_data = gcr_speed_zone + (half_track - 2) * max_track_length;

    if (gap > 0)
        memset(zone_data + gcr_track_size, 0, gap);

    for (i = 0; i < (max_track_length / 4); i++)
        comp_speed[i] = (zone_data[i * 4]
                         | (zone_data[i * 4 + 1] << 2)
                         | (zone_data[i * 4 + 2] << 4)
                         | (zone_data[i * 4 + 3] << 6));

    if (fseek(fsimage->fd, offset, SEEK_SET) < 0
        || fwrite((char *)comp_speed, max_track_length / 4, 1
        fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not write GCR disk image");
        return;
    }
#endif

    /* Make sure the stream is visible to other readers.  */
    fflush(fsimage->fd);

    return 0;
}

int fsimage_gcr_write_track(disk_image_t *image, unsigned int track,
                            int gcr_track_size, BYTE *gcr_speed_zone,
                            BYTE *gcr_track_start_ptr)
{
  return fsimage_gcr_write_track(image, track << 1, gcr_track_size, gcr_speed_zone, gcr_track_start_ptr);
}

/*-----------------------------------------------------------------------*/
/* Read a sector from the GCR disk image.  */

int fsimage_gcr_read_sector(disk_image_t *image, BYTE *buf,
                               unsigned int track, unsigned int sector)
{
    unsigned int max_track_length;
    BYTE *gcr_data;
    BYTE *gcr_track_start_ptr;
    int gcr_track_size, gcr_current_track_size;

    max_track_length=fsimage_gcr_set_max_track_length(image);
    if (max_track_length < 0) {
        log_error(fsimage_gcr_log, "Could not get max track length.");
        return -1;
    }

    if (track > image->tracks) {
        log_error(fsimage_gcr_log,
                  "Track %i out of bounds.  Cannot read GCR track.",
                  track);
        return -1;
    }

    gcr_data = (BYTE*) lib_malloc(max_track_length);

    if (image->gcr == NULL) {
        if (fsimage_gcr_read_track(image, track, gcr_data,
            &gcr_track_size) < 0) {
            log_error(fsimage_gcr_log,
                      "Cannot read track %i from GCR image.", track);
            lib_free(gcr_data);
            return -1;
        }
        gcr_track_start_ptr = gcr_data;
        gcr_current_track_size = gcr_track_size;
    } else {
        gcr_track_start_ptr = image->gcr->data
                              + (((track << 1) - 2) * max_track_length);
        gcr_current_track_size = image->gcr->track_size[(track << 1) - 2];
    }
    if (gcr_read_sector(gcr_track_start_ptr, gcr_current_track_size,
        buf, track, sector) < 0) {
        log_error(fsimage_gcr_log,
                  "Cannot find track: %i sector: %i within GCR image.",
                  track, sector);
        lib_free(gcr_data);
        return -1;
    }

    lib_free(gcr_data);

    return 0;
}


/*-----------------------------------------------------------------------*/
/* Write a sector to the GCR disk image.  */

int fsimage_gcr_write_sector(disk_image_t *image, BYTE *buf,
                                unsigned int track, unsigned int sector)
{
    unsigned int max_track_length;
    BYTE *gcr_data;
    BYTE *gcr_track_start_ptr, *speed_zone;
    int gcr_track_size, gcr_current_track_size;

    max_track_length=fsimage_gcr_set_max_track_length(image);
    if (max_track_length < 0) {
        log_error(fsimage_gcr_log, "Could not get max track length.");
        return -1;
    }

    if (track > image->tracks) {
        log_error(fsimage_gcr_log,
                  "Track %i out of bounds.  Cannot write GCR sector",
                  track);
        return -1;
    }

    gcr_data = (BYTE*) lib_malloc(max_track_length);

    if (image->gcr == NULL) {
        if (fsimage_gcr_read_track(image, track, gcr_data,
            &gcr_track_size) < 0) {
            log_error(fsimage_gcr_log,
                      "Cannot read track %i from GCR image.", track);
            lib_free(gcr_data);
            return -1;
        }
        gcr_track_start_ptr = gcr_data;
        gcr_current_track_size = gcr_track_size;
        speed_zone = NULL;
    } else {
        gcr_track_start_ptr = image->gcr->data
                              + (((track << 1) - 2) * max_track_length);
        gcr_current_track_size = image->gcr->track_size[(track << 1) - 2];
        speed_zone = image->gcr->speed_zone;
    }
    if (gcr_write_sector(gcr_track_start_ptr,
        gcr_current_track_size, buf, track, sector) < 0) {
        log_error(fsimage_gcr_log,
                  "Could not find track %i sector %i in disk image",
                  track, sector);
        lib_free(gcr_data);
        return -1;
    }
    if (disk_image_write_track(image, track, gcr_current_track_size,
        speed_zone, gcr_track_start_ptr) < 0) {
        log_error(fsimage_gcr_log,
                  "Failed writing track %i to disk image.", track);
        lib_free(gcr_data);
        return -1;
    }

    lib_free(gcr_data);

    return 0;
}

/*-----------------------------------------------------------------------*/

void fsimage_gcr_init(void)
{
    fsimage_gcr_log = log_open("Filesystem Image GCR");
}

/*-----------------------------------------------------------------------*/
/* Read in the max track length from the .g64 header					 */

int fsimage_gcr_set_max_track_length(disk_image_t *image)
{
    fsimage_t *fsimage;
    BYTE temp_len[2];
    size_t max_track_len;

    fsimage = image->media.fsimage;

    fseek(fsimage->fd, 10, SEEK_SET);
    if (fread(temp_len, 2, 1, fsimage->fd) < 1) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }

    max_track_len = temp_len[0] + temp_len[1] * 256;

    return max_track_len;
}
