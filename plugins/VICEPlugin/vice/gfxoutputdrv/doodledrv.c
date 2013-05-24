/*
 * doodledrv.c - Create a c64 doodle type file.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdlib.h>

#include "archdep.h"
#include "cmdline.h"
#include "doodledrv.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "gfxoutput.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vsync.h"

/* TODO:
 * - add VICII FLI / mixed mode handling
 * - add VICII super hires handling
 * - add VICII super hires FLI handling
 * - add VDC text mode
 * - add VDC bitmap mode
 * - add TED FLI / mixed mode handling
 * - add TED hires mode lum handling
 * - add TED multi-color text mode
 * - add TED multi-color bitmap mode
 * - add VIC mixed mode handling
 * - add possible CRTC mixed mode handling
 * - add PET CRTC DWW handling
 * - add C64DTV specific modes handling
 */

#if defined(__BEOS__) && defined(WORDS_BIGENDIAN)
extern gfxoutputdrv_t doodle_drv;
#else
static gfxoutputdrv_t doodle_drv;
#endif

static BYTE *regs = NULL;

/* ------------------------------------------------------------------------ */

static int oversize_handling;
static int multicolor_handling;
static int ted_lum_handling;
static int crtc_text_color;
static BYTE crtc_fgcolor;

static int set_oversize_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_OVERSIZE_SCALE:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
        case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
        case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
            oversize_handling = val;
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

static int set_multicolor_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_MC2HR_BLACK_WHITE:
        case NATIVE_SS_MC2HR_2_COLORS:
        case NATIVE_SS_MC2HR_4_COLORS:
        case NATIVE_SS_MC2HR_GRAY:
        case NATIVE_SS_MC2HR_DITHER:
            multicolor_handling = val;
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

static int set_ted_lum_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_TED_LUM_IGNORE:
        case NATIVE_SS_TED_LUM_DITHER:
            ted_lum_handling = val;
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

static int set_crtc_text_color(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_CRTC_WHITE:
            crtc_fgcolor = 1;
            break;
        case NATIVE_SS_CRTC_AMBER:
            crtc_fgcolor = 8;
            break;
        case NATIVE_SS_CRTC_GREEN:
            crtc_fgcolor = 5;
            break;
        default:
            return -1;
            break;
    }
    crtc_text_color = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "DoodleOversizeHandling", NATIVE_SS_OVERSIZE_SCALE, RES_EVENT_NO, NULL,
      &oversize_handling, set_oversize_handling, NULL },
    { "DoodleMultiColorHandling", NATIVE_SS_MC2HR_2_COLORS, RES_EVENT_NO, NULL,
      &multicolor_handling, set_multicolor_handling, NULL },
    { "DoodleTEDLumHandling", NATIVE_SS_TED_LUM_IGNORE, RES_EVENT_NO, NULL,
      &ted_lum_handling, set_ted_lum_handling, NULL },
    { "DoodleCRTCTextColor", NATIVE_SS_CRTC_WHITE, RES_EVENT_NO, NULL,
      &crtc_text_color, set_crtc_text_color, NULL },
    { NULL }
};

static int doodledrv_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
    { "-doodleoversize", SET_RESOURCE, 1,
      NULL, NULL, "DoodleOversizeHandling", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_OVERSIZED_HANDLING,
      NULL, NULL },
    { "-doodlemc", SET_RESOURCE, 1,
      NULL, NULL, "DoodleMultiColorHandling", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_MULTICOLOR_HANDLING,
      NULL, NULL },
    { "-doodletedlum", SET_RESOURCE, 1,
      NULL, NULL, "DoodleTEDLumHandling", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_TED_LUM_HANDLING,
      NULL, NULL },
    { "-doodlecrtctextcolor", SET_RESOURCE, 1,
      NULL, NULL, "DoodleCRTCTextColor", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_COLOR, IDCLS_CRTC_TEXT_COLOR,
      NULL, NULL },
    { NULL }
};

static int doodledrv_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------ */

static void doodle_smooth_scroll_borderize_colormap(doodle_data_t *source, BYTE bordercolor, BYTE xcover, BYTE ycover)
{
    int i, j, k;
    int xstart = 0;
    int xsize;
    int xendamount = 0;
    int ystart = 0;
    int ysize;
    int yendamount = 0;

    if (xcover == 255) {
        xstart = 0;
        xsize = source->xsize;
        xendamount = 0;
    } else {
        xstart = 7 - xcover;
        xsize = source->xsize - 16;
        xendamount = 16 - xstart;
    }

    if (ycover == 255) {
        ystart = 0;
        ysize = source->ysize;
        yendamount = 0;
    } else {
        ystart = 7 - ycover;
        ysize = source->ysize - 8;
        yendamount = 8 - ystart;
    }

    k = 0;

    /* render top border if needed */
    for (i = 0; i < ystart; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[k++] = bordercolor;
        }
    }

    for (i = 0; i < ysize; i++) {

        /* render left border if needed */
        for (j = 0; j < xstart; j++) {
            source->colormap[k++] = bordercolor;
        }

        /* skip screen data */
        k += xsize;

        /* render right border if needed */
        for (j = 0; j < xendamount; j++) {
            source->colormap[k++] = bordercolor;
        }
    }

    /* render bottom border if needed */
    for (i = 0; i < yendamount; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[k++] = bordercolor;
        }
    }
}

static doodle_data_t *doodle_borderize_colormap(doodle_data_t *source, BYTE bordercolor)
{
    int i, j, k, l;
    int xstart = 0;
    int xendamount = 0;
    int ystart = 0;
    int yendamount = 0;
    doodle_data_t *dest = lib_malloc(sizeof(doodle_data_t));

    dest->filename = source->filename;

    if (source->xsize < 320) {
        dest->xsize = 320;
        xstart = ((320 - source->xsize) / 16) * 8;
        xendamount = 320 - xstart - source->xsize;
    } else {
        dest->xsize = source->xsize;
    }

    if (source->ysize < 200) {
        dest->ysize = 200;
        ystart = ((200 - source->ysize) / 16) * 8;
        yendamount = 200 - ystart - source->ysize;
    } else {
        dest->ysize = source->ysize;
    }

    dest->colormap = lib_malloc(dest->xsize * dest->ysize);

    k = 0;
    l = 0;

    /* render top border if needed */
    for (i = 0; i < ystart; i++) {
        for (j = 0; j < dest->xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    for (i = 0; i < source->ysize; i++) {

        /* render left border if needed */
        for (j = 0; j < xstart; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* copy screen data */
        for (j = 0; j < source->xsize; j++) {
            dest->colormap[k++] = source->colormap[l++];
        }

        /* render right border if needed */
        for (j = 0; j < xendamount; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    /* render bottom border if needed */
    for (i = 0; i < yendamount; i++) {
        for (j = 0; j < dest->xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

static doodle_data_t *doodle_crop_and_borderize_colormap(doodle_data_t *source, BYTE bordercolor)
{
    int startx;
    int starty;
    int skipxstart = 0;
    int skipxend = 0;
    int skipystart = 0;
    int i, j, k, l;
    doodle_data_t *dest = lib_malloc(sizeof(doodle_data_t));

    dest->filename = source->filename;

    startx = (320 - source->xsize) / 2;
    starty = (200 - source->ysize) / 2;

    if (source->xsize > 320) {
        dest->xsize = 320;
    } else {
        dest->xsize = source->xsize;
    }

    if (source->ysize > 200) {
        dest->ysize = 200;
    } else {
        dest->ysize = source->ysize;
    }

    dest->colormap = lib_malloc(dest->xsize * dest->ysize);

    if (startx < 0) {
        switch (oversize_handling) {
            default:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
                skipxend = source->xsize - 320;
                break;
            case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
            case NATIVE_SS_OVERSIZE_CROP_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
                skipxstart = 0 - startx;
                skipxend = source->xsize - 320 - skipxstart;
                break;
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
                skipxstart = source->xsize - 320;
                break;
        }
        startx = 0;
    } else {
        startx = ((320 - source->xsize) / 16) * 8;
    }

    if (starty < 0) {
        switch (oversize_handling) {
            default:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
                break;
            case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
                skipystart = 0 - starty;
                break;
            case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
                skipystart = source->ysize - 200;
                break;
        }
        starty = 0;
    } else {
        starty = ((200 - source->ysize) / 16) * 8;
    }

    k = 0;
    l = 0;

    /* skip top lines for cropping if needed */
    for (i = 0; i < skipystart; i++) {
        for (j = 0; j < source->ysize; j++) {
            l++;
        }
    }

    /* render top border if needed */
    for (i = 0; i < starty; i++) {
        for (j = 0; j < 320; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    for (i = starty; i < starty + dest->ysize; i++) {
        /* skip right part for cropping if needed */
        for (j = 0; j < skipxstart; j++) {
            l++;
        }

        /* render left border if needed */
        for (j = 0; j < startx; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* copy main body */
        for (j = startx; j < startx + dest->xsize; j++) {
            dest->colormap[k++] = source->colormap[l++];
        }

        /* render right border if needed */
        for (j = startx + dest->xsize; j < 320; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* skip right part for cropping if needed */
        for (j = 0; j < skipxend; j++) {
            l++;
        }
    }

    /* render bottom border if needed */
    for (i = starty + dest->ysize; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

static doodle_data_t *doodle_scale_colormap(doodle_data_t *source)
{
    doodle_data_t *dest = lib_malloc(sizeof(doodle_data_t));
    int i, j;
    int xmult, ymult;

    dest->filename = source->filename;

    dest->xsize = 320;
    dest->ysize = 200;

    dest->colormap = lib_malloc(320 * 200);

    xmult = (source->xsize << 8) / 320;
    ymult = (source->ysize << 8) / 200;

    for (i = 0; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            dest->colormap[(i * 320) + j] = source->colormap[(((i * ymult) >> 8) * source->xsize) + ((j * xmult) >> 8)];
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

static doodle_color_sort_t *doodle_sort_colors_colormap(doodle_data_t *source)
{
    int i, j;
    BYTE color;
    int highest;
    int amount;
    int highestindex = 0;
    doodle_color_sort_t *colors = lib_malloc(sizeof(doodle_color_sort_t) * 16);

    for (i = 0; i < 16; i++) {
        colors[i].color = i;
        colors[i].amount = 0;
    }

    /* count the colors used */
    for (i = 0; i < (source->xsize * source->ysize); i++) {
        colors[source->colormap[i]].amount++;
    }

    /* sort colors from highest to lowest */
    for (i = 0; i < 16; i++) {
        highest = 0;
        for (j = i; j < 16; j++) {
            if (colors[j].amount >= highest) {
                highest = colors[j].amount;
                highestindex = j;
            }
        }
        color = colors[i].color;
        amount = colors[i].amount;
        colors[i].color = colors[highestindex].color;
        colors[i].amount = colors[highestindex].amount;
        colors[highestindex].color = color;
        colors[highestindex].amount = amount;
    }
    return colors;
}

static BYTE vicii_color_bw_translate[16] = {
    0,    /* vicii black       (0) -> vicii black (0) */
    1,    /* vicii white       (1) -> vicii white (1) */
    0,    /* vicii red         (2) -> vicii black (0) */
    1,    /* vicii cyan        (3) -> vicii white (1) */
    1,    /* vicii purple      (4) -> vicii white (1) */
    0,    /* vicii green       (5) -> vicii black (0) */
    0,    /* vicii blue        (6) -> vicii black (0) */
    1,    /* vicii yellow      (7) -> vicii white (1) */
    0,    /* vicii orange      (8) -> vicii black (0) */
    0,    /* vicii brown       (9) -> vicii black (0) */
    1,    /* vicii light red   (A) -> vicii white (1) */
    0,    /* vicii dark gray   (B) -> vicii black (0) */
    1,    /* vicii medium gray (C) -> vicii white (1) */
    1,    /* vicii light green (D) -> vicii white (1) */
    1,    /* vicii light blue  (E) -> vicii white (1) */
    1     /* vicii light gray  (F) -> vicii white (1) */
};

static inline BYTE vicii_color_to_bw(BYTE color)
{
    return vicii_color_bw_translate[color];
}

static void doodle_color_to_bw_colormap(doodle_data_t *source)
{
    int i, j;

    for (i = 0; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            source->colormap[(i * 320) + j] = vicii_color_to_bw(source->colormap[(i * 320) + j]);
        }
    }
}

static BYTE vicii_color_gray_translate[16] = {
    0x0,    /* vicii black       (0) -> vicii black       (0) */
    0xF,    /* vicii white       (1) -> vicii light gray  (F) */
    0xB,    /* vicii red         (2) -> vicii dark gray   (B) */
    0xC,    /* vicii cyan        (3) -> vicii medium gray (C) */
    0xC,    /* vicii purple      (4) -> vicii medium gray (C) */
    0xB,    /* vicii green       (5) -> vicii dark gray   (B) */
    0xB,    /* vicii blue        (6) -> vicii dark gray   (B) */
    0xC,    /* vicii yellow      (7) -> vicii medium gray (C) */
    0xC,    /* vicii orange      (8) -> vicii medium gray (C) */
    0xB,    /* vicii brown       (9) -> vicii dark gray   (B) */
    0xC,    /* vicii light red   (A) -> vicii medium gray (C) */
    0xB,    /* vicii dark gray   (B) -> vicii dark gray   (B) */
    0xC,    /* vicii medium gray (C) -> vicii medium gray (C) */
    0xF,    /* vicii light green (D) -> vicii light gray  (F) */
    0xC,    /* vicii light blue  (E) -> vicii medium gray (C) */
    0xF     /* vicii light gray  (F) -> vicii light gray  (F) */
};

static inline BYTE vicii_color_to_gray(BYTE color)
{
    return vicii_color_gray_translate[color];
}

static void doodle_color_to_gray_colormap(doodle_data_t *source)
{
    int i, j;

    for (i = 0; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            source->colormap[(i * 320) + j] = vicii_color_to_gray(source->colormap[(i * 320) + j]);
        }
    }
}

static BYTE vicii_closest_color[16][16] = {
    /* vicii black (0) */
    { 0, 9, 11, 2, 6, 8, 5, 12, 4, 10, 14, 3, 13, 15, 7, 1 },

    /* vicii white (1) */
    { 1, 15, 13, 7, 3, 10, 14, 12, 4, 5, 11, 8, 6, 2, 9, 0 },

    /* vicii red (2) */
    { 2, 8, 9, 11, 0, 10, 12, 5, 4, 6, 7, 14, 15, 3, 13, 1 },

    /* vicii cyan (3) */
    { 3, 13, 14, 15, 12, 10, 5, 7, 4, 11, 1, 6, 8, 9, 2, 0 },

    /* vicii purple (4) */
    { 4, 10, 12, 11, 15, 14, 6, 8, 2, 3, 13, 9, 7, 5, 1, 0 },

    /* vicii green (5) */
    { 5, 11, 12, 8, 9, 3, 10, 2, 13, 7, 14, 15, 0, 4, 6, 1 },

    /* vicii blue (6) */
    { 6, 11, 9, 0, 4, 12, 14, 2, 8, 10, 3, 5, 13, 15, 7, 1 },

    /* vicii yellow (7) */
    { 7, 13, 15, 10, 3, 12, 1, 5, 8, 4, 14, 11, 2, 9, 6, 0 },

    /* vicii orange (8) */
    { 8, 2, 9, 11, 10, 5, 12, 4, 0, 7, 6, 15, 3, 14, 13, 1 },

    /* vicii brown (9) */
    { 9, 11, 2, 0, 8, 6, 5, 12, 4, 10, 14, 3, 15, 13, 7, 1 },

    /* vicii light red (10) */
    { 10, 12, 4, 15, 7, 8, 3, 11, 13, 14, 2, 5, 9, 1, 6, 0 },

    /* vicii dark gray (11) */
    { 11, 9, 12, 6, 2, 8, 5, 0, 4, 10, 14, 3, 15, 13, 7, 1 },

    /* vicii medium gray (12) */
    { 12, 10, 4, 3, 14, 11, 15, 5, 13, 8, 9, 6, 7, 2, 1, 0 },

    /* vicii light green (13) */
    { 13, 3, 15, 7, 12, 15, 1, 10, 5, 4, 11, 8, 9, 2, 6, 0 },

    /* vicii light blue (14) */
    { 14, 3, 12, 11, 4, 13, 6, 11, 10, 5, 9, 1, 7, 8, 2, 0 },

    /* vicii light gray (15) */
    { 15, 13, 3, 12, 14, 10, 7, 1, 4, 5, 11, 8, 6, 9, 2, 0 }
};

static inline BYTE vicii_color_to_nearest_color(BYTE color, doodle_color_sort_t *altcolors)
{
    int i, j;

    for (i = 0; i < 16; i++) {
        for (j = 0; altcolors[j].color != 255; j++) {
            if (vicii_closest_color[color][i] == altcolors[j].color) {
                return vicii_closest_color[color][i];
            }
        }
    }
    return 0;
}

static void doodle_color_to_nearest_colors_colormap(doodle_data_t *source, doodle_color_sort_t *colors)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] = vicii_color_to_nearest_color(source->colormap[(i * source->xsize) +  j], colors);
        }
    }
}

static void doodle_check_and_correct_cell(doodle_data_t *source)
{
    doodle_data_t *dest = lib_malloc(sizeof(doodle_data_t));
    int i, j, k, l;
    doodle_color_sort_t *colors = NULL;

    dest->xsize = 8;
    dest->ysize = 8;
    dest->colormap = lib_malloc(8 * 8);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dest->colormap[(k * 8) + l] = source->colormap[(i * 8 * 320) + (j * 8) + (k * 320) + l];
                }
            }
            colors = doodle_sort_colors_colormap(dest);
            if (colors[2].amount !=0) {
                colors[2].color = 255;
                doodle_color_to_nearest_colors_colormap(dest, colors);
                for (k = 0; k < 8; k++) {
                    for (l = 0; l < 8; l++) {
                        source->colormap[(i * 8 * 320) + (j * 8) + (k * 320) + l] = dest->colormap[(k * 8) + l];
                    }
                }
            }
            lib_free(colors);
        }
    }
    lib_free(dest->colormap);
    lib_free(dest);
}

static int doodle_render_and_save(doodle_data_t *source)
{
    BYTE load_adr_low = 0;
    BYTE load_adr_high = 0x1c;
    FILE *fd;
    char *filename_ext = NULL;
    BYTE *bitmap = NULL;
    BYTE *videoram = NULL;
    int i, j, k, l;
    int m = 0;
    int n = 0;
    int retval = 0;
    BYTE fgcolor = 0;
    BYTE bgcolor;
    BYTE colorbyte;

    bitmap = lib_malloc(8000);
    videoram = lib_malloc(1000);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            bgcolor = 255;
            for (k = 0; k < 8; k++) {
                bitmap[m] = 0;
                for (l = 0; l < 8; l++) {
                    colorbyte = source->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l];
                    if (k == 0 && l == 0) {
                        fgcolor = colorbyte;
                    }
                    if (colorbyte == fgcolor) {
                        bitmap[m] |= (1 << (7 - l));
                    } else {
                        if (colorbyte != bgcolor) {
                            bgcolor = colorbyte;
                        }
                    }
                }
                m++;
            }
            videoram[n++] = ((fgcolor & 0xf) << 4) | (bgcolor & 0xf);
        }
    }

    filename_ext = util_add_extension_const(source->filename, doodle_drv.default_extension);
    fd = fopen(filename_ext, MODE_WRITE);
    if (fd == NULL) {
        retval = -1;
    }

    if (retval != -1) {
        if (fwrite(&load_adr_low, 1, 1, fd) < 1) {
            retval = -1;
        }
    }

    if (retval != -1) {
        if (fwrite(&load_adr_high, 1, 1, fd) < 1) {
            retval = -1;
        }
    }

    if (retval != -1) {
        if (fwrite(videoram, 1000, 1, fd) < 1) {
            retval = -1;
        }
    }

    for (i = 0; i < 24 && retval != -1; i++) {
        if (fwrite(&load_adr_low, 1, 1, fd) < 1) {
            retval = -1;
        }
    }

    if (retval != -1) {
        if (fwrite(bitmap, 8000, 1, fd) < 1) {
            retval = -1;
        }
    }

    for (i = 0; i < 192 && retval != -1; i++) {
        if (fwrite(&load_adr_low, 1, 1, fd) < 1) {
            retval = -1;
        }
    }

    if (fd != NULL) {
        fclose(fd);
    }

    lib_free(source->colormap);
    lib_free(source);
    lib_free(filename_ext);
    lib_free(bitmap);
    lib_free(videoram);

    return 0;
}

/* ------------------------------------------------------------------------ */

static int doodle_vicii_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    int i, j, k, l;
    doodle_data_t *data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    bgcolor = regs[0x21] & 0xf;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        doodle_smooth_scroll_borderize_colormap(data, (BYTE)(regs[0x20] & 0xf), (BYTE)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (BYTE)((regs[0x11]  & 8) ? 255 : regs[0x11] & 7));
    }
    return doodle_render_and_save(data);
}

static int doodle_vicii_extended_background_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    int i, j, k, l;
    doodle_data_t *data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            bgcolor = regs[0x21 + ((screenshot->screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x3f) * 8) + k];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        doodle_smooth_scroll_borderize_colormap(data, (BYTE)(regs[0x20] & 0xf), (BYTE)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (BYTE)((regs[0x11]  & 8) ? 255 : regs[0x11] & 7));
    }
    return doodle_render_and_save(data);
}

static int doodle_vicii_multicolor_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE color0;
    BYTE color1;
    BYTE color2;
    BYTE color3;
    int i, j, k, l;
    doodle_data_t *data = lib_malloc(sizeof(doodle_data_t));
    doodle_color_sort_t *color_order = NULL;
    int mc_data_present = 0;

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    color0 = regs[0x21] & 0xf;
    color1 = regs[0x22] & 0xf;
    color2 = regs[0x23] & 0xf;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            color3 = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                if (color3 & 8) {
                    for (l = 0; l < 4; l++) {
                        mc_data_present = 1;
                        switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                            case 0:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
                                break;
                            case 1:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
                                break;
                            case 2:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
                                break;
                            case 3:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3 & 7;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3 & 7;
                                break;
                        }
                    }
                } else {
                    for (l = 0; l < 8; l++) {
                        if (bitmap & (1 << (7 - l))) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color3;
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color0;
                        }
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        doodle_smooth_scroll_borderize_colormap(data, (BYTE)(regs[0x20] & 0xf), (BYTE)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (BYTE)((regs[0x11]  & 8) ? 255 : regs[0x11] & 7));
    }
    if (mc_data_present) {
        switch (multicolor_handling) {
            case NATIVE_SS_MC2HR_BLACK_WHITE:
                doodle_color_to_bw_colormap(data);
                break;
            case NATIVE_SS_MC2HR_GRAY:
                doodle_color_to_gray_colormap(data);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_2_COLORS:
                color_order = doodle_sort_colors_colormap(data);
                color_order[2].color = 255;
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_4_COLORS:
                color_order = doodle_sort_colors_colormap(data);
                color_order[4].color = 255;
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_DITHER:
                color_order = doodle_sort_colors_colormap(data);
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            default:
                return -1;
                break;
        }
    }
    return doodle_render_and_save(data);
}

static int doodle_vicii_hires_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    int i, j, k, l;
    doodle_data_t *data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            bgcolor = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                if (((i * 40 * 8) + (j * 8) + k) < 4096) {
                    bitmap = screenshot->bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
                } else {
                    bitmap = screenshot->bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        doodle_smooth_scroll_borderize_colormap(data, (BYTE)(regs[0x20] & 0xf), (BYTE)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (BYTE)((regs[0x11]  & 8) ? 255 : regs[0x11] & 7));
    }
    return doodle_render_and_save(data);
}

static int doodle_vicii_multicolor_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE color0;
    BYTE color1;
    BYTE color2;
    BYTE color3;
    int i, j, k, l;
    doodle_data_t *data = NULL;
    doodle_color_sort_t *color_order = NULL;

    data = lib_malloc(sizeof(doodle_data_t));
    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    color0 = regs[0x21] & 0xf;
    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            color1 = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            color2 = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            color3 = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                if (((i * 40 * 8) + (j * 8) + k) < 4096) {
                    bitmap = screenshot->bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
                } else {
                    bitmap = screenshot->bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
                }
                for (l = 0; l < 4; l++) {
                    switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                        case 0:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
                            break;
                        case 1:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
                            break;
                        case 2:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
                            break;
                        case 3:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3;
                            break;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        doodle_smooth_scroll_borderize_colormap(data, (BYTE)(regs[0x20] & 0xf), (BYTE)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (BYTE)((regs[0x11]  & 8) ? 255 : regs[0x11] & 7));
    }
    switch (multicolor_handling) {
        case NATIVE_SS_MC2HR_BLACK_WHITE:
            doodle_color_to_bw_colormap(data);
            break;
        case NATIVE_SS_MC2HR_GRAY:
            doodle_color_to_gray_colormap(data);
            doodle_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_2_COLORS:
            color_order = doodle_sort_colors_colormap(data);
            color_order[2].color = 255;
            doodle_color_to_nearest_colors_colormap(data, color_order);
            lib_free(color_order);
            doodle_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_4_COLORS:
            color_order = doodle_sort_colors_colormap(data);
            color_order[4].color = 255;
            doodle_color_to_nearest_colors_colormap(data, color_order);
            lib_free(color_order);
            doodle_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_DITHER:
            color_order = doodle_sort_colors_colormap(data);
            doodle_color_to_nearest_colors_colormap(data, color_order);
            lib_free(color_order);
            doodle_check_and_correct_cell(data);
            break;
        default:
            return -1;
            break;
    }
    return doodle_render_and_save(data);
}

static int doodle_vicii_save(screenshot_t *screenshot, const char *filename)
{
    BYTE mc;
    BYTE eb;
    BYTE bm;
    BYTE blank;

    regs = screenshot->video_regs;
    mc = (regs[0x16] & 0x10) >> 4;
    eb = (regs[0x11] & 0x40) >> 6;
    bm = (regs[0x11] & 0x20) >> 5;

    blank = (regs[0x11] & 0x10) >> 4;

    if (!blank) {
        ui_error("Screen is blanked, no picture to save");
        return -1;
    }

    switch (mc << 2 | eb << 1 | bm) {
        case 0:    /* normal text mode */
            return doodle_vicii_text_mode_render(screenshot, filename);
            break;
        case 1:    /* hires bitmap mode */
            return doodle_vicii_hires_bitmap_mode_render(screenshot, filename);
            break;
        case 2:    /* extended background mode */
            return doodle_vicii_extended_background_mode_render(screenshot, filename);
            break;
        case 4:    /* multicolor text mode */
            return doodle_vicii_multicolor_text_mode_render(screenshot, filename);
            break;
        case 5:    /* multicolor bitmap mode */
            return doodle_vicii_multicolor_bitmap_mode_render(screenshot, filename);
            break;
        default:   /* illegal modes (3, 6 and 7) */
            ui_error("Illegal mode, no saving will be done");
            return -1;
            break;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */

static BYTE ted_vicii_translate[16] = {
    0x0,    /* ted black        (0) -> vicii black       (0) */
    0x1,    /* ted white        (1) -> vicii white       (1) */
    0x2,    /* ted red          (2) -> vicii red         (2) */
    0x3,    /* ted cyan         (3) -> vicii cyan        (3) */
    0x4,    /* ted purple       (4) -> vicii purple      (4) */
    0x5,    /* ted green        (5) -> vicii green       (5) */
    0x6,    /* ted blue         (6) -> vicii blue        (6) */
    0x7,    /* ted yellow       (7) -> vicii yellow      (7) */
    0x8,    /* ted orange       (8) -> vicii orange      (8) */
    0x9,    /* ted brown        (9) -> vicii brown       (9) */
    0xD,    /* ted yellow-green (A) -> vicii light green (D) */
    0xA,    /* ted pink         (B) -> vicii light red   (A) */
    0xE,    /* ted blue-green   (C) -> vicii light blue  (E) */
    0xE,    /* ted light blue   (D) -> vicii light blue  (E) */
    0x6,    /* ted dark blue    (E) -> vicii blue        (6) */
    0xD     /* ted light green  (F) -> vicii light green (D) */
};

static inline BYTE ted_to_vicii_color(BYTE color)
{
    return ted_vicii_translate[color];
}

static BYTE ted_lum_vicii_translate[16 * 8] = {
    0x0,    /* ted black L0        (0) -> vicii black     (0) */
    0x9,    /* ted white L0        (1) -> vicii brown     (9) */
    0x2,    /* ted red L0          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L0         (3) -> vicii dark gray (B) */
    0x6,    /* ted purple L0       (4) -> vicii blue      (6) */
    0x0,    /* ted green L0        (5) -> vicii black     (0) */
    0x6,    /* ted blue L0         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L0       (7) -> vicii brown     (9) */
    0x9,    /* ted orange L0       (8) -> vicii brown     (9) */
    0x9,    /* ted brown L0        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L0 (A) -> vicii brown     (9) */
    0x9,    /* ted pink L0         (B) -> vicii brown     (9) */
    0x0,    /* ted blue-green L0   (C) -> vicii black     (0) */
    0x6,    /* ted light blue L0   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L0    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L0  (F) -> vicii brown     (9) */

    0x0,    /* ted black L1        (0) -> vicii black     (0) */
    0xB,    /* ted white L1        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L1          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L1         (3) -> vicii dark gray (B) */
    0x6,    /* ted purple L1       (4) -> vicii blue      (6) */
    0x9,    /* ted green L1        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L1         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L1       (7) -> vicii brown     (9) */
    0x2,    /* ted orange L1       (8) -> vicii red       (2) */
    0x9,    /* ted brown L1        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L1 (A) -> vicii brown     (9) */
    0xB,    /* ted pink L1         (B) -> vicii dark gray (B) */
    0xB,    /* ted blue-green L1   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L1   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L1    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L1  (F) -> vicii brown     (9) */

    0x0,    /* ted black L2        (0) -> vicii black     (0) */
    0xB,    /* ted white L2        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L2          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L2         (3) -> vicii dark gray (B) */
    0x4,    /* ted purple L2       (4) -> vicii purple    (4) */
    0x9,    /* ted green L2        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L2         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L2       (7) -> vicii brown     (9) */
    0x2,    /* ted orange L2       (8) -> vicii red       (2) */
    0x9,    /* ted brown L2        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L2 (A) -> vicii brown     (9) */
    0xB,    /* ted pink L2         (B) -> vicii dark gray (B) */
    0xB,    /* ted blue-green L2   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L2   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L2    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L2  (F) -> vicii brown     (9) */

    0x0,    /* ted black L3        (0) -> vicii black     (0) */
    0xB,    /* ted white L3        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L3          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L3         (3) -> vicii dark gray (B) */
    0x4,    /* ted purple L3       (4) -> vicii purple    (4) */
    0x9,    /* ted green L3        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L3         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L3       (7) -> vicii brown     (9) */
    0x8,    /* ted orange L3       (8) -> vicii orange    (8) */
    0x8,    /* ted brown L3        (9) -> vicii orange    (8) */
    0x9,    /* ted yellow-green L3 (A) -> vicii brown     (9) */
    0x4,    /* ted pink L3         (B) -> vicii purple    (4) */
    0xB,    /* ted blue-green L3   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L3   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L3    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L3  (F) -> vicii brown     (9) */

    0x0,    /* ted black L4        (0) -> vicii black       (0) */
    0xC,    /* ted white L4        (1) -> vicii medium gray (C) */
    0xA,    /* ted red L4          (2) -> vicii light red   (A) */
    0xE,    /* ted cyan L4         (3) -> vicii light blue  (E) */
    0x4,    /* ted purple L4       (4) -> vicii purple      (4) */
    0x5,    /* ted green L4        (5) -> vicii green       (5) */
    0xE,    /* ted blue L4         (6) -> vicii light blue  (E) */
    0x5,    /* ted yellow L4       (7) -> vicii green       (5) */
    0xA,    /* ted orange L4       (8) -> vicii light red   (A) */
    0x8,    /* ted brown L4        (9) -> vicii orange      (8) */
    0x5,    /* ted yellow-green L4 (A) -> vicii green       (5) */
    0x4,    /* ted pink L4         (B) -> vicii purple      (4) */
    0xC,    /* ted blue-green L4   (C) -> vicii medium gray (C) */
    0xE,    /* ted light blue L4   (D) -> vicii light blue  (E) */
    0xE,    /* ted dark blue L4    (E) -> vicii light blue  (E) */
    0x5,    /* ted light green L4  (F) -> vicii green       (5) */

    0x0,    /* ted black L5        (0) -> vicii black       (0) */
    0xC,    /* ted white L5        (1) -> vicii medium gray (C) */
    0xA,    /* ted red L5          (2) -> vicii light red   (A) */
    0x3,    /* ted cyan L5         (3) -> vicii cyan        (3) */
    0xF,    /* ted purple L5       (4) -> vicii light gray  (F) */
    0x5,    /* ted green L5        (5) -> vicii green       (5) */
    0xE,    /* ted blue L5         (6) -> vicii light blue  (E) */
    0x5,    /* ted yellow L5       (7) -> vicii green       (5) */
    0xA,    /* ted orange L5       (8) -> vicii light red   (A) */
    0xA,    /* ted brown L5        (9) -> vicii light red   (A) */
    0x5,    /* ted yellow-green L5 (A) -> vicii green       (5) */
    0xA,    /* ted pink L5         (B) -> vicii light red   (A) */
    0x3,    /* ted blue-green L5   (C) -> vicii cyan        (3) */
    0xE,    /* ted light blue L5   (D) -> vicii light blue  (E) */
    0xE,    /* ted dark blue L5    (E) -> vicii light blue  (E) */
    0x5,    /* ted light green L5  (F) -> vicii green       (5) */

    0x0,    /* ted black L6        (0) -> vicii black       (0) */
    0xF,    /* ted white L6        (1) -> vicii light gray  (F) */
    0xA,    /* ted red L6          (2) -> vicii light red   (A) */
    0x3,    /* ted cyan L6         (3) -> vicii cyan        (3) */
    0xF,    /* ted purple L6       (4) -> vicii light gray  (F) */
    0xD,    /* ted green L6        (5) -> vicii light green (D) */
    0xF,    /* ted blue L6         (6) -> vicii light gray  (F) */
    0x7,    /* ted yellow L6       (7) -> vicii yellow      (7) */
    0xA,    /* ted orange L6       (8) -> vicii light red   (A) */
    0xA,    /* ted brown L6        (9) -> vicii light red   (A) */
    0x7,    /* ted yellow-green L6 (A) -> vicii yellow      (7) */
    0xF,    /* ted pink L6         (B) -> vicii light gray  (F) */
    0x3,    /* ted blue-green L6   (C) -> vicii cyan        (3) */
    0xF,    /* ted light blue L6   (D) -> vicii light gray  (F) */
    0xF,    /* ted dark blue L6    (E) -> vicii light gray  (F) */
    0x7,    /* ted light green L6  (F) -> vicii yellow      (7) */

    0x0,    /* ted black L7        (0) -> vicii black       (0) */
    0x1,    /* ted white L7        (1) -> vicii white       (1) */
    0x1,    /* ted red L7          (2) -> vicii white       (1) */
    0x1,    /* ted cyan L7         (3) -> vicii white       (1) */
    0x1,    /* ted purple L7       (4) -> vicii white       (1) */
    0xD,    /* ted green L7        (5) -> vicii light green (D) */
    0x1,    /* ted blue L7         (6) -> vicii white       (1) */
    0x7,    /* ted yellow L7       (7) -> vicii yellow      (7) */
    0xF,    /* ted orange L7       (8) -> vicii light gray  (F) */
    0x7,    /* ted brown L7        (9) -> vicii yellow      (7) */
    0x7,    /* ted yellow-green L7 (A) -> vicii yellow      (7) */
    0x1,    /* ted pink L7         (B) -> vicii white       (1) */
    0xD,    /* ted blue-green L7   (C) -> vicii light green (D) */
    0x1,    /* ted light blue L7   (D) -> vicii white       (1) */
    0x1,    /* ted dark blue L7    (E) -> vicii white       (1) */
    0xD     /* ted light green L7  (F) -> vicii light green (D) */
};

static inline BYTE ted_lum_to_vicii_color(BYTE color, BYTE lum)
{
    return ted_lum_vicii_translate[(lum * 16) + color];
}

static int doodle_ted_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    BYTE brdrcolor;
    BYTE fglum;
    BYTE bglum;
    BYTE brdrlum;
    int i, j, k, l;
    doodle_data_t *data;

    data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    bgcolor = regs[0x15] & 0xf;
    bglum = (regs[0x15] & 0x70) >> 4;

    brdrcolor = regs[0x19] & 0xf;
    brdrlum = (regs[0x19] & 0x70) >> 4;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            fglum = (screenshot->color_ram_ptr[(i * 40) + j] & 0x70) >> 4;
            for (k = 0; k < 8; k++) {
                if (regs[0x07] & 0x80) {
                    bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                } else {
                    bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x7f) * 8) + k];
                    if (screenshot->screen_ptr[(i * 40) + j] & 0x80) {
                        bitmap = ~bitmap;
                    }
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(fgcolor,  fglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(fgcolor);
                        }
                    } else {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(bgcolor,  bglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(bgcolor);
                        }
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
            doodle_smooth_scroll_borderize_colormap(data, ted_lum_to_vicii_color(brdrcolor, brdrlum), (BYTE)((regs[0x07] & 8) ? 255  : regs[0x07] & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        } else {
            doodle_smooth_scroll_borderize_colormap(data, ted_to_vicii_color(brdrcolor), (BYTE)((regs[0x07] & 8) ? 255 : regs[0x07]  & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        }
    }
    return doodle_render_and_save(data);
}

static int doodle_ted_extended_background_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    BYTE brdrcolor;
    BYTE fglum;
    BYTE bglum;
    BYTE brdrlum;
    int i, j, k, l;
    doodle_data_t *data;

    data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    brdrcolor = regs[0x19] & 0xf;
    brdrlum = (regs[0x19] & 0x70) >> 4;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            fglum = (screenshot->color_ram_ptr[(i * 40) + j] & 0x70) >> 4;
            bgcolor = regs[0x15 + ((screenshot->screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0xf;
            bglum = (regs[0x15 + ((screenshot->screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0x70) >> 4;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x3f) * 8) + k];
                if ((regs[0x07] & 0x80) && (screenshot->screen_ptr[(i * 40) + j] & 0x80)) {
                    bitmap = ~bitmap;
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(fgcolor,  fglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(fgcolor);
                        }
                    } else {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(bgcolor,  bglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(bgcolor);
                        }
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
            doodle_smooth_scroll_borderize_colormap(data, ted_lum_to_vicii_color(brdrcolor, brdrlum), (BYTE)((regs[0x07] & 8) ? 255  : regs[0x07] & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        } else {
            doodle_smooth_scroll_borderize_colormap(data, ted_to_vicii_color(brdrcolor), (BYTE)((regs[0x07] & 8) ? 255 : regs[0x07]  & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        }
    }
    return doodle_render_and_save(data);
}

static int doodle_ted_hires_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    BYTE brdrcolor;
    BYTE fglum = 0;
    BYTE bglum = 0;
    BYTE brdrlum;
    int i, j, k, l;
    doodle_data_t *data;

    data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    brdrcolor = regs[0x19] & 0xf;
    brdrlum = (regs[0x19] & 0x70) >> 4;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            bgcolor = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->bitmap_ptr[(i * 40 * 8) + j + (k * 40)];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(fgcolor,  fglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(fgcolor);
                        }
                    } else {
                        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_lum_to_vicii_color(bgcolor,  bglum);
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = ted_to_vicii_color(bgcolor);
                        }
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
            doodle_smooth_scroll_borderize_colormap(data, ted_lum_to_vicii_color(brdrcolor, brdrlum), (BYTE)((regs[0x07] & 8) ? 255  : regs[0x07] & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        } else {
            doodle_smooth_scroll_borderize_colormap(data, ted_to_vicii_color(brdrcolor), (BYTE)((regs[0x07] & 8) ? 255 : regs[0x07]  & 7), (BYTE)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
        }
    }
    return doodle_render_and_save(data);
}

static int doodle_ted_save(screenshot_t *screenshot, const char *filename)
{
    BYTE mc;
    BYTE eb;
    BYTE bm;

    regs = screenshot->video_regs;
    mc = (regs[0x07] & 0x10) >> 4;
    eb = (regs[0x06] & 0x40) >> 6;
    bm = (regs[0x06] & 0x20) >> 5;

    switch (mc << 2 | eb << 1 | bm) {
        case 0:    /* normal text mode */
            return doodle_ted_text_mode_render(screenshot, filename);
            break;
        case 1:    /* hires bitmap mode */
            return doodle_ted_hires_bitmap_mode_render(screenshot, filename);
            break;
        case 2:    /* extended background mode */
            return doodle_ted_extended_background_mode_render(screenshot, filename);
            break;
        case 4:    /* multicolor text mode */
            ui_error("This screen saver is a WIP, it doesn't support multicolor text mode (yet)");
            return -1;
            break;
        case 5:    /* multicolor bitmap mode */
            ui_error("This screen saver is a WIP, it doesn't support multicolor bitmap mode (yet)");
            return -1;
            break;
        default:   /* illegal modes (3, 6 and 7) */
            ui_error("Illegal mode, no saving will be done");
            return -1;
            break;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */

static BYTE vic_vicii_translate[16] = {
    0x0,    /* vic black        (0) -> vicii black       (0) */
    0x1,    /* vic white        (1) -> vicii white       (1) */
    0x2,    /* vic red          (2) -> vicii red         (2) */
    0x3,    /* vic cyan         (3) -> vicii cyan        (3) */
    0x4,    /* vic purple       (4) -> vicii purple      (4) */
    0x5,    /* vic green        (5) -> vicii green       (5) */
    0x6,    /* vic blue         (6) -> vicii blue        (6) */
    0x7,    /* vic yellow       (7) -> vicii yellow      (7) */
    0x8,    /* vic orange       (8) -> vicii orange      (8) */
    0x8,    /* vic light orange (9) -> vicii orange      (8) */
    0x8,    /* vic pink         (A) -> vicii orange      (8) */
    0xD,    /* vic light cyan   (B) -> vicii light green (D) */
    0x4,    /* vic light purple (C) -> vicii purple      (4) */
    0xD,    /* vic light green  (D) -> vicii light green (D) */
    0xE,    /* vic light blue   (E) -> vicii light blue  (E) */
    0x7     /* vic light yellow (F) -> vicii yellow      (7) */
};

static inline BYTE vic_to_vicii_color(BYTE color)
{
    return vic_vicii_translate[color];
}

static int doodle_vic_save(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    BYTE brdrcolor;
    BYTE auxcolor;
    int i, j, k, l;
    int mc_data_present = 0;
    doodle_data_t *data;
    doodle_color_sort_t *color_order = NULL;
    BYTE xsize;
    BYTE ysize;
    BYTE charsize;

    regs = screenshot->video_regs;

    xsize = regs[0x02] & 0x7f;
    ysize = (regs[0x03] & 0x7e) >> 1;
    charsize = regs[0x03] & 1;

    if (xsize == 0 || ysize == 0) {
        ui_error("Screen is blank, no save will be done");
        return -1;
    }

    if (screenshot->chargen_ptr == NULL) {
        ui_error("Character generator memory is illegal");
        return -1;
    }

    data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = xsize * 8;
    data->ysize = ysize * 8;

    data->colormap = lib_malloc(data->xsize * data->ysize);

    bgcolor = (regs[0xf] & 0xf0) >> 4;
    auxcolor = (regs[0xe] & 0xf0) >> 4;
    brdrcolor = regs[0xf] & 3;
    for (i = 0; i < ysize; i++) {
        for (j = 0; j < xsize; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * xsize) + j] & 7;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * xsize) + j] * 8) + k];
                if (!(regs[0xf] & 8)) {
                    bitmap = ~bitmap;
                }
                if (screenshot->color_ram_ptr[(i * xsize) + j] & 8) {
                    for (l = 0; l < 4; l++) {
                        mc_data_present = 1;
                        switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                            case 0:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] =  vic_to_vicii_color(bgcolor);
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] =  vic_to_vicii_color(bgcolor);
                                break;
                            case 1:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] =  vic_to_vicii_color(brdrcolor);
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] =  vic_to_vicii_color(brdrcolor);
                                break;
                            case 2:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] =  vic_to_vicii_color(fgcolor);
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] =  vic_to_vicii_color(fgcolor);
                                break;
                            case 3:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] =  vic_to_vicii_color(auxcolor);
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] =  vic_to_vicii_color(auxcolor);
                                break;
                        }
                    }
                } else {
                    for (l = 0; l < 8; l++) {
                        if (bitmap & (1 << (7 - l))) {
                            data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = vic_to_vicii_color (fgcolor);
                        } else {
                            data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = vic_to_vicii_color (bgcolor);
                        }
                    }
                }
            }
        }
    }

    if (ysize > 25) {
        if (oversize_handling == NATIVE_SS_OVERSIZE_SCALE) {
            data = doodle_borderize_colormap(data, (BYTE)(regs[0xf] & 7));
            data = doodle_scale_colormap(data);
        } else {
            data = doodle_crop_and_borderize_colormap(data, (BYTE)(regs[0xf] & 7));
        }
    } else {
        data = doodle_borderize_colormap(data, (BYTE)(regs[0xf] & 7));
    }

    if (mc_data_present) {
        switch (multicolor_handling) {
            case NATIVE_SS_MC2HR_BLACK_WHITE:
                doodle_color_to_bw_colormap(data);
                break;
            case NATIVE_SS_MC2HR_GRAY:
                doodle_color_to_gray_colormap(data);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_2_COLORS:
                color_order = doodle_sort_colors_colormap(data);
                color_order[2].color = 255;
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_4_COLORS:
                color_order = doodle_sort_colors_colormap(data);
                color_order[4].color = 255;
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_DITHER:
                color_order = doodle_sort_colors_colormap(data);
                doodle_color_to_nearest_colors_colormap(data, color_order);
                lib_free(color_order);
                doodle_check_and_correct_cell(data);
                break;
            default:
                return -1;
                break;
        }
    }
    return doodle_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int doodle_crtc_save(screenshot_t *screenshot, const char *filename)
{
    BYTE bitmap;
    BYTE fgcolor;
    BYTE bgcolor;
    int i, j, k, l;
    doodle_data_t *data;
    BYTE xsize;
    BYTE ysize;
    BYTE invert;
    BYTE charheight;
    int shift1;
    int shift2;
    int shiftand;

    regs = screenshot->video_regs;

    switch (screenshot->bitmap_low_ptr[0]) {
        default:
        case 40:
            xsize = regs[0x01];
            shift1 = (regs[0x0c] & 1) ? 256 : 0;
            shift2 = (regs[0x0c] & 2) ? 512 : 0;
            shiftand = 0x3ff;
            break;
        case 60:
            xsize = regs[0x01];
            shift1 = (regs[0x0c] & 1) ? 256 : 0;
            shift2 = (regs[0x0c] & 2) ? 512 : 0;
            shiftand = 0x7ff;
            break;
        case 80:
            xsize = regs[0x01] << 1;
            shift1 = (regs[0x0c] & 1) ? 512 : 0;
            shift2 = (regs[0x0c] & 2) ? 1024 : 0;
            shiftand = 0x7ff;
            break;
    }

    ysize = regs[0x06];
    invert = (regs[0x0c] & 0x10) >> 4;

    if (xsize == 0 || ysize == 0) {
        ui_error("Screen is blank, no save will be done");
        return -1;
    }

    charheight = screenshot->bitmap_high_ptr[0];

    data = lib_malloc(sizeof(doodle_data_t));

    data->filename = filename;

    data->xsize = xsize * 8;
    data->ysize = ysize * charheight;

    data->colormap = lib_malloc(data->xsize * data->ysize);

    bgcolor = 0;
    fgcolor = crtc_fgcolor;
    for (i = 0; i < ysize; i++) {
        for (j = 0; j < xsize; j++) {
            for (k = 0; k < charheight; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[((i * xsize) + j + shift1 + shift2) & shiftand] *  16) + k];
                if (!invert) {
                    bitmap = ~bitmap;
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * data->xsize * charheight) + (j * 8) + (k * data->xsize) + l] = fgcolor;
                    } else {
                        data->colormap[(i * data->xsize * charheight) + (j * 8) + (k * data->xsize) + l] = bgcolor;
                    }
                }
            }
        }
    }

    if (data->xsize > 320 || data->ysize > 200) {
        if (oversize_handling == NATIVE_SS_OVERSIZE_SCALE) {
            data = doodle_borderize_colormap(data, 0);
            data = doodle_scale_colormap(data);
        } else {
            data = doodle_crop_and_borderize_colormap(data, 0);
        }
    } else {
        data = doodle_borderize_colormap(data, 0);
    }
    return doodle_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int doodledrv_save(screenshot_t *screenshot, const char *filename)
{
    if (!(strcmp(screenshot->chipid, "VICII"))) {
        return doodle_vicii_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VDC"))) {
        ui_error("This screen saver is a WIP, it doesn't work for the VDC chip (yet)");
        return -1;
    }
    if (!(strcmp(screenshot->chipid, "CRTC"))) {
        return doodle_crtc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "TED"))) {
        return doodle_ted_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VIC"))) {
        return doodle_vic_save(screenshot, filename);
    }
    ui_error("Unknown graphics chip");
    return -1;
}

static gfxoutputdrv_t doodle_drv =
{
    "DOODLE",
    "C64 doodle screenshot",
    "dd",
    NULL, /* formatlist */
    NULL,
    NULL,
    NULL,
    NULL,
    doodledrv_save,
    NULL,
    NULL,
    doodledrv_resources_init,
    doodledrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    ,NULL
#endif
};

void gfxoutput_init_doodle(void)
{
    gfxoutput_register(&doodle_drv);
}
