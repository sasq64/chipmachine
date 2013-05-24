/*
 * fsimage.c
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
#include <stdlib.h>

#include "archdep.h"
#include "cbmdos.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-gcr.h"
#include "fsimage-p64.h"
#include "fsimage-probe.h"
#include "fsimage.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "x64.h"
#include "zfile.h"


static log_t fsimage_log = LOG_DEFAULT;


void fsimage_name_set(disk_image_t *image, char *name)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    fsimage->name = name;
}

char *fsimage_name_get(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    return fsimage->name;
}

void *fsimage_fd_get(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    return (void *)(fsimage->fd);
}

/*-----------------------------------------------------------------------*/

void fsimage_error_info_create(fsimage_t *fsimage)
{
    fsimage->error_info = lib_calloc(1, MAX_BLOCKS_ANY);
}

void fsimage_error_info_destroy(fsimage_t *fsimage)
{
    lib_free(fsimage->error_info);
    fsimage->error_info = NULL;
}

/*-----------------------------------------------------------------------*/

void fsimage_media_create(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = lib_calloc(1, sizeof(fsimage_t));

    image->media.fsimage = fsimage;
}

void fsimage_media_destroy(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    lib_free(fsimage->name);
    fsimage_error_info_destroy(fsimage);

    lib_free(fsimage);
}

/*-----------------------------------------------------------------------*/

int fsimage_open(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (image->read_only) {
        fsimage->fd = zfile_fopen(fsimage->name, MODE_READ);
    } else  {
        fsimage->fd = zfile_fopen(fsimage->name, MODE_READ_WRITE);

        /* If we cannot open the image read/write, try to open it read only. */
        if (fsimage->fd == NULL) {
            fsimage->fd = zfile_fopen(fsimage->name, MODE_READ);
            image->read_only = 1;
        }
    }

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Cannot open file `%s'.", fsimage->name);
        return -1;
    }

    if (fsimage_probe(image) == 0) {
        return 0;
    }

    zfile_fclose(fsimage->fd);
    log_message(fsimage_log, "Unknown disk image `%s'.", fsimage->name);
    return -1;
}

int fsimage_close(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Cannot close file `%s'.",  fsimage->name);
        return -1;
    }

/*   if (image->type == DISK_IMAGE_TYPE_P64) {
	fsimage_write_p64_image(image);
    }*/
    
    zfile_fclose(fsimage->fd);

    fsimage_error_info_destroy(fsimage);

    return 0;
}

/*-----------------------------------------------------------------------*/

int fsimage_read_sector(disk_image_t *image, BYTE *buf, unsigned int track,
                        unsigned int sector)
{
    int sectors;
    long offset;
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Attempt to read without disk image.");
        return 74;
    }

    switch (image->type) {
      case DISK_IMAGE_TYPE_D64:
      case DISK_IMAGE_TYPE_D67:
      case DISK_IMAGE_TYPE_D71:
      case DISK_IMAGE_TYPE_D81:
      case DISK_IMAGE_TYPE_D80:
      case DISK_IMAGE_TYPE_D82:
      case DISK_IMAGE_TYPE_X64:
      case DISK_IMAGE_TYPE_D1M:
      case DISK_IMAGE_TYPE_D2M:
      case DISK_IMAGE_TYPE_D4M:
        sectors = disk_image_check_sector(image, track, sector);

        if (sectors < 0) {
            log_error(fsimage_log, "Track %i, Sector %i out of bounds.",
                      track, sector);
            return 66;
        }

        offset = sectors << 8;

        if (image->type == DISK_IMAGE_TYPE_X64)
            offset += X64_HEADER_LENGTH;

        fseek(fsimage->fd, offset, SEEK_SET);

        if (fread((char *)buf, 256, 1, fsimage->fd) < 1) {
            log_error(fsimage_log,
                      "Error reading T:%i S:%i from disk image.",
                      track, sector);
            return -1;
        }

        if (fsimage->error_info != NULL) {
            switch (fsimage->error_info[sectors]) {
              case 0x0:
              case 0x1:
                return CBMDOS_IPE_OK;               /* 0 */
              case 0x2:
                return CBMDOS_IPE_READ_ERROR_BNF;   /* 20 */
              case 0x3:
                return CBMDOS_IPE_READ_ERROR_SYNC;  /* 21 */
              case 0x4:
                return CBMDOS_IPE_READ_ERROR_DATA;  /* 22 */
              case 0x5:
                return CBMDOS_IPE_READ_ERROR_CHK;   /* 23 */ 
              case 0x7:
                return CBMDOS_IPE_WRITE_ERROR_VER;  /* 25 */
              case 0x8:
                return CBMDOS_IPE_WRITE_PROTECT_ON; /* 26 */
              case 0x9:
                return CBMDOS_IPE_READ_ERROR_BCHK;  /* 27 */
              case 0xA:
                return CBMDOS_IPE_WRITE_ERROR_BIG;  /* 28 */
              case 0xB:
                return CBMDOS_IPE_DISK_ID_MISMATCH; /* 29 */
              case 0xF:
                return CBMDOS_IPE_NOT_READY;        /* 74 */
              case 0x10:
                return CBMDOS_IPE_READ_ERROR_GCR;   /* 24 */
              default:
                return 0;
            }
        }
        break;
      case DISK_IMAGE_TYPE_G64:
        if (fsimage_gcr_read_sector(image, buf, track, sector) < 0) {
            return -1;
        }
        break;
      case DISK_IMAGE_TYPE_P64:
        if (fsimage_p64_read_sector(image, buf, track, sector) < 0) {
            return -1;
        }
        break;
      default:
        log_error(fsimage_log,
                  "Unknown disk image type %i.  Cannot read sector.",
                  image->type);
        return -1;
    }
    return 0;
}

int fsimage_write_sector(disk_image_t *image, BYTE *buf, unsigned int track,
                         unsigned int sector)
{
    int sectors;
    long offset;
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Attempt to write without disk image.");
        return -1;
    }

    if (image->read_only != 0) {
        log_error(fsimage_log, "Attempt to write to read-only disk image.");
        return -1;
    }

    sectors = disk_image_check_sector(image, track, sector);

    switch (image->type) {
      case DISK_IMAGE_TYPE_D64:
      case DISK_IMAGE_TYPE_D67:
      case DISK_IMAGE_TYPE_D71:
      case DISK_IMAGE_TYPE_D81:
      case DISK_IMAGE_TYPE_D80:
      case DISK_IMAGE_TYPE_D82:
      case DISK_IMAGE_TYPE_X64:
      case DISK_IMAGE_TYPE_D1M:
      case DISK_IMAGE_TYPE_D2M:
      case DISK_IMAGE_TYPE_D4M:
        if (sectors < 0) {
            log_error(fsimage_log, "Track: %i, Sector: %i out of bounds.",
                      track, sector);
            return -1;
        }
        offset = sectors << 8;

        if (image->type == DISK_IMAGE_TYPE_X64)
            offset += X64_HEADER_LENGTH;

        fseek(fsimage->fd, offset, SEEK_SET);

        if (fwrite((char *)buf, 256, 1, fsimage->fd) < 1) {
            log_error(fsimage_log, "Error writing T:%i S:%i to disk image.",
                      track, sector);
            return -1;
        }

        /* Make sure the stream is visible to other readers.  */
        fflush(fsimage->fd);
        break;
      case DISK_IMAGE_TYPE_G64:
        if (fsimage_gcr_write_sector(image, buf, track, sector) < 0) {
            return -1;
        }
        break;
      case DISK_IMAGE_TYPE_P64:
        if (fsimage_p64_write_sector(image, buf, track, sector) < 0) {
            return -1;
        }
        break;
      default:
        log_error(fsimage_log, "Unknown disk image.  Cannot write sector.");
        return -1;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/

void fsimage_init(void)
{
    fsimage_log = log_open("Filesystem Image");
    fsimage_gcr_init();
    fsimage_p64_init();
    fsimage_probe_init();
}

