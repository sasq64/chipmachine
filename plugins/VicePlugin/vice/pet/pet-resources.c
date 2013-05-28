/*
 * pet-resources.c
 *
 * Written by
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

#include <stdio.h>
#include <string.h>

#include "crtc.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "pet-resources.h"
#include "pet.h"
#include "petmem.h"
#include "petrom.h"
#include "pets.h"
#include "resources.h"
#include "util.h"

/*
    the following keyboard modells exist for the PET:

    - chicklet keyboard (original PET 2001)
    - graphics keyboard with the 20 key numpad (PET 2001-N)

      "Graphics" (BGRS/BGRP)

    - business keyboard (PET 2001-B)

      For some countries (e.g. Germany) there where some kits for the business
      keyboard including an changed Exxx-EPROM (editor) with other keyboard
      mappings and some changeable keyboard caps with the right characters on it.

      "Business (us)"
      "Business (uk)" (BUKS/BUKP)
      "Business (de)" (BDES/BDEP)
      "Business (jp)"
*/

#define KBD_INDEX_PET_BUKS  0
#define KBD_INDEX_PET_BUKP  1
#define KBD_INDEX_PET_BGRS  2
#define KBD_INDEX_PET_BGRP  3
#define KBD_INDEX_PET_BDES  4
#define KBD_INDEX_PET_BDEP  5

static int sync_factor;

static int set_iosize(int val, void *param)
{
    petres.IOSize = val;

    mem_initialize_memory();

    return 0;
}

static int set_crtc_enabled(int val, void *param)
{
    petres.crtc = val;

    return 0;
}

static int set_superpet_enabled(int val, void *param)
{
    if (val < 2)
        petres.superpet = (unsigned int)val;

    mem_initialize_memory();

    return 0;
}

static int set_ram_9_enabled(int val, void *param)
{
    if (val < 2)
        petres.mem9 = (unsigned int)val;

    mem_initialize_memory();

    return 0;
}

static int set_ram_a_enabled(int val, void *param)
{
    if (val < 2)
        petres.memA = (unsigned int)val;

    mem_initialize_memory();

    return 0;
}

static int set_ramsize(int size, void *param)
{
    int i;
    const int sizes[] = { 4, 8, 16, 32, 96, 128 };

    for (i = 0; i < util_arraysize(sizes); i++) {
        if (size <= sizes[i])
            break;
    }

    if (i > util_arraysize(sizes) - 1)
        i = util_arraysize(sizes) - 1;

    size = sizes[i];

    petres.ramSize = size;
    petres.map = 0;

    if (size == 96) {
        petres.map = PET_MAP_8096;         /* 8096 mapping */
    } else if (size == 128) {
        petres.map = PET_MAP_8296;         /* 8296 mapping */
    }

    petmem_check_info(&petres);
    mem_initialize_memory();

    return 0;
}

static int set_video(int col, void *param)
{
    if (col != petres.video) {
        if (col == 0 || col == 40 || col == 80) {

            petres.video = col;

            petmem_check_info(&petres);

            pet_crtc_set_screen();
        }
    }

    return 0;
}

/* ROM filenames */

static int set_chargen_rom_name(const char *val, void *param)
{
    if (util_string_set(&petres.chargenName, val))
        return 0;

    return petrom_load_chargen();
}

static int set_kernal_rom_name(const char *val, void *param)
{
    if (util_string_set(&petres.kernalName, val))
        return 0;

    return petrom_load_kernal();
}

static int set_basic_rom_name(const char *val, void *param)
{
/*  do we want to reload the basic even with the same name - romB can
    overload the basic ROM image and we can restore it only here ?
*/
    if (util_string_set(&petres.basicName, val))
        return 0;

    return petrom_load_basic();
}

static int set_editor_rom_name(const char *val, void *param)
{
    if (util_string_set(&petres.editorName, val))
        return 0;

    return petrom_load_editor();
}

static int set_rom_module_9_name(const char *val, void *param)
{
    if (util_string_set(&petres.mem9name, val))
        return 0;

    return petrom_load_rom9();
}

static int set_rom_module_a_name(const char *val, void *param)
{
    if (util_string_set(&petres.memAname, val))
        return 0;

    return petrom_load_romA();
}

static int set_rom_module_b_name(const char *val, void *param)
{
    if (util_string_set(&petres.memBname, val))
        return 0;

    return petrom_load_romB();
}

/* Enable/disable patching the PET 2001 chargen ROM/kernal ROM */

static int set_pet2k_enabled(int val, void *param)
{
    int i = ((val) ? 1 : 0);

    if (i != petres.pet2k) {
        if (petres.pet2k)
            petrom_unpatch_2001();

        petres.pet2k = i;

        if (petres.pet2k)
            petrom_patch_2001();
    }
    return 0;
}

static int set_pet2kchar_enabled(int val, void *param)
{
    int i = ((val) ? 1 : 0);

    if (i != petres.pet2kchar) {
        petres.pet2kchar = i;

        /* function reverses itself -> no reload necessary */
        petrom_convert_chargen_2k();
    }
    return 0;
}

static int set_eoiblank_enabled(int val, void *param)
{
    int i = ((val) ? 1 : 0);

    petres.eoiblank = i;

    crtc_enable_hw_screen_blank(petres.eoiblank);

    return 0;
}

static int set_sync_factor(int val, void *param)
{
    int change_timing = 0;

    if (sync_factor != val)
        change_timing = 1;

    switch (val) {
      case MACHINE_SYNC_PAL:
        sync_factor = val;
        if (change_timing)
            machine_change_timing(MACHINE_SYNC_PAL);
        break;
      case MACHINE_SYNC_NTSC:
        sync_factor = val;
        if (change_timing)
            machine_change_timing(MACHINE_SYNC_NTSC);
        break;
      default:
        return -1;
    }
    return 0;
}

static int set_h6809_rom_name(const char *val, void *param)
{
    unsigned int num = vice_ptr_to_uint(param);

    if (util_string_set(&petres.h6809romName[num], val))
        return 0;

    return petrom_load_6809rom(num);
}

static int set_superpet_cpu_switch(int val, void *param)
{
    int i;
 
    switch (val) {
        case 6502:
        case SUPERPET_CPU_6502:
            i = SUPERPET_CPU_6502;
            break;
        case 6809:
        case SUPERPET_CPU_6809:
            i = SUPERPET_CPU_6809;
            break;
        case SUPERPET_CPU_PROG:
            i = SUPERPET_CPU_PROG;
            break;
        default:
            return -1;
    }

    petres.superpet_cpu_switch = i;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "ChargenName", "chargen", RES_EVENT_NO, NULL,
      &petres.chargenName, set_chargen_rom_name, NULL },
    { "KernalName", PET_KERNAL4NAME, RES_EVENT_NO, NULL,
      &petres.kernalName, set_kernal_rom_name, NULL },
    { "EditorName", PET_EDITOR4B80NAME, RES_EVENT_NO, NULL,
      &petres.editorName, set_editor_rom_name, NULL },
    { "BasicName", PET_BASIC4NAME, RES_EVENT_NO, NULL,
      &petres.basicName, set_basic_rom_name, NULL },
    { "RomModule9Name", "", RES_EVENT_NO, NULL,
      &petres.mem9name, set_rom_module_9_name, NULL },
    { "RomModuleAName", "", RES_EVENT_NO, NULL,
      &petres.memAname, set_rom_module_a_name, NULL },
    { "RomModuleBName", "", RES_EVENT_NO, NULL,
      &petres.memBname, set_rom_module_b_name, NULL },
#ifdef COMMON_KBD
    { "KeymapBusinessUKSymFile", KBD_PET_SYM_UK, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[0], keyboard_set_keymap_file, (void *)0 },
    { "KeymapBusinessUKPosFile", KBD_PET_POS_UK, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[1], keyboard_set_keymap_file, (void *)1 },
    { "KeymapGraphicsSymFile", KBD_PET_SYM_GR, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[2], keyboard_set_keymap_file, (void *)2 },
    { "KeymapGraphicsPosFile", KBD_PET_POS_GR, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[3], keyboard_set_keymap_file, (void *)3 },
    { "KeymapBusinessDESymFile", KBD_PET_SYM_DE, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[4], keyboard_set_keymap_file, (void *)4 },
    { "KeymapBusinessDEPosFile", KBD_PET_POS_DE, RES_EVENT_NO, NULL,
      &machine_keymap_file_list[5], keyboard_set_keymap_file, (void *)5 },
#endif
    { "H6809RomAName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[0], set_h6809_rom_name, (void *)0 },
    { "H6809RomBName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[1], set_h6809_rom_name, (void *)1 },
    { "H6809RomCName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[2], set_h6809_rom_name, (void *)2 },
    { "H6809RomDName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[3], set_h6809_rom_name, (void *)3 },
    { "H6809RomEName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[4], set_h6809_rom_name, (void *)4 },
    { "H6809RomFName", "", RES_EVENT_NO, NULL,
      &petres.h6809romName[5], set_h6809_rom_name, (void *)5 },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
    { "RamSize", 32, RES_EVENT_SAME, NULL,
      &petres.ramSize, set_ramsize, NULL },
    { "IOSize", 0x800, RES_EVENT_SAME, NULL,
      &petres.IOSize, set_iosize, NULL },
    { "Crtc", 1, RES_EVENT_SAME, NULL,
      &petres.crtc, set_crtc_enabled, NULL },
    { "VideoSize", 1, RES_EVENT_SAME, NULL,
      &petres.video, set_video, NULL },
    { "Ram9", 0, RES_EVENT_SAME, NULL,
      &petres.mem9, set_ram_9_enabled, NULL },
    { "RamA", 0, RES_EVENT_SAME, NULL,
      &petres.memA, set_ram_a_enabled, NULL },
    { "SuperPET", 0, RES_EVENT_SAME, NULL,
      &petres.superpet, set_superpet_enabled, NULL },
    { "Basic1", 1, RES_EVENT_SAME, NULL,
      &petres.pet2k, set_pet2k_enabled, NULL },
    { "Basic1Chars", 0, RES_EVENT_SAME, NULL,
      &petres.pet2kchar, set_pet2kchar_enabled, NULL },
    { "EoiBlank", 0, RES_EVENT_SAME, NULL,
      &petres.eoiblank, set_eoiblank_enabled, NULL },
#ifdef COMMON_KBD
    { "KeymapIndex", KBD_INDEX_PET_BUKS, RES_EVENT_NO, NULL,
      &machine_keymap_index, keyboard_set_keymap_index, NULL },
#endif
    { "CPUswitch", 0, RES_EVENT_SAME, NULL,
      &petres.superpet_cpu_switch, set_superpet_cpu_switch, NULL },
//  { "SuperPETRamWriteProtect", 0, RES_EVENT_SAME, NULL,
//    &petres.ramwp, set_super_write_protect, NULL },
    { NULL }
};

int pet_resources_init(void)
{
    int i;

    petres.chargenName = NULL;
    petres.kernalName = NULL;
    petres.editorName = NULL;
    petres.basicName = NULL;
    petres.memBname = NULL;
    petres.memAname = NULL;
    petres.mem9name = NULL;

    for (i = 0; i < NUM_6809_ROMS; i++) {
        petres.h6809romName[i] = NULL;
    }
    petres.superpet_cpu_switch = SUPERPET_CPU_6502;

    if (resources_register_string(resources_string) < 0)
        return -1;

    return resources_register_int(resources_int);
}

void pet_resources_shutdown(void)
{
    int i;

    lib_free(machine_keymap_file_list[0]);
    lib_free(machine_keymap_file_list[1]);
    lib_free(machine_keymap_file_list[2]);
    lib_free(machine_keymap_file_list[3]);
    lib_free(machine_keymap_file_list[4]);
    lib_free(machine_keymap_file_list[5]);
    lib_free(petres.chargenName);
    lib_free(petres.kernalName);
    lib_free(petres.editorName);
    lib_free(petres.basicName);
    lib_free(petres.memBname);
    lib_free(petres.memAname);
    lib_free(petres.mem9name);

    for (i = 0; i < NUM_6809_ROMS; i++) {
        lib_free(petres.h6809romName[i]);
    }
}

