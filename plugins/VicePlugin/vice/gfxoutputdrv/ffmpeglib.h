/*
 * ffmpeglib.h - Interface to access the ffmpeg libs.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifndef VICE_FFMPEGLIB_H
#define VICE_FFMPEGLIB_H

#include "vice.h"

#ifdef HAVE_FFMPEG_HEADER_SUBDIRS
#include "libavformat/avformat.h"
#ifdef HAVE_FFMPEG_SWSCALE
#include "libswscale/swscale.h"
#endif
#else
#include "avformat.h"
#ifdef HAVE_FFMPEG_SWSCALE
#include "swscale.h"
#endif
#endif

/* backwards compatibility */
#if LIBAVUTIL_VERSION_MAJOR < 51
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
/* the win32 check is needed otherwise *nix builds fail */
#if defined(WIN32) || defined(WATCOM_COMPILE)
#define av_guess_format guess_format
#define av_guess_format_t guess_format_t
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif
#endif

/* wether this check makes sense for V51 is unclear */
#if LIBAVFORMAT_VERSION_MAJOR < 51
#define av_guess_format guess_format
#define av_guess_format_t guess_format_t
#endif

/* wether this check makes sense for V51 is unclear */
#if LIBAVCODEC_VERSION_MAJOR < 51
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

/* generic version function */
typedef unsigned (*ffmpeg_version_t) (void);

/* avcodec fucntions */
typedef int (*avcodec_open_t) (AVCodecContext*, AVCodec*);
typedef int (*avcodec_close_t) (AVCodecContext*);
typedef AVCodec* (*avcodec_find_encoder_t) (enum CodecID);
typedef int (*avcodec_encode_audio_t) (AVCodecContext*, uint8_t*, int, const short*);
typedef int (*avcodec_encode_video_t) (AVCodecContext*, uint8_t*, int, const AVFrame*);
typedef int (*avpicture_fill_t) (AVPicture*, uint8_t*, int, int, int);
typedef int (*avpicture_get_size_t) (int, int, int);

/* avformat functions */
typedef void (*av_init_packet_t) (AVPacket *pkt);
typedef void (*av_register_all_t) (void);
typedef AVStream* (*av_new_stream_t) (AVFormatContext*, int);
typedef int (*av_set_parameters_t) (AVFormatContext*, AVFormatParameters*);
typedef int (*av_write_header_t) (AVFormatContext*);
typedef int (*av_write_frame_t) (AVFormatContext*, AVPacket*);
typedef int (*av_write_trailer_t) (AVFormatContext*);
typedef int (*url_fopen_t) (ByteIOContext**, const char*, int);
typedef int (*url_fclose_t) (ByteIOContext*);
typedef void (*dump_format_t) (AVFormatContext *, int, const char*, int);
typedef AVOutputFormat* (*av_guess_format_t) (const char*, const char*, const char*);
typedef int (*img_convert_t) (AVPicture*, int, AVPicture*, int, int, int);

/* avutil functions */
typedef void (*av_free_t) (void**);

#ifdef HAVE_FFMPEG_SWSCALE
/* swscale functions */
typedef struct SwsContext * (*sws_getContext_t)(int srcW, int srcH,
  enum PixelFormat srcFormat, int dstW, int dstH, enum PixelFormat dstFormat,
  int flags, SwsFilter *srcFilter, SwsFilter *dstFilter, double *param);
typedef void (*sws_freeContext_t)(struct SwsContext *swsContext);
typedef int (*sws_scale_t)(struct SwsContext *context, uint8_t* srcSlice[],
  int srcStride[], int srcSliceY, int srcSliceH, uint8_t* dst[],
  int dstStride[]);
#endif

struct ffmpeglib_s {
    /* avcodec */
    avcodec_open_t              p_avcodec_open;
    avcodec_close_t             p_avcodec_close;
    avcodec_find_encoder_t      p_avcodec_find_encoder;
    avcodec_encode_audio_t      p_avcodec_encode_audio;
    avcodec_encode_video_t      p_avcodec_encode_video;
    avpicture_fill_t            p_avpicture_fill;
    avpicture_get_size_t        p_avpicture_get_size;

    /* avformat */
    av_init_packet_t            p_av_init_packet;
    av_register_all_t           p_av_register_all;
    av_new_stream_t             p_av_new_stream;
    av_set_parameters_t         p_av_set_parameters;
    av_write_header_t           p_av_write_header;
    av_write_frame_t            p_av_write_frame;
    av_write_trailer_t          p_av_write_trailer;
    url_fopen_t                 p_url_fopen;
    url_fclose_t                p_url_fclose;
    dump_format_t               p_dump_format;
    av_guess_format_t           p_av_guess_format;
#ifndef HAVE_FFMPEG_SWSCALE
    img_convert_t               p_img_convert;
#endif

    /* avutil */
    av_free_t                   p_av_free;

#ifdef HAVE_FFMPEG_SWSCALE    
    /* swscale */
    sws_getContext_t            p_sws_getContext;
    sws_freeContext_t           p_sws_freeContext;
    sws_scale_t                 p_sws_scale;
#endif
};

typedef struct ffmpeglib_s ffmpeglib_t;

extern int ffmpeglib_open(ffmpeglib_t *lib);
extern void ffmpeglib_close(ffmpeglib_t *lib);

#endif
