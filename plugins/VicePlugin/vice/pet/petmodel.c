/*
 * petmodel.c - PET model detection and setting.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "machine.h"
#include "petmodel.h"
#include "pets.h"
#include "resources.h"
#include "vsync.h"

#define PET_CHARGEN_NAME        "chargen"
#define SUPERPET_CHARGEN_NAME   "characters.901640-01.bin"

int pet_init_ok = 0; /* set to 1 in pet.c */

/* ------------------------------------------------------------------------- */

/*
 * table with Model information
 */
struct pet_table_s { 
    const char *model;
    petinfo_t info;
};
typedef struct pet_table_s pet_table_t;

static pet_table_t pet_table[] = {
    { "2001",
      { 8, 0x0800, 0, 40, 0, 0, 1, 1, 1, 1, 0,
        PET_CHARGEN_NAME, PET_KERNAL1NAME, PET_EDITOR1G40NAME, PET_BASIC1NAME,
        NULL, NULL, NULL } },
    { "3008",
      { 8, 0x0800, 0, 40, 0, 0, 1, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3016",
      { 16, 0x0800, 0, 40, 0, 0, 1, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3032",
      { 32, 0x0800, 0, 40, 0, 0, 1, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3032B",
      { 32, 0x0800, 0, 40, 0, 0, 0, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2B40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "4016",
      { 16, 0x0800, 1, 40, 0, 0, 1, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4G40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "4032",
      { 32, 0x0800, 1, 40, 0, 0, 1, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4G40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "4032B",
      { 32, 0x0800, 1, 40, 0, 0, 0, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8032",
      { 32, 0x0800, 1, 80, 0, 0, 0, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8096",
      { 96, 0x0800, 1, 80, 0, 0, 0, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8296",
      { 128, 0x0100, 1, 80, 0, 0, 0, 0, 0, 0, 0,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "SuperPET",
      { 32, 0x0800, 1, 80, 0, 0, 0, 0, 0, 0, 1,
        "characters.901640-01.bin", PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL,
          { "waterloo-a000.901898-01.bin",
            "waterloo-b000.901898-02.bin",
            "waterloo-c000.901898-03.bin",
            "waterloo-d000.901898-04.bin",
            "waterloo-e000.901897-01.bin",
            "waterloo-f000.901898-05.bin" }
        } },
    { NULL }
};

int petmem_set_conf_info(petinfo_t *pi)
{
    int kindex;

    resources_set_int("RamSize", pi->ramSize);
    resources_set_int("IOSize", pi->IOSize);
    resources_set_int("Crtc", pi->crtc);
    resources_set_int("VideoSize", pi->video);
    resources_set_int("Ram9", pi->mem9);
    resources_set_int("RamA", pi->memA);
    resources_set_int("EoiBlank", pi->eoiblank);
    resources_set_int("SuperPET", pi->superpet);

    resources_get_int("KeymapIndex", &kindex);
    resources_set_int("KeymapIndex", (kindex & 1) + 2 * pi->kbd_type);
    return 0;
}

static int pet_set_model_info(petinfo_t *pi)
{
    /* set hardware config */
    petmem_set_conf_info(pi);

    if (pi->pet2k) {    /* set resource only when necessary */
        resources_set_int("Basic1", pi->pet2k);
    }
    resources_set_int("Basic1Chars", pi->pet2kchar);

    resources_set_string("ChargenName", pi->chargenName);
    resources_set_string("KernalName", pi->kernalName);
    resources_set_string("BasicName", pi->basicName);
    resources_set_string("EditorName", pi->editorName);

    /* allow additional ROMs to survive a model switch. */
    if (pi->mem9name)
        resources_set_string("RomModule9Name", pi->mem9name);
    if (pi->memAname)
        resources_set_string("RomModuleAName", pi->memAname);
    if (pi->memBname)
        resources_set_string("RomModuleBName", pi->memBname);
    if (pi->superpet) {
        int i;

        for (i = 0; i < NUM_6809_ROMS; i++) {
            if (pi->h6809romName[i]) {
                resources_set_string_sprintf("H6809Rom%cName",
                                            pi->h6809romName[i],
                                            'A'+i);
            }
        }
    }
    return 0;
}

static int pet_model = 8;

/* FIXME: this one should only be used by commandline */
int pet_set_model(const char *model_name, void *extra)
{
    int i;

    i = 0;
    while (pet_table[i].model) {
        if (!strcmp(pet_table[i].model, model_name)) {
            petmodel_set(i);
            return 0;
        }
        i++;
    }

    return -1;
}

int petmodel_get(void)
{
    return pet_model;
}

void petmodel_set(int model)
{
    petres.video = -1; /* force reinitialization in pet-resources.c:set_video, see bug #3496413 */
    pet_set_model_info(&pet_table[model].info);

    /* we have to wait until we have done enough initialization */
    if (pet_init_ok) {
        /* mem_load(); - not needed as resources now load */
        vsync_suspend_speed_eval();
        machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
        pet_model = model;
    }
 }
