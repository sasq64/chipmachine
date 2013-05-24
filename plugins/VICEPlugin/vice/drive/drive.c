/*
 * drive.c - Hardware-level disk drive emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  André Fachat <fachat@physik.tu-chemnitz.de>
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

/* TODO:
        - more accurate emulation of disk rotation.
        - different speeds within one track.
        - check for byte ready *within* `BVC', `BVS' and `PHP'.
        - serial bus handling might be faster.  */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "attach.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "drive-check.h"
#include "drive-overflow.h"
#include "drive.h"
#include "drivecpu.h"
#include "drivecpu65c02.h"
#include "driveimage.h"
#include "drivesync.h"
#include "driverom.h"
#include "drivetypes.h"
#include "gcr.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "lib.h"
#include "log.h"
#include "machine-drive.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "rotation.h"
#include "types.h"
#include "uiapi.h"
#include "ds1216e.h"
#include "drive-sound.h"
#include "p64.h"

static int drive_init_was_called = 0;

drive_context_t *drive_context[DRIVE_NUM];

/* Generic drive logging goes here.  */
static log_t drive_log = LOG_ERR;

/* If nonzero, at least one vaild drive ROM has already been loaded.  */
int rom_loaded = 0;

/* ------------------------------------------------------------------------- */

static int drive_led_color[DRIVE_NUM];

static void drive_extend_disk_image(drive_t *drive);

/* ------------------------------------------------------------------------- */

void drive_set_disk_memory(BYTE *id, unsigned int track, unsigned int sector,
                           struct drive_context_s *drv)
{
    drive_t *drive;

    drive = drv->drive;

    if (drive->type == DRIVE_TYPE_1541
        || drive->type == DRIVE_TYPE_1541II
        || drive->type == DRIVE_TYPE_1570
        || drive->type == DRIVE_TYPE_1571
        || drive->type == DRIVE_TYPE_1571CR) {
        drv->cpud->drive_ram[0x12] = id[0];
        drv->cpud->drive_ram[0x13] = id[1];
        drv->cpud->drive_ram[0x16] = id[0];
        drv->cpud->drive_ram[0x17] = id[1];
        drv->cpud->drive_ram[0x18] = track;
        drv->cpud->drive_ram[0x19] = sector;
        drv->cpud->drive_ram[0x22] = track;
    }
}

void drive_set_last_read(unsigned int track, unsigned int sector, BYTE *buffer,
                         struct drive_context_s *drv)
{
    drive_t *drive;

    drive = drv->drive;

    drive_gcr_data_writeback(drive);
    drive_set_half_track(track * 2, drive);

    if (drive->type == DRIVE_TYPE_1541
        || drive->type == DRIVE_TYPE_1541II
        || drive->type == DRIVE_TYPE_1570
        || drive->type == DRIVE_TYPE_1571
        || drive->type == DRIVE_TYPE_1571CR) {
        memcpy(&(drv->cpud->drive_ram[0x0400]), buffer, 256);
    }
}

/* ------------------------------------------------------------------------- */

/* Global clock counters.  */
CLOCK drive_clk[DRIVE_NUM];

/* Initialize the hardware-level drive emulation (should be called at least
   once before anything else).  Return 0 on success, -1 on error.  */
int drive_init(void)
{
    unsigned int dnr;
    drive_t *drive;

    if (rom_loaded)
        return 0;

    drive_init_was_called = 1;

    driverom_init();
    drive_image_init();

    drive_log = log_open("Drive");

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        char *logname;

        drive = drive_context[dnr]->drive;
        logname = lib_msprintf("Drive %i", dnr + 8);
        drive->log = log_open(logname);
        lib_free(logname);

        drive_clk[dnr] = 0L;
        drive->clk = &drive_clk[dnr];
        drive->mynumber = dnr;
    }

    if (driverom_load_images() < 0) {
        resources_set_int("Drive8Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive10Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive11Type", DRIVE_TYPE_NONE);
        return -1;
    }

    log_message(drive_log, "Finished loading ROM images.");
    rom_loaded = 1;

    drive_overflow_init();

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        drive->drive_ram_expand2 = NULL;
        drive->drive_ram_expand4 = NULL;
        drive->drive_ram_expand6 = NULL;
        drive->drive_ram_expand8 = NULL;
        drive->drive_ram_expanda = NULL;

        machine_drive_port_default(drive_context[dnr]);

        if (drive_check_type(drive->type, dnr) < 1)
            resources_set_int_sprintf("Drive%iType", DRIVE_TYPE_NONE, dnr + 8);

        machine_drive_rom_setup_image(dnr);

        drive->rtc_offset = (time_t)0; /* TODO: offset */
        drive->ds1216 = ds1216e_init(&drive->rtc_offset);
        drive->ds1216->hours12 = 1;
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        drive->gcr = gcr_create_image();
        drive->p64 = lib_calloc(1, sizeof(TP64Image));
        P64ImageCreate(drive->p64);
        drive->byte_ready_level = 1;
        drive->byte_ready_edge = 1;
        drive->GCR_dirty_track = 0;
        drive->GCR_write_value = 0x55;
        drive->GCR_track_start_ptr = drive->gcr->data;
        drive->GCR_current_track_size = 0;
        drive->attach_clk = (CLOCK)0;
        drive->detach_clk = (CLOCK)0;
        drive->attach_detach_clk = (CLOCK)0;
        drive->old_led_status = 0;
        drive->old_half_track = 0;
        drive->side = 0;
        drive->GCR_image_loaded = 0;
        drive->P64_image_loaded = 0;
        drive->P64_dirty = 0;
        drive->read_only = 0;
        drive->clock_frequency = 1;
        drive->led_last_change_clk = *(drive->clk);
        drive->led_last_uiupdate_clk = *(drive->clk);
        drive->led_active_ticks = 0;

        rotation_reset(drive);
        drive_image_init_track_size_d64(drive);

        /* Position the R/W head on the directory track.  */
        drive_set_half_track(36, drive);
        drive_led_color[dnr] = DRIVE_ACTIVE_RED;
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        driverom_initialize_traps(drive, 1);

        drivesync_clock_frequency(drive->type, drive);

        rotation_init((drive->clock_frequency == 2) ? 1 : 0, dnr);

        if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
            drivecpu65c02_init(drive_context[dnr], drive->type);
        } else {
            drivecpu_init(drive_context[dnr], drive->type);
        }

        /* Make sure the sync factor is acknowledged correctly.  */
        drivesync_factor(drive_context[dnr]);

        /* Make sure the traps are moved as needed.  */
        if (drive->enable)
            drive_enable(drive_context[dnr]);
    }

    return 0;
}

void drive_shutdown(void)
{
    unsigned int dnr;

    if (!drive_init_was_called) {
        /* happens at the -help command line command*/
        return;
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        if (drive_context[dnr]->drive->type == DRIVE_TYPE_2000 || drive_context[dnr]->drive->type == DRIVE_TYPE_4000) {
            drivecpu65c02_shutdown(drive_context[dnr]);
        } else {
            drivecpu_shutdown(drive_context[dnr]);
        }
        if (drive_context[dnr]->drive->gcr) {
            gcr_destroy_image(drive_context[dnr]->drive->gcr);
        }
        if (drive_context[dnr]->drive->p64) {
            P64ImageDestroy(drive_context[dnr]->drive->p64);
            lib_free(drive_context[dnr]->drive->p64);
        }
        ds1216e_destroy(drive_context[dnr]->drive->ds1216);
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        lib_free(drive_context[dnr]->drive);
        lib_free(drive_context[dnr]);
    }
}

void drive_set_active_led_color(unsigned int type, unsigned int dnr)
{
    switch (type) {
      case DRIVE_TYPE_1541:
      case DRIVE_TYPE_1551:
      case DRIVE_TYPE_1570:
      case DRIVE_TYPE_1571:
      case DRIVE_TYPE_1571CR:
        drive_led_color[dnr] = DRIVE_ACTIVE_RED;
        break;
      case DRIVE_TYPE_1541II:
      case DRIVE_TYPE_1581:
      case DRIVE_TYPE_2000:
      case DRIVE_TYPE_4000:
        drive_led_color[dnr] = DRIVE_ACTIVE_GREEN;
        break;
      case DRIVE_TYPE_2031:
      case DRIVE_TYPE_2040:
      case DRIVE_TYPE_3040:
      case DRIVE_TYPE_4040:
      case DRIVE_TYPE_1001:
      case DRIVE_TYPE_8050:
      case DRIVE_TYPE_8250:
        drive_led_color[dnr] = DRIVE_ACTIVE_RED;
        break;
      default:
        drive_led_color[dnr] = DRIVE_ACTIVE_RED;
    }
}

int drive_set_disk_drive_type(unsigned int type, struct drive_context_s *drv)
{
    unsigned int dnr;
    drive_t *drive;
    drive_t *drive1;

    dnr = drv->mynumber;

    if (machine_drive_rom_check_loaded(type) < 0)
        return -1;

    drive = drv->drive;
    rotation_rotate_disk(drive);

    drivesync_clock_frequency(type, drive);

    rotation_init(0, dnr);
    drive->type = type;
    if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
        drivecpu65c02_setup_context(drv, 0);
    } else {
        drivecpu_setup_context(drv, 0);
    }
    drive->side = 0;
    machine_drive_rom_setup_image(dnr);
    drivesync_factor(drv);
    drive_set_active_led_color(type, dnr);

    /* set up (relatively) easy detection of dual drives */
    drive1 = drive_context[mk_drive1(dnr)]->drive;
    drive->drive0 = NULL;
    drive1->drive1 = NULL;
    if (is_drive0(dnr) && drive_check_dual(type)) {
        drive->drive1 = drive1;
        drive1->drive0 = drive;
    } else {
        drive->drive1 = NULL;
        drive1->drive0 = NULL;
    }

    if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
        drivecpu65c02_init(drv, type);
    } else {
        drivecpu_init(drv, type);
    }

    return 0;
}

void drive_enable_update_ui(drive_context_t *drv)
{
    int i;
    unsigned int enabled_drives = 0;

    for (i = 0; i < DRIVE_NUM; i++) {
        unsigned int the_drive;
        drive_t *drive = drive_context[i]->drive;

        the_drive = 1 << i;

        if (drive->enable || (drive->drive0 && drive->drive0->enable)) {
            enabled_drives |= the_drive;
            drive->old_led_status = -1;
            drive->old_half_track = -1;
        }
    }

    ui_enable_drive_status(enabled_drives,
                           drive_led_color);
}

/* Activate full drive emulation. */
int drive_enable(drive_context_t *drv)
{
    int drive_true_emulation = 0;
    unsigned int dnr;
    drive_t *drive;

    dnr = drv->mynumber;
    drive = drv->drive;

    /* This must come first, because this might be called before the drive
       initialization.  */
    if (!rom_loaded)
        return -1;

    resources_get_int("DriveTrueEmulation", &drive_true_emulation);

    /* Always disable kernal traps. */
    if (!drive_true_emulation)
        return 0;

    if (drive->type == DRIVE_TYPE_NONE)
        return 0;

    /* Recalculate drive geometry.  */
    if (drive->image != NULL)
        drive_image_attach(drive->image, dnr + 8);

    if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
        drivecpu65c02_wake_up(drv);
    } else {
        drivecpu_wake_up(drv);
    }

    /* Make sure the UI is updated.  */
    drive_enable_update_ui(drv);
    return 0;
}

/* Disable full drive emulation.  */
void drive_disable(drive_context_t *drv)
{
    int drive_true_emulation = 0;
    drive_t *drive;

    drive = drv->drive;

    /* This must come first, because this might be called before the true
       drive initialization.  */
    drive->enable = 0;

    resources_get_int("DriveTrueEmulation", &drive_true_emulation);

    if (rom_loaded) {
        if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
            drivecpu65c02_sleep(drv);
        } else {
            drivecpu_sleep(drv);
        }
        machine_drive_port_default(drv);

        drive_gcr_data_writeback(drive);
    }

    /* Make sure the UI is updated.  */
    drive_enable_update_ui(drv);
}

void drive_reset(void)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
            drivecpu65c02_reset(drive_context[dnr]);
        } else {
            drivecpu_reset(drive_context[dnr]);
        }

        drive->led_last_change_clk = *(drive->clk);
        drive->led_last_uiupdate_clk = *(drive->clk);
        drive->led_active_ticks = 0;
    }
}

void drive_current_track_size_set(drive_t *dptr)
{
    dptr->GCR_current_track_size =
        dptr->gcr->track_size[dptr->current_half_track - 2];
}

/* Move the head to half track `num'.  */
void drive_set_half_track(int num, drive_t *dptr)
{
    if ((dptr->type == DRIVE_TYPE_1541 || dptr->type == DRIVE_TYPE_1541II
        || dptr->type == DRIVE_TYPE_1551 || dptr->type == DRIVE_TYPE_1570
        || dptr->type == DRIVE_TYPE_2031) && num > 84)
        num = 84;
    if ((dptr->type == DRIVE_TYPE_1571 || dptr->type == DRIVE_TYPE_1571CR)
        && num > 140)
        num = 140;
    if (num < 2)
        num = 2;

    if (dptr->current_half_track != num) {
        dptr->current_half_track = num;
        if (dptr->p64) {
            dptr->p64->PulseStreams[dptr->current_half_track].CurrentIndex = -1;
        }
    }

    dptr->GCR_track_start_ptr = (dptr->gcr->data
                                + ((dptr->current_half_track - 2)
                                * dptr->gcr->max_track_size));

    if (dptr->GCR_current_track_size != 0)
        dptr->GCR_head_offset = (dptr->GCR_head_offset
            * dptr->gcr->track_size[dptr->current_half_track - 2])
            / dptr->GCR_current_track_size;
    else
        dptr->GCR_head_offset = 0;

    drive_current_track_size_set(dptr);
}

/*-------------------------------------------------------------------------- */

/* Increment the head position by `step' half-tracks. Valid values
   for `step' are `+1', '+2' and `-1'.  */
void drive_move_head(int step, drive_t *drive)
{
    drive_gcr_data_writeback(drive);
    if (drive->type == DRIVE_TYPE_1571
        || drive->type == DRIVE_TYPE_1571CR) {
        if (drive->current_half_track + step >= 71)
            return;
    }
    drive_sound_head(drive->current_half_track, step, drive->mynumber);
    drive_set_half_track(drive->current_half_track + step, drive);
}

/* Hack... otherwise you get internal compiler errors when optimizing on
    gcc2.7.2 on RISC OS */
static void gcr_data_writeback2(BYTE *buffer, BYTE *offset, unsigned int track,
                                unsigned int sector, drive_t *drive)
{
    int rc;

    gcr_convert_GCR_to_sector(buffer, offset,
                              drive->GCR_track_start_ptr,
                              drive->GCR_current_track_size);
    if (buffer[0] != 0x7) {
        log_error(drive->log,
                  "Could not find data block id of T:%d S:%d.",
                  track, sector);
    } else {
        rc = disk_image_write_sector(drive->image, buffer + 1, track, sector);
        if (rc < 0)
            log_error(drive->log,
                      "Could not update T:%d S:%d.", track, sector);
    }
}

void drive_gcr_data_writeback(drive_t *drive)
{
    int extend;
    unsigned int half_track, track, sector, max_sector = 0;
    BYTE buffer[260], *offset;

    if (drive->image == NULL) {
        return;
    }

    half_track = drive->current_half_track;
    track = drive->current_half_track / 2;

    if (drive->image->type == DISK_IMAGE_TYPE_P64) {
        return;
    }

    if (!(drive->GCR_dirty_track)) {
        return;
    }

    if (drive->image->type == DISK_IMAGE_TYPE_G64) {
        BYTE *gcr_track_start_ptr;
        unsigned int gcr_current_track_size;

        gcr_current_track_size = drive->gcr->track_size[half_track - 2];

        gcr_track_start_ptr = drive->gcr->data
                              + ((half_track - 2) * drive->gcr->max_track_size);

        disk_image_write_half_track(drive->image, half_track,
                                   gcr_current_track_size,
                                   drive->gcr->speed_zone,
                                   gcr_track_start_ptr);
        drive->GCR_dirty_track = 0;
        return;
    }

    if (drive->image->type == DISK_IMAGE_TYPE_D64
        || drive->image->type == DISK_IMAGE_TYPE_X64) {
        if (track > EXT_TRACKS_1541)
            return;
        max_sector = disk_image_sector_per_track(DISK_IMAGE_TYPE_D64, track);
        if (track > drive->image->tracks) {
            switch (drive->extend_image_policy) {
              case DRIVE_EXTEND_NEVER:
                drive->ask_extend_disk_image = 1;
                return;
              case DRIVE_EXTEND_ASK:
                if (drive->ask_extend_disk_image == 1) {
                    extend = ui_extend_image_dialog();
                    if (extend == 0) {
                        drive->ask_extend_disk_image = 0;
                        return;
                    } else {
                        drive_extend_disk_image(drive);
                    }
                } else {
                    return;
                }
                break;
              case DRIVE_EXTEND_ACCESS:
                drive->ask_extend_disk_image = 1;
                drive_extend_disk_image(drive);
                break;
            }
        }
    }

    if (drive->image->type == DISK_IMAGE_TYPE_D71) {
        if (track > MAX_TRACKS_1571)
            return;
        max_sector = disk_image_sector_per_track(DISK_IMAGE_TYPE_D71, track);
    }

    drive->GCR_dirty_track = 0;

    for (sector = 0; sector < max_sector; sector++) {

        offset = gcr_find_sector_header(track, sector,
                                        drive->GCR_track_start_ptr,
                                        drive->GCR_current_track_size);
        if (offset == NULL) {
            log_error(drive->log,
                      "Could not find header of T:%d S:%d.",
                      track, sector);
        } else {
            offset = gcr_find_sector_data(offset,
                                          drive->GCR_track_start_ptr,
                                          drive->GCR_current_track_size);
            if (offset == NULL) {
                log_error(drive->log,
                          "Could not find data sync of T:%d S:%d.",
                          track, sector);
            } else {
                gcr_data_writeback2(buffer, offset, track, sector, drive);
            }
        }
    }
}

void drive_gcr_data_writeback_all(void)
{
    drive_t *drive;
    unsigned int i;

    for (i = 0; i < DRIVE_NUM; i++) {
        drive = drive_context[i]->drive;
        drive_gcr_data_writeback(drive);
        if (drive->P64_image_loaded && drive->image && drive->image->p64) {
            if (drive->image->type == DISK_IMAGE_TYPE_P64) {
                if (drive->P64_dirty) {
                drive->P64_dirty = 0;
                    disk_image_write_p64_image(drive->image);
                }
            }
        }
    }


}

static void drive_extend_disk_image(drive_t *drive)
{
    int rc;
    unsigned int track, sector;
    BYTE buffer[256];

    drive->image->tracks = EXT_TRACKS_1541;
    memset(buffer, 0, 256);
    for (track = NUM_TRACKS_1541 + 1; track <= EXT_TRACKS_1541; track++) {
        for (sector = 0;
             sector < disk_image_sector_per_track(DISK_IMAGE_TYPE_D64, track);
             sector++) {
             rc = disk_image_write_sector(drive->image, buffer, track,
                                          sector);
             if (rc < 0)
                 log_error(drive->log,
                           "Could not update T:%d S:%d.", track, sector);
        }
    }
}

/* ------------------------------------------------------------------------- */

static void drive_led_update(drive_t *drive, drive_t *drive0)
{
    int my_led_status = 0;
    CLOCK led_period;
    unsigned int led_pwm;

    /* Actually update the LED status only if the `trap idle'
       idling method is being used, as the LED status could be
       incorrect otherwise.  */

    if (drive0->idling_method != DRIVE_IDLE_SKIP_CYCLES)
        my_led_status = drive->led_status;

    /* Update remaining led clock ticks. */
    if (drive->led_status & 1)
        drive->led_active_ticks += *(drive0->clk)
                                   - drive->led_last_change_clk;
    drive->led_last_change_clk = *(drive0->clk);

    led_period = *(drive0->clk) - drive->led_last_uiupdate_clk;
    drive->led_last_uiupdate_clk = *(drive0->clk);

    if (led_period == 0) {
        return;
    }

    if (drive->led_active_ticks > led_period) {
    /* during startup it has been observer that led_pwm > 1000,
       which potentially breaks several UIs */
    /* this also happens when the drive is reset from UI
       and the LED was on */
        led_pwm = 1000;
    } else {
        led_pwm = drive->led_active_ticks * 1000 / led_period;
    }
    assert(led_pwm <= MAX_PWM);
    if (led_pwm > MAX_PWM) {
        led_pwm = MAX_PWM;
    }

    drive->led_active_ticks = 0;

    if (led_pwm != drive->led_last_pwm
        || my_led_status != drive->old_led_status) {
        ui_display_drive_led(drive->mynumber, led_pwm,
                             (my_led_status & 2) ? 1000 : 0);
        drive->led_last_pwm = led_pwm;
        drive->old_led_status = my_led_status;
    }
}

/* Update the status bar in the UI.  */
void drive_update_ui_status(void)
{
    int i;

    if (console_mode || (machine_class == VICE_MACHINE_VSID)) {
        return;
    }

    /* Update the LEDs and the track indicators.  */
    for (i = 0; i < DRIVE_NUM; i++) {
        drive_t *drive = drive_context[i]->drive;
        drive_t *drive0 = drive->drive0;
        int dual = drive0 && drive0->enable;

        if (drive->enable || dual) {
            if (!drive0) {
                drive0 = drive;
            }

            drive_led_update(drive, drive0);

            if (drive->current_half_track != drive->old_half_track) {
                drive->old_half_track = drive->current_half_track;
                dual = dual || drive->drive1;   /* also include drive 0 */
                ui_display_drive_track(i,
                        dual ? 0 : 8,
                        drive->current_half_track);
            }
        }
    }
}

int drive_num_leds(unsigned int dnr)
{
    drive_t *drive = drive_context[dnr]->drive;

    if (drive_check_old(drive->type)) {
        return 2;
    }

    if (drive->drive0) {
        return 2;
    }

    if (drive->type == DRIVE_TYPE_2000
        || drive->type == DRIVE_TYPE_4000) {
        return 2;
    }

    return 1;
}

/* This is called at every vsync. */
void drive_vsync_hook(void)
{
    unsigned int dnr;

    drive_update_ui_status();

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive_t *drive = drive_context[dnr]->drive;
        if (drive->enable) {
            if (drive->idling_method != DRIVE_IDLE_SKIP_CYCLES) {
                if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
                    drivecpu65c02_execute(drive_context[dnr], maincpu_clk);
                } else {
                    drivecpu_execute(drive_context[dnr], maincpu_clk);
                }
            }
            if (drive->idling_method == DRIVE_IDLE_NO_IDLE) {
                /* if drive is never idle, also rotate the disk. this prevents
                 * huge peaks in cpu usage when the drive must catch up with
                 * a longer period of time.
                 */
                rotation_rotate_disk(drive);
            }
            /* printf("drive_vsync_hook drv %d @clk:%d\n", dnr, maincpu_clk); */
        }
    }
}

/* ------------------------------------------------------------------------- */

static void drive_setup_context_for_drive(drive_context_t *drv,
                                          unsigned int dnr)
{
    drv->mynumber = dnr;
    drv->drive = lib_calloc(1, sizeof(drive_t));
    drv->clk_ptr = &drive_clk[dnr];

    if (drv->drive->type == DRIVE_TYPE_2000 || drv->drive->type == DRIVE_TYPE_4000) {
        drivecpu65c02_setup_context(drv, 1);
    } else {
        drivecpu_setup_context(drv, 1);
    }
    machine_drive_setup_context(drv);
}

void drive_setup_context(void)
{
    unsigned int dnr;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive_context[dnr] = lib_calloc(1, sizeof(drive_context_t));
        drive_setup_context_for_drive(drive_context[dnr], dnr);
    }
}
