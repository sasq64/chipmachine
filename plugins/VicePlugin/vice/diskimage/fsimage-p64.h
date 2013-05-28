/*
 * fsimage-p64.h
 *
 * Written by
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

#ifndef VICE_FSIMAGE_P64_H
#define VICE_FSIMAGE_P64_H

#include "types.h"

struct disk_image_s;

extern void fsimage_p64_init(void);

extern int fsimage_read_p64_image(disk_image_t *image);

extern int fsimage_write_p64_image(disk_image_t *image);

extern int fsimage_p64_read_half_track(struct disk_image_s *image,
                                       unsigned int half_track, BYTE *gcr_data,
                                       int *gcr_track_size);
extern int fsimage_p64_read_track(struct disk_image_s *image,
                                  unsigned int track, BYTE *gcr_data,
                                  int *gcr_track_size);
extern int fsimage_p64_write_half_track(struct disk_image_s *image,
                                        unsigned int half_track, int gcr_track_size,
                                        BYTE *gcr_speed_zone,
                                        BYTE *gcr_track_start_ptr);
extern int fsimage_p64_write_track(struct disk_image_s *image,
                                   unsigned int track, int gcr_track_size,
                                   BYTE *gcr_speed_zone,
                                   BYTE *gcr_track_start_ptr);
extern int fsimage_p64_read_sector(struct disk_image_s *image, BYTE *buf,
                                   unsigned int track, unsigned int sector);
extern int fsimage_p64_write_sector(struct disk_image_s *image, BYTE *buf,
                                    unsigned int track, unsigned int sector);
#endif

