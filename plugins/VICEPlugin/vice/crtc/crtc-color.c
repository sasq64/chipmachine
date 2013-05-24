/*
 * crtc-color.c - Colors for the CRTC emulation.
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

#include "crtctypes.h"
#include "crtc-color.h"
#include "crtc-resources.h"
#include "video.h"

/* base saturation */
#define CRTC_SATURATION  150.0f

/* phase shift of all colors */
#define CRTC_PHASE         0.5f

/* chroma angles in UV space */
#define ANGLE_ORN        -45.0f /* negative orange (orange is at +135.0 degree) */
#define ANGLE_GRN       -135.0f

static video_cbm_color_t crtc_colors[CRTC_NUM_COLORS]=
{
    {   0.0f, ANGLE_ORN, -0, "Black"       },
    { 192.0f, ANGLE_GRN,  1, "Green"       },
};

static video_cbm_palette_t crtc_palette =
{
    CRTC_NUM_COLORS,
    crtc_colors,
    CRTC_SATURATION,
    CRTC_PHASE
};

int crtc_color_update_palette(struct video_canvas_s *canvas)
{
    video_color_palette_internal(canvas, &crtc_palette);
    return video_color_update_palette(canvas);
}
