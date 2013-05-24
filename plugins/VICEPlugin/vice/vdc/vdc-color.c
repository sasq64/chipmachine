/*
 * vdc-color.c - Colors for the VDC emulation.
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

#include "vdctypes.h"
#include "vdc-color.h"
#include "vdc-resources.h"
#include "video.h"

/*
    FIXME: the VDC (CGA) colors are not evenly saturated, which
    means they can not be accurately defined with the current
    system
*/

/* base saturation of all colors except the grey tones */
#define VDC_SATURATION   (128.0f)

/* phase shift of all colors */

#define VDC_PHASE          0.0f

/* chroma angles in UV space */

#define ANGLE_BLU          0.0f
#define ANGLE_RED        120.0f
#define ANGLE_GRN       -120.0f
#define ANGLE_YEL     ANGLE_BLU // neg
#define ANGLE_BRN        150.0f
#define ANGLE_CYN     ANGLE_RED // neg
#define ANGLE_PUR     ANGLE_GRN // neg
#define ANGLE_BLK          0.0f

/* luminances */
#define LUMA(r,g,b)     (0.2989f * (r) + 0.5866f * (g) + 0.1145f * (b))

#define VDC_LUMA_0      LUMA(  0.0f,   0.0f,   0.0f)
#define VDC_LUMA_1      LUMA( 85.0f,  85.0f,  85.0f)
#define VDC_LUMA_2      LUMA(  0.0f,   0.0f, 170.0f)
#define VDC_LUMA_3      LUMA( 85.0f,  85.0f, 255.0f)
#define VDC_LUMA_4      LUMA(  0.0f, 170.0f,   0.0f)
#define VDC_LUMA_5      LUMA( 85.0f, 255.0f,  85.0f)
#define VDC_LUMA_6      LUMA(  0.0f, 170.0f, 170.0f)
#define VDC_LUMA_7      LUMA( 85.0f, 255.0f, 255.0f)
#define VDC_LUMA_8      LUMA(170.0f,   0.0f,   0.0f)
#define VDC_LUMA_9      LUMA(255.0f,  85.0f,  85.0f)
#define VDC_LUMA_10     LUMA(170.0f,   0.0f, 170.0f)
#define VDC_LUMA_11     LUMA(255.0f,  85.0f, 255.0f)
#define VDC_LUMA_12     LUMA(170.0f,  85.0f,   0.0f)
#define VDC_LUMA_13     LUMA(255.0f, 255.0f,  85.0f)
#define VDC_LUMA_14     LUMA(170.0f, 170.0f, 170.0f)
#define VDC_LUMA_15     LUMA(255.0f, 255.0f, 255.0f)

/* the VDC palette converted to yuv space */

static video_cbm_color_t vdc_colors[VDC_NUM_COLORS]=
{                                                  /*  r g b        y      u      v     sat  hue */
    { VDC_LUMA_0,  ANGLE_BLK, -0, "Black"       }, /* 000000 ->   0.0%   0.0%   0.0%   0.0%      */
    { VDC_LUMA_1,  ANGLE_BLK,  0, "Medium Gray" }, /* 555555 ->  33.3%   0.0%   0.0%   0.0%      */
    { VDC_LUMA_2,  ANGLE_BLU,  1, "Blue"        }, /* 0000AA ->   7.6%  29.1%  -6.7%  66.7%  240 */
    { VDC_LUMA_3,  ANGLE_BLU,  1, "Light Blue"  }, /* 5555FF ->  40.9%  29.1%  -6.7%  66.7%      */
    { VDC_LUMA_4,  ANGLE_GRN,  1, "Green"       }, /* 00AA00 ->  39.1% -19.3% -34.3%  66.7%  120 */
    { VDC_LUMA_5,  ANGLE_GRN,  1, "Light Green" }, /* 55FF55 ->  72.5% -19.3% -34.3%  66.7%      */
    { VDC_LUMA_6,  ANGLE_CYN, -1, "Cyan"        }, /* 00AAAA ->  46.7%   9.8% -41.0%  66.7%  180 */
    { VDC_LUMA_7,  ANGLE_CYN, -1, "Light Cyan"  }, /* 55FFFF ->  80.1%   9.8% -41.0%  66.7%      */
    { VDC_LUMA_8,  ANGLE_RED,  1, "Red"         }, /* AA0000 ->  19.9%  -9.8%  41.0%  66.7%    0 */
    { VDC_LUMA_9,  ANGLE_RED,  1, "Light Red"   }, /* FF5555 ->  53.3%  -9.8%  41.0%  66.7%      */
    { VDC_LUMA_10, ANGLE_PUR, -1, "Purple"      }, /* AA00AA ->  27.5%  19.3%  34.3%  66.7%  -60 */
    { VDC_LUMA_11, ANGLE_PUR, -1, "Light Purple"}, /* FF55FF ->  60.9%  19.3%  34.3%  66.7%      */
    { VDC_LUMA_12, ANGLE_BRN,  1, "Brown"       }, /* AA5500 ->  39.5% -19.4%  23.8%  66.7%   30 */
    { VDC_LUMA_13, ANGLE_YEL, -1, "Yellow"      }, /* FFFF55 ->  92.4% -29.1%   6.7%  66.7%   60 */
    { VDC_LUMA_14, ANGLE_BLK, -0, "Light Gray"  }, /* AAAAAA ->  66.7%   0.0%   0.0%   0.0%      */
    { VDC_LUMA_15, ANGLE_BLK,  0, "White"       }, /* FFFFFF -> 100.0%   0.0%   0.0%   0.0%      */
};

static video_cbm_palette_t vdc_palette =
{
    VDC_NUM_COLORS,
    vdc_colors,
    VDC_SATURATION,
    VDC_PHASE
};

int vdc_color_update_palette(struct video_canvas_s *canvas)
{
    video_color_palette_internal(canvas, &vdc_palette);
    return video_color_update_palette(canvas);
}

