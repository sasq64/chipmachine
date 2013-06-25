/*
 * renderyuv.c - YUV rendering.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
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


/* The PAL Y/C and PAL Composite emulation is based on work by John
   Selck <graham@cruise.de>. */

#include "vice.h"

#include <stdlib.h>

#include "renderyuv.h"

/* Extract YUV components. */
inline static unsigned int Y(unsigned int YUV)
{
    return YUV >> 16;
}

inline static unsigned int U(unsigned int YUV)
{
    return (YUV >> 8) & 0xff;
}

inline static unsigned int V(unsigned int YUV)
{
    return YUV & 0xff;
}


typedef struct {
    unsigned int Y0, Y1, U, V;
} YUV_avg;

static YUV_avg yuv_lines[2][1024];


/* Render packed YUV 4:2:2 formats. */
void renderyuv_4_2_2(image_t* image,
                     int shift_y0, int shift_u, int shift_v, int shift_y1,
                     unsigned char* src,
                     int src_pitch,
                     unsigned int* src_color,
                     int src_x, int src_y,
                     unsigned int src_w, unsigned int src_h,
                     int dest_x, int dest_y)
{
    unsigned int x, y;
    unsigned int *dest = (unsigned int *)(image->data + image->offsets[0]);
    int dest_pitch = image->pitches[0]/4;

    /* Normalize to 2x1 blocks. */
    if (dest_x & 1) {
        dest_x--;
        src_x--;
        src_w++;
    }
    if (src_w & 1) {
        src_w++;
    }

    /* Add start offsets. */
    dest += dest_pitch * dest_y + (dest_x >> 1);
    src += src_pitch * src_y + src_x;

    /* Render 2x1 blocks, YUV 4:2:2 */
    for (y = 0; y < src_h; y++) {
        for (x = 0; x < src_w; x += 2) {
            unsigned int YUV0 = src_color[*src++];
            unsigned int YUV1 = src_color[*src++];
            *dest++ =
                (Y(YUV0) << shift_y0)
                | (((U(YUV0) + U(YUV1)) >> 1) << shift_u)
                | (((V(YUV0) + V(YUV1)) >> 1) << shift_v)
                | (Y(YUV1) << shift_y1);
        }
        src += src_pitch - src_w;
        dest += dest_pitch - (src_w >> 1);
    }
}


/* Render packed YUV 4:2:2 formats, double size. */
void renderyuv_2x_4_2_2(image_t* image,
                        int shift_y0, int shift_u, int shift_v, int shift_y1,
                        unsigned char* src,
                        int src_pitch,
                        unsigned int* src_color,
                        int src_x, int src_y,
                        unsigned int src_w, unsigned int src_h,
                        int dest_x, int dest_y,
                        int double_scan, int pal_scanline_shade)
{
    unsigned int x, y;
    unsigned int *dest = (unsigned int *)(image->data + image->offsets[0]);
    int dest_pitch = image->pitches[0]/4;

    /* No need to normalize to 2x1 blocks because of size doubling. */

    /* Add start offsets. */
    dest += (dest_pitch << 1)*dest_y + dest_x;
    src += src_pitch*src_y + src_x;

    /* Render 2x1 blocks, YUV 4:2:2 */
    for (y = 0; y < src_h; y++) {
        for (x = 0; x < src_w; x++) {
            unsigned int YUV = src_color[*src++];
            unsigned int Y0 = Y(YUV);
            unsigned int color =
                (U(YUV) << shift_u)
                | (V(YUV) << shift_v);
            unsigned int pixel2 =
                (Y0 << shift_y0)
                | color
                | (Y0 << shift_y1);
            *dest = pixel2;
            if (!double_scan) {
                /* Set scanline shade intensity. */
                Y0 = Y0*pal_scanline_shade >> 10;
                pixel2 =
                    (Y0 << shift_y0)
                    | color
                    | (Y0 << shift_y1);
            }
            *(dest + dest_pitch) = pixel2;
            dest++;
        }
        src += src_pitch - src_w;
        dest += (dest_pitch << 1) - src_w;
    }
}

/* Render planar YUV 4:1:1 formats. */
void renderyuv_4_1_1(image_t* image,
                     int plane_y, int plane_u, int plane_v,
                     unsigned char* src,
                     int src_pitch,
                     unsigned int* src_color,
                     int src_x, int src_y,
                     unsigned int src_w, unsigned int src_h,
                     int dest_x, int dest_y)
{
    unsigned int x, y;
    BYTE *Yptr = image->data + image->offsets[plane_y];
    BYTE *Uptr = image->data + image->offsets[plane_u];
    BYTE *Vptr = image->data + image->offsets[plane_v];
    int Ypitch = image->pitches[plane_y];
    int Upitch = image->pitches[plane_u];
    int Vpitch = image->pitches[plane_v];

    /* Normalize to 2x2 blocks. */
    if (dest_x & 1) {
        dest_x--;
        src_x--;
        src_w++;
    }
    if (src_w & 1) {
        src_w++;
    }
    if (dest_y & 1) {
        dest_y--;
        src_y--;
        src_h++;
    }
    if (src_h & 1) {
        src_h++;
    }

    /* Add start offsets. */
    Yptr += Ypitch*dest_y + dest_x;
    Uptr += (Upitch*dest_y + dest_x) >> 1;
    Vptr += (Vpitch*dest_y + dest_x) >> 1;
    src += src_pitch*src_y + src_x;

    /* Render 2x2 blocks, YUV 4:1:1 */
    for (y = 0; y < src_h; y += 2) {
        for (x = 0; x < src_w; x += 2) {
            unsigned int YUV0 = src_color[*src];
            unsigned int YUV1 = src_color[*(src + 1)];
            unsigned int YUV2 = src_color[*(src + src_pitch)];
            unsigned int YUV3 = src_color[*(src + src_pitch + 1)];
            src += 2;
            *Yptr = Y(YUV0);
            *(Yptr + 1) = Y(YUV1);
            *(Yptr + Ypitch) = Y(YUV2);
            *(Yptr + Ypitch + 1) = Y(YUV3);
            Yptr += 2;
            *Uptr++ = (U(YUV0) + U(YUV1) + U(YUV2) + U(YUV3)) >> 2;
            *Vptr++ = (V(YUV0) + V(YUV1) + V(YUV2) + V(YUV3)) >> 2;
        }
        src += (src_pitch << 1) - src_w;
        Yptr += (Ypitch << 1) - src_w;
        Uptr += Upitch - (src_w >> 1);
        Vptr += Vpitch - (src_w >> 1);
    }
}


/* Render planar YUV 4:1:1 formats, double size. */
void renderyuv_2x_4_1_1(image_t* image,
                        int plane_y, int plane_u, int plane_v,
                        unsigned char* src,
                        int src_pitch,
                        unsigned int* src_color,
                        int src_x, int src_y,
                        unsigned int src_w, unsigned int src_h,
                        int dest_x, int dest_y,
                        int double_scan, int pal_scanline_shade)
{
    unsigned int x, y;
    BYTE *Yptr = image->data + image->offsets[plane_y];
    BYTE *Uptr = image->data + image->offsets[plane_u];
    BYTE *Vptr = image->data + image->offsets[plane_v];
    int Ypitch = image->pitches[plane_y];
    int Upitch = image->pitches[plane_u];
    int Vpitch = image->pitches[plane_v];

    /* No need to normalize to 2x2 blocks because of size doubling. */

    /* Add start offsets. */
    Yptr += (Ypitch*dest_y + dest_x) << 1;
    Uptr += Upitch*dest_y + dest_x;
    Vptr += Vpitch*dest_y + dest_x;
    src += src_pitch*src_y + src_x;

    /* Render 2x2 blocks, YUV 4:1:1 */
    for (y = 0; y < src_h; y++) {
        for (x = 0; x < src_w; x++) {
            unsigned int YUV = src_color[*src++];
            unsigned int Y0 = Y(YUV);
            *Yptr = Y0;
            *(Yptr + 1) = Y0;
            if (!double_scan) {
                /* Set scanline shade intensity. */
                Y0 = Y0*pal_scanline_shade >> 10;
            }
            *(Yptr + Ypitch) = Y0;
            *(Yptr + Ypitch + 1) = Y0;
            Yptr += 2;
            *Uptr++ = U(YUV);
            *Vptr++ = V(YUV);
        }
        src += src_pitch - src_w;
        Yptr += (Ypitch - src_w) << 1;
        Uptr += Upitch - src_w;
        Vptr += Vpitch - src_w;
    }
}


/* Render planar YUV 4:1:1 formats - PAL emulation. */
void renderyuv_4_1_1_pal(image_t* image,
                         int plane_y, int plane_u, int plane_v,
                         unsigned char* src,
                         int src_pitch,
                         unsigned int* src_color,
                         int src_x, int src_y,
                         unsigned int src_w, unsigned int src_h,
                         int dest_x, int dest_y,
                         int pal_blur)
{
    unsigned int x, y;
    unsigned int
        YUVm10, YUV00, YUV10, YUV20,
        YUVm11, YUV01, YUV11, YUV21;
    BYTE *Yptr = image->data + image->offsets[plane_y];
    BYTE *Uptr = image->data + image->offsets[plane_u];
    BYTE *Vptr = image->data + image->offsets[plane_v];
    int Ypitch = image->pitches[plane_y];
    int Upitch = image->pitches[plane_u];
    int Vpitch = image->pitches[plane_v];
    int pal_sharp = 256 - (pal_blur << 1);

    /* Normalize to 2x2 blocks. */
    if (dest_x & 1) {
        dest_x--;
        src_x--;
        src_w++;
    }
    if (src_w & 1) {
        src_w++;
    }
    if (dest_y & 1) {
        dest_y--;
        src_y--;
        src_h++;
    }
    if (src_h & 1) {
        src_h++;
    }

    /* Enlarge the rendered area to ensure that neighboring pixels are
       correctly averaged. */
    if (dest_x > 0) {
        dest_x -= 2;
        src_x -= 2;
        src_w += 2;
    }
    if (dest_x + (int)src_w < (int)(image->width)) {
        src_w += 2;
    }

    /* Add start offsets. */
    Yptr += Ypitch*dest_y + dest_x;
    Uptr += (Upitch*dest_y + dest_x) >> 1;
    Vptr += (Vpitch*dest_y + dest_x) >> 1;
    src += src_pitch*src_y + src_x;

    /* Render 2x2 blocks, YUV 4:1:1 */
    for (y = 0; y < src_h; y += 2) {
        /* Read first 2x2 block. */
        if (dest_x > 0) {
            YUVm10 = src_color[*(src - 1)];
            YUVm11 = src_color[*(src + src_pitch - 1)];
        } else {
            YUVm10 = src_color[*src];
            YUVm11 = src_color[*(src + src_pitch)];
        }
        YUV00 =  src_color[*src];
        YUV01 =  src_color[*(src + src_pitch)];
        for (x = 0; x < src_w - 2; x += 2) {
            /* Read next 2x2 block. */
            YUV10 = src_color[*(src + 1)];
            YUV20 = src_color[*(src + 2)];
            YUV11 = src_color[*(src + src_pitch + 1)];
            YUV21 = src_color[*(src + src_pitch + 2)];
            src += 2;
            *Yptr =               (Y(YUV00)*pal_sharp + (Y(YUVm10) + Y(YUV10))*pal_blur) >> 8;
            *(Yptr + 1) =          (Y(YUV10)*pal_sharp + (Y(YUV00) + Y(YUV20))*pal_blur) >> 8;
            *(Yptr + Ypitch) =    (Y(YUV01)*pal_sharp + (Y(YUVm11) + Y(YUV11))*pal_blur) >> 8;
            *(Yptr + Ypitch + 1) = (Y(YUV11)*pal_sharp + (Y(YUV01) + Y(YUV21))*pal_blur) >> 8;
            Yptr += 2;
            *Uptr++ =
                (U(YUVm10) + U(YUV00) + U(YUV10) + U(YUV20) +
                 U(YUVm11) + U(YUV01) + U(YUV11) + U(YUV21)) >> 3;
            *Vptr++ =
                (V(YUVm10) + V(YUV00) + V(YUV10) + V(YUV20) +
                 V(YUVm11) + V(YUV01) + V(YUV11) + V(YUV21)) >> 3;

            /* Prepare to read next 2x2 block. */
            YUVm10 = YUV10; YUV00 = YUV20;
            YUVm11 = YUV11; YUV01 = YUV21;
        }
        /* Read last 2x2 block. */
        YUV10 = src_color[*(src + 1)];
        YUV11 = src_color[*(src + src_pitch + 1)];
        if (dest_x + (int)src_w < (int)(image->width)) {
            YUV20 = YUV10;
            YUV21 = YUV11;
        } else {
            YUV20 = src_color[*(src + 2)];
            YUV21 = src_color[*(src + src_pitch + 2)];
        }
        src += 2;
        *Yptr =               (Y(YUV00)*pal_sharp + (Y(YUVm10) + Y(YUV10))*pal_blur) >> 8;
        *(Yptr + 1) =          (Y(YUV10)*pal_sharp + (Y(YUV00) + Y(YUV20))*pal_blur) >> 8;
        *(Yptr + Ypitch) =    (Y(YUV01)*pal_sharp + (Y(YUVm11) + Y(YUV11))*pal_blur) >> 8;
        *(Yptr + Ypitch + 1) = (Y(YUV11)*pal_sharp + (Y(YUV01) + Y(YUV21))*pal_blur) >> 8;
        Yptr += 2;
        *Uptr++ =
            (U(YUVm10) + U(YUV00) + U(YUV10) + U(YUV20) +
             U(YUVm11) + U(YUV01) + U(YUV11) + U(YUV21)) >> 3;
        *Vptr++ =
            (V(YUVm10) + V(YUV00) + V(YUV10) + V(YUV20) +
             V(YUVm11) + V(YUV01) + V(YUV11) + V(YUV21)) >> 3;

        src += (src_pitch << 1) - src_w;
        Yptr += (Ypitch << 1) - src_w;
        Uptr += Upitch - (src_w >> 1);
        Vptr += Vpitch - (src_w >> 1);
    }
}


/* Render planar YUV 4:1:1 formats - PAL emulation, double size. */
void renderyuv_2x_4_1_1_pal(image_t* image,
                            int plane_y, int plane_u, int plane_v,
                            unsigned char* src,
                            int src_pitch,
                            unsigned int* src_color,
                            int src_x, int src_y,
                            unsigned int src_w, unsigned int src_h,
                            int dest_x, int dest_y,
                            int pal_blur,
                            int double_scan, int pal_scanline_shade)
{
    unsigned int x, y;
    unsigned int YUVm2, YUVm1, YUV0, YUV1;
    int lineno = 0;
    YUV_avg* linepre;
    YUV_avg* line;
    BYTE *Yptr = image->data + image->offsets[plane_y];
    BYTE *Uptr = image->data + image->offsets[plane_u];
    BYTE *Vptr = image->data + image->offsets[plane_v];
    int Ypitch = image->pitches[plane_y];
    int Upitch = image->pitches[plane_u];
    int Vpitch = image->pitches[plane_v];
    int pal_sharp = 256 - (pal_blur << 1);

    /* No need to normalize to 2x2 blocks because of size doubling. */

    /* Enlarge the rendered area to ensure that neighboring pixels are
       correctly averaged. */
    dest_x--;
    src_x--;
    src_w++;
    if (dest_x < 0) {
        src_x -= dest_x;
        src_w += dest_x;
        dest_x = 0;
    }
    src_w += 2;
    if (dest_x + (int)src_w > (int)(image->width >> 1)) {
        src_w = (image->width >> 1) - dest_x;
    }
    if (dest_y + (int)src_h < (int)(image->height >> 1)) {
        src_h++;
    }

    /* Add start offsets. */
    Yptr += (Ypitch*dest_y + dest_x) << 1;
    Uptr += Upitch*dest_y + dest_x;
    Vptr += Vpitch*dest_y + dest_x;
    src += src_pitch*src_y + src_x;

    if (dest_y > 0) {
        /* Store previous line. */
        linepre = yuv_lines[lineno];
        line = linepre;
        src -= src_pitch;

        /* Read first three pixels. */
        YUVm2 = YUVm1 = YUV0 = src_color[*src];
        if (dest_x > 0) {
            YUVm1 = src_color[*(src - 1)];
            if (dest_x > 1) {
                YUVm2 = src_color[*(src - 2)];
            }
        }
        for (x = 0; x < src_w - 1; x++) {
            /* Read next pixel. */
            YUV1 = src_color[*++src];

            line->U = U(YUVm2) + U(YUVm1) + U(YUV0) + U(YUV1);
            line->V = V(YUVm2) + V(YUVm1) + V(YUV0) + V(YUV1);

            /* Prepare to read next pixel. */
            line++;
            YUVm2 = YUVm1;
            YUVm1 = YUV0;
            YUV0 = YUV1;
        }
        /* Read last pixel. */
        if (dest_x + (int)src_w < (int)(image->width >> 1)) {
            YUV1 = src_color[*++src];
        } else {
            YUV1 = src_color[*src++];
        }
        line->U = U(YUVm2) + U(YUVm1) + U(YUV0) + U(YUV1);
        line->V = V(YUVm2) + V(YUVm1) + V(YUV0) + V(YUV1);

        src += src_pitch - src_w;
    } else {
        /* Trick main loop into averaging first line with itself. */
        linepre = yuv_lines[lineno ^ 1];
    }

    for (y = 0; y < src_h; y++) {
        /* Store current line. */
        lineno ^= 1;
        line = yuv_lines[lineno];

        /* Read first three pixels. */
        YUVm2 = YUVm1 = YUV0 = src_color[*src];
        if (dest_x > 0) {
            YUVm1 = src_color[*(src - 1)];
            if (dest_x > 1) {
                YUVm2 = src_color[*(src - 2)];
            }
        }
        for (x = 0; x < src_w - 1; x++) {
            /* Read next pixel. */
            YUV1 = src_color[*++src];
            line->Y0 = (Y(YUV0)*pal_sharp + (Y(YUVm1) + Y(YUV1))*pal_blur) >> 8;
            line->U = U(YUVm2) + U(YUVm1) + U(YUV0) + U(YUV1);
            line->V = V(YUVm2) + V(YUVm1) + V(YUV0) + V(YUV1);

            /* Prepare to read next pixel. */
            line++;
            YUVm2 = YUVm1;
            YUVm1 = YUV0;
            YUV0 = YUV1;
        }
        /* Read last pixel. */
        if (dest_x + (int)src_w < (int)(image->width >> 1)) {
            YUV1 = src_color[*++src];
        } else {
            YUV1 = src_color[*src++];
        }
        line->Y0 = (Y(YUV0)*pal_sharp + (Y(YUVm1) + Y(YUV1))*pal_blur) >> 8;
        line->U = U(YUVm2) + U(YUVm1) + U(YUV0) + U(YUV1);
        line->V = V(YUVm2) + V(YUVm1) + V(YUV0) + V(YUV1);
        src += src_pitch - src_w;

        line = yuv_lines[lineno];

        /* Render 2x1 blocks, YUV 4:1:1 */
        for (x = 0; x < src_w; x++) {
            unsigned int Y0 = line->Y0;
            *Yptr = Y0;
            *(Yptr + 1) = Y0;
            if (!double_scan) {
                /* Set scanline shade intensity. */
                Y0 = line->Y0*pal_scanline_shade >> 10;
            }
            *(Yptr + Ypitch) = Y0;
            *(Yptr + Ypitch + 1) = Y0;
            Yptr += 2;
            *Uptr++ =
                (line->U + linepre->U) >> 3;
            *Vptr++ =
                (line->V + linepre->V) >> 3;
            line++;
            linepre++;
        }
        Yptr += (Ypitch - src_w) << 1;
        Uptr += Upitch - src_w;
        Vptr += Vpitch - src_w;

        linepre = yuv_lines[lineno];
    }
}
