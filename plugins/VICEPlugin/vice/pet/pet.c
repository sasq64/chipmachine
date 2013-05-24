/*
 * pet.c
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  André Fachat <fachat@physik.tu-chemnitz.de>
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

#include <math.h>   /* modf */
#include <stdio.h>
#include <stdlib.h>

#include "attach.h"
#include "autostart.h"
#include "clkguard.h"
#include "cmdline.h"
#include "crtc-mem.h"
#include "crtc.h"
#include "datasette.h"
#include "debug.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-snapshot.h"
#include "drive-sound.h"
#include "drive.h"
#include "drivecpu.h"
#include "iecdrive.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "log.h"
#include "machine-drive.h"
#include "machine-printer.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "parallel.h"
#include "pet-cmdline-options.h"
#include "pet-resources.h"
#include "pet-snapshot.h"
#include "pet.h"
#include "petiec.h"
#include "petmem.h"
#include "petreu.h"
#include "petdww.h"
#include "pets.h"
#include "petsound.h"
#include "petui.h"
#include "petvia.h"
#include "petacia.h"
#include "petpia.h"
#include "printer.h"
#include "resources.h"
#include "rs232drv.h"
#include "screenshot.h"
#include "sid.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sidcart.h"
#include "sound.h"
#include "tape.h"
#include "traps.h"
#include "types.h"
#include "userport_dac.h"
#include "util.h"
#include "via.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

machine_context_t machine_context;

#define NUM_KEYBOARD_MAPPINGS 6

/* beos dummy for the generally used cart function in ui_file.cc */
#ifdef __BEOS__
int cartridge_attach_image(int type, const char *filename)
{
    return 0;
}
#endif

const char *machine_keymap_res_name_list[NUM_KEYBOARD_MAPPINGS] = {
    "KeymapBusinessUKSymFile", "KeymapBusinessUKPosFile",
    "KeymapGraphicsSymFile", "KeymapGraphicsPosFile",
    "KeymapBusinessDESymFile", "KeymapBusinessDEPosFile"
};

char *machine_keymap_file_list[NUM_KEYBOARD_MAPPINGS] = {
    NULL, NULL, NULL, NULL, NULL, NULL
};

const char machine_name[] = "PET";
int machine_class = VICE_MACHINE_PET;

static void machine_vsync_hook(void);

/*
static long     pet_cycles_per_rfsh     = PET_PAL_CYCLES_PER_RFSH;
static double   pet_rfsh_per_sec        = PET_PAL_RFSH_PER_SEC;
*/

static log_t pet_log = LOG_ERR;
static machine_timing_t machine_timing; 

/* ------------------------------------------------------------------------ */

/* PET-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0
        || vsync_resources_init() < 0
        || machine_video_resources_init() < 0
        || pet_resources_init() < 0
        || petreu_resources_init() < 0
        || pia1_init_resources() < 0
        || crtc_resources_init() < 0
        || petdww_resources_init() < 0
        || sound_resources_init() < 0
        || sidcart_resources_init() < 0
        || userport_dac_resources_init() < 0
        || drive_resources_init() < 0
        || datasette_resources_init() < 0
        || acia1_resources_init() < 0
        || rs232drv_resources_init() < 0
        || printer_resources_init() < 0
#ifndef COMMON_KBD
        || pet_kbd_resources_init() < 0
#endif
        )
        return -1;

    return 0;
}

void machine_resources_shutdown(void)
{
    petdww_resources_shutdown();
    video_resources_shutdown();
    pet_resources_shutdown();
    petreu_resources_shutdown();
    sound_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
}

/* PET-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
#if 0
    if (cmdline_register_options(cmdline_options) < 0)
        return -1;
#endif

    if (traps_cmdline_options_init() < 0
        || vsync_cmdline_options_init() < 0
        || video_init_cmdline_options() < 0
        || pet_cmdline_options_init() < 0
        || petreu_cmdline_options_init() < 0
        || crtc_cmdline_options_init() < 0
        || petdww_cmdline_options_init() < 0
        || pia1_init_cmdline_options() < 0
        || sound_cmdline_options_init() < 0
        || sidcart_cmdline_options_init() < 0
        || userport_dac_cmdline_options_init() < 0
        || drive_cmdline_options_init() < 0
        || datasette_cmdline_options_init() < 0
        || acia1_cmdline_options_init() < 0
        || rs232drv_cmdline_options_init() < 0
        || printer_cmdline_options_init() < 0
#ifndef COMMON_KBD
        || pet_kbd_cmdline_options_init() < 0
#endif
        )
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

#define SIGNAL_VERT_BLANK_OFF   pia1_signal(PIA_SIG_CB1, PIA_SIG_RISE);

#define SIGNAL_VERT_BLANK_ON    pia1_signal(PIA_SIG_CB1, PIA_SIG_FALL);

static void pet_crtc_signal(unsigned int signal) {
    if (signal) {
        SIGNAL_VERT_BLANK_ON
    } else {
        SIGNAL_VERT_BLANK_OFF
    }
}

/* ------------------------------------------------------------------------- */

void machine_handle_pending_alarms(int num_write_cycles)
{
}

static void pet_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502;
    monitor_cpu_type_t asm6809;
    monitor_interface_t *drive_interface_init[DRIVE_NUM];
    monitor_cpu_type_t *asmarray[3];

    asmarray[0]=&asm6502;
    asmarray[1]=&asm6809;
    asmarray[2]=NULL;

    asm6502_init(&asm6502);
    asm6809_init(&asm6809);

    for (dnr = 0; dnr < DRIVE_NUM; dnr++)
        drive_interface_init[dnr] = drivecpu_monitor_interface_get(dnr);

    /* Initialize the monitor.  */
    monitor_init(maincpu_monitor_interface_get(), drive_interface_init,
                 asmarray);
}

void machine_setup_context(void)
{
    petvia_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

/* PET-specific initialization.  */
int machine_specific_init(void)
{
    pet_log = log_open("PET");

    pet_init_ok = 1;    /* used in pet_set_model() */

    /* Setup trap handling - must be before mem_load() */
    traps_init();

    if (mem_load() < 0)
        return -1;

    log_message(pet_log, "Initializing IEEE488 bus...");

    rs232drv_init();

    /* initialize print devices */
    printer_init();

    /* Initialize autostart.  FIXME: We could probably use smaller values.  */
    /* moved to mem_load() as it is kernal-dependant AF 30jun1998
    autostart_init(1 * PET_PAL_RFSH_PER_SEC * PET_PAL_CYCLES_PER_RFSH, 0);
    */

    /* Initialize the CRTC emulation.  */
    if (crtc_init() == NULL)
        return -1;

    crtc_set_retrace_type(petres.crtc);
    crtc_set_retrace_callback(pet_crtc_signal);
    pet_crtc_set_screen();

    via_init(machine_context.via);
    pia1_init();
    pia2_init();
    acia1_init();

#ifndef COMMON_KBD
    /* Initialize the keyboard.  */
    if (pet_kbd_init() < 0)
        return -1;
#endif

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level 1541 emulation.  */
    drive_init();

    pet_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* Initialize the sidcart first */
    sidcart_sound_chip_init();

    /* Initialize native sound chip */
    pet_sound_chip_init();

    /* Initialize cartridge based sound chips */
    userport_dac_sound_chip_init();

    drive_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);

    /* Initialize keyboard buffer.  FIXME: Is this correct?  */
    /* moved to mem_load() because it's model specific... AF 30jun1998
    kbdbuf_init(631, 198, 10, PET_PAL_CYCLES_PER_RFSH * PET_PAL_RFSH_PER_SEC);
    */

    /* Initialize the PET-specific part of the UI.  */
    petui_init();

    /* Initialize the PET Ram and Expansion Unit. */
    petreu_init();

    /* Initialize the PET Double-W Hi-Res graphics card. */
    petdww_init();

    petiec_init();

    machine_drive_stub();

#if defined (USE_XF86_EXTENSIONS) && \
    (defined(USE_XF86_VIDMODE_EXT) || defined (HAVE_XRANDR))
    {
	/* set fullscreen if user used `-fullscreen' on cmdline */
	int fs;
	resources_get_int("UseFullscreen", &fs);
	if (fs)
	    resources_set_int("CRTCFullscreen", 1);
    }
#endif
    return 0;
}

/* PET-specific initialization.  */
void machine_specific_reset(void)
{
    pia1_reset();
    pia2_reset();
    viacore_reset(machine_context.via);
    acia1_reset();
    crtc_reset();
    sid_reset();
    petmem_reset();
    rs232drv_reset();
    printer_reset();
    drive_reset();
    datasette_reset();
    petreu_reset();
    petdww_reset();
}

void machine_specific_powerup(void)
{
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(1);

    viacore_shutdown(machine_context.via);

    /* close the video chip(s) */
    petdww_shutdown();
    crtc_shutdown();

    petreu_shutdown();

    petui_shutdown();
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    CLOCK sub;

    autostart_advance();

    drive_vsync_hook();

    screenshot_record();

    sub = clk_guard_prevent_overflow(maincpu_clk_guard);

    /* The drive has to deal both with our overflowing and its own one, so
       it is called even when there is no overflowing in the main CPU.  */
    drivecpu_prevent_clk_overflow_all(sub);
}

/* Dummy - no restore key.  */
void machine_set_restore_key(int v)
{
}

int machine_has_restore_key(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

long machine_get_cycles_per_second(void)
{
    return machine_timing.cycles_per_sec;
}

long machine_get_cycles_per_frame(void)
{
    return machine_timing.cycles_per_rfsh;
}

void machine_get_line_cycle(unsigned int *line, unsigned int *cycle, int *half_cycle)
{
    *line = (unsigned int)((maincpu_clk) / machine_timing.cycles_per_line
            % machine_timing.screen_lines);

    *cycle = (unsigned int)((maincpu_clk) % machine_timing.cycles_per_line);

    *half_cycle = (int)-1;
}

void machine_change_timing(int timeval)
{

    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = PET_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = PET_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = PET_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = PET_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = PET_PAL_SCREEN_LINES;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = PET_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = PET_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = PET_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = PET_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = PET_NTSC_SCREEN_LINES;
            break;
        default:
            log_error(pet_log, "Unknown machine timing.");
    }

    debug_set_machine_parameter(machine_timing.cycles_per_line, machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);

    /* Should these be called also?
    //vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);
    clk_guard_set_clk_base(maincpu_clk_guard, machine_timing.cycles_per_rfsh);
    */

    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

/* Set the screen refresh rate, as this is variable in the CRTC */
void machine_set_cycles_per_frame(long cpf)
{
    double i, f;

    machine_timing.cycles_per_rfsh = cpf;
    machine_timing.rfsh_per_sec = ((double)machine_timing.cycles_per_sec)
                                  / ((double)cpf);

    f = modf(machine_timing.rfsh_per_sec, &i) * 1000;

    log_message(pet_log, "cycles per frame set to %ld, refresh to %d.%03dHz",
                cpf, (int)i, (int)f);

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* sound_set_cycles_per_rfsh(machine_timing.cycles_per_rfsh); */
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks,
                           int event_mode)
{
    return pet_snapshot_write(name, save_roms, save_disks, event_mode);
}

int machine_read_snapshot(const char *name, int event_mode)
{
    return pet_snapshot_read(name, event_mode);
}


/* ------------------------------------------------------------------------- */

int machine_autodetect_psid(const char *name)
{
    return -1;
}

void machine_play_psid(int tune)
{
}


/* ------------------------------------------------------------------------- */

void pet_crtc_set_screen(void)
{
    int cols, vmask;

    cols = petres.video;
    vmask = petres.vmask;

    /* mem_initialize_memory(); */

    if (!cols) {
        cols = petres.rom_video;
        vmask = (cols == 40) ? 0x3ff : 0x7ff;
    }
    if (!cols) {
        cols = PET_COLS;
        vmask = (cols == 40) ? 0x3ff : 0x7ff;
    }

    /* when switching 8296 to 40 columns, CRTC ends up at $9000 otherwise...*/
    if (cols == 40)
        vmask = 0x3ff;
/*
    log_message(pet_mem_log, "set_screen(vmask=%04x, cols=%d, crtc=%d)",
                vmask, cols, petres.crtc);
*/
/*
    crtc_set_screen_mode(mem_ram + 0x8000, vmask, cols, (cols==80) ? 2 : 0);
*/
    crtc_set_screen_options(cols, 25 * 10);
    crtc_set_screen_addr(mem_ram + 0x8000);
    crtc_set_hw_options((cols == 80) ? 2 : 0, vmask, 0x2000, 512, 0x1000);
    crtc_set_retrace_type(petres.crtc ? 1 : 0);

    /* No CRTC -> assume 40 columns */
    if (!petres.crtc) {
        crtc_store(0, 13);
        crtc_store(1, 0);
        crtc_store(0, 12);
        crtc_store(1, 0x10);
        crtc_store(0, 9);
        crtc_store(1, 7);
        crtc_store(0, 8);
        crtc_store(1, 0);
        crtc_store(0, 7);
        crtc_store(1, 29);
        crtc_store(0, 6);
        crtc_store(1, 25);
        crtc_store(0, 5);
        crtc_store(1, 16);
        crtc_store(0, 4);
        crtc_store(1, 32);
        crtc_store(0, 3);
        crtc_store(1, 8);
        crtc_store(0, 2);
        crtc_store(1, 50);
        crtc_store(0, 1);
        crtc_store(1, 40);
        crtc_store(0, 0);
        crtc_store(1, 63);
    }
}

int machine_screenshot(screenshot_t *screenshot, struct video_canvas_s *canvas)
{
    if (canvas != crtc_get_canvas())
        return -1;

    crtc_screenshot(screenshot);
    return 0;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
{
    if (canvas != crtc_get_canvas())
        return -1;

    crtc_async_refresh(refresh);
    return 0;
}

int machine_num_keyboard_mappings(void)
{
    return NUM_KEYBOARD_MAPPINGS;
}

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return NULL;
}

BYTE machine_tape_type_default(void)
{
    return TAPE_CAS_TYPE_PRG;
}

int machine_addr_in_ram(unsigned int addr)
{
    return addr < 0xb000;
}

const char *machine_get_name(void)
{
    return machine_name;
}

#ifdef USE_SDLUI
/* Kludges for vsid & linking issues */
const char **csidmodel = NULL;
void psid_init_driver(void) {}
#endif
