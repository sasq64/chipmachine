/*
 * pet-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  André Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "cmdline.h"
#include "machine.h"
#include "pet-cmdline-options.h"
#include "pets.h"
#include "resources.h"
#include "translate.h"

static const cmdline_option_t cmdline_options[] = {
    { "-pal", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_PAL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PAL_SYNC_FACTOR,
      NULL, NULL },
    { "-ntsc", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSC,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-model", CALL_FUNCTION, 1,
      pet_set_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODELNUMBER, IDCLS_SPECIFY_PET_MODEL,
      NULL, NULL },
    { "-kernal", SET_RESOURCE, 1,
      NULL, NULL, "KernalName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_KERNAL_ROM_NAME,
      NULL, NULL },
    { "-basic", SET_RESOURCE, 1,
      NULL, NULL, "BasicName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BASIC_ROM_NAME,
      NULL, NULL },
    { "-editor", SET_RESOURCE, 1,
      NULL, NULL, "EditorName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EDITOR_ROM_NAME,
      NULL, NULL },
    { "-chargen", SET_RESOURCE, 1,
      NULL, NULL, "ChargenName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CHARGEN_ROM_NAME,
      NULL, NULL },
    { "-rom9", SET_RESOURCE, 1,
      NULL, NULL, "RomModule9Name", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_4K_ROM_9XXX_NAME,
      NULL, NULL },
    { "-romA", SET_RESOURCE, 1,
      NULL, NULL, "RomModuleAName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_4K_ROM_AXXX_NAME,
      NULL, NULL },
    { "-romB", SET_RESOURCE, 1,
      NULL, NULL, "RomModuleBName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_4K_ROM_BXXX_NAME,
      NULL, NULL },
    { "-petram9", SET_RESOURCE, 0,
      NULL, NULL, "Ram9", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_PET8296_ENABLE_4K_RAM_AT_9000,
      NULL, NULL },
    { "+petram9", SET_RESOURCE, 0,
      NULL, NULL, "Ram9", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_PET8296_DISABLE_4K_RAM_AT_9000,
      NULL, NULL },
    { "-petramA", SET_RESOURCE, 0,
      NULL, NULL, "RamA", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_PET8296_ENABLE_4K_RAM_AT_A000,
      NULL, NULL },
    { "+petramA", SET_RESOURCE, 0,
      NULL, NULL, "RamA", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_PET8296_DISABLE_4K_RAM_AT_A000,
      NULL, NULL },
    { "-superpet", SET_RESOURCE, 0,
      NULL, NULL, "SuperPET", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SUPERPET_IO,
      NULL, NULL },
    { "+superpet", SET_RESOURCE, 0,
      NULL, NULL, "SuperPET", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SUPERPET_IO,
      NULL, NULL },
    { "-basic1", SET_RESOURCE, 0,
      NULL, NULL, "Basic1", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_ROM_1_KERNAL_PATCHES,
      NULL, NULL },
    { "+basic1", SET_RESOURCE, 0,
      NULL, NULL, "Basic1", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_ROM_1_KERNAL_PATCHES,
      NULL, NULL },
    { "-basic1char", SET_RESOURCE, 0,
      NULL, NULL, "Basic1Chars", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SWITCH_UPPER_LOWER_CHARSET,
      NULL, NULL },
    { "+basic1char", SET_RESOURCE, 0,
      NULL, NULL, "Basic1Chars", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_NO_SWITCH_UPPER_LOWER_CHARSET,
      NULL, NULL },
    { "-eoiblank", SET_RESOURCE, 0,
      NULL, NULL, "EoiBlank", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_EOI_BLANKS_SCREEN,
      NULL, NULL },
    { "+eoiblank", SET_RESOURCE, 0,
      NULL, NULL, "EoiBlank", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_EOI_DOES_NOT_BLANK_SCREEN,
      NULL, NULL },
#ifdef COMMON_KBD
    { "-keymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapIndex", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_KEYMAP_INDEX,
      NULL, NULL },
    { "-grsymkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapGraphicsSymFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_GFX_SYM_KEYMAP_NAME,
      NULL, NULL },
    { "-grposkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapGraphicsPosFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_GFX_POS_KEYMAP_NAME,
      NULL, NULL },
    { "-buksymkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapBusinessUKSymFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BUK_SYM_KEYMAP_NAME,
      NULL, NULL },
    { "-bukposkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapBusinessUKPosFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BUK_POS_KEYMAP_NAME,
      NULL, NULL },
    { "-bdesymkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapBusinessDESymFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BDE_SYM_KEYMAP_NAME,
      NULL, NULL },
    { "-bdeposkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapBusinessDEPosFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BDE_POS_KEYMAP_NAME,
      NULL, NULL },
#endif
    { "-cpu6502", SET_RESOURCE, 0,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_6502,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Set SuperPET CPU switch to '6502'" },
    { "-cpu6809", SET_RESOURCE, 0,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_6809,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Set SuperPET CPU switch to '6809'" },
    { "-cpuprog", SET_RESOURCE, 0,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_PROG,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Set SuperPET CPU switch to 'Prog'" },
    { "-6809romA", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomAName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED, 
      NULL, "Specify 4K to 24K ROM file name at $A000 for 6809" },
    { "-6809romB", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomBName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Specify 4K to 20K ROM file name at $B000 for 6809" },
    { "-6809romC", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomCName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Specify 4K to 16K ROM file name at $C000 for 6809" },
    { "-6809romD", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomDName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Specify 4K to 12K ROM file name at $D000 for 6809" },
    { "-6809romE", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomEName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Specify 2K or 8K ROM file name at $E000 for 6809" },
    { "-6809romF", SET_RESOURCE, 1,
      NULL, NULL, "H6809RomFName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_STRING,
      IDCLS_P_NAME, IDCLS_UNUSED,
      NULL, "Specify 4K ROM file name at $F000 for 6809" },
    { NULL }
};

int pet_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
