/*
 * ffmpeglib.c - Interface to access the ffmpeg libs.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifdef HAVE_FFMPEG 

#include "gfxoutputdrv/ffmpeglib.h"
#include "log.h"
#include "translate.h"
#include "uiapi.h"
#include "dynlib.h"

#ifdef WIN32
#define MAKE_SO_NAME(name,version)  #name "-" #version ".dll"
#else
#ifdef MACOSX_SUPPORT
#define MAKE_SO_NAME(name,version)  "/opt/local/lib/lib" #name "." #version ".dylib"
#else
#define MAKE_SO_NAME(name,version)  "lib" #name ".so." #version
#endif
#endif

// add second level macro to allow expansion and stringification
#define MAKE_SO_NAME2(n,v) MAKE_SO_NAME(n,v)

// define major version if its not already defined
#ifndef LIBAVCODEC_VERSION_MAJOR
#define LIBAVCODEC_VERSION_MAJOR  51
#endif
#ifndef LIBAVFORMAT_VERSION_MAJOR
#define LIBAVFORMAT_VERSION_MAJOR 52
#define NO_AVFORMAT_CHECK 1
#endif
#ifndef LIBAVUTIL_VERSION_MAJOR
#define LIBAVUTIL_VERSION_MAJOR   49
#define NO_AVUTIL_CHECK 1
#endif
#ifndef LIBSWSCALE_VERSION_MAJOR
#define LIBSWSCALE_VERSION_MAJOR  0
#endif

#define AVCODEC_SO_NAME     MAKE_SO_NAME2(avcodec,LIBAVCODEC_VERSION_MAJOR)
#define AVFORMAT_SO_NAME    MAKE_SO_NAME2(avformat,LIBAVFORMAT_VERSION_MAJOR)
#define AVUTIL_SO_NAME      MAKE_SO_NAME2(avutil,LIBAVUTIL_VERSION_MAJOR)
#define SWSCALE_SO_NAME     MAKE_SO_NAME2(swscale,LIBSWSCALE_VERSION_MAJOR)

static void *avcodec_so = NULL;
static void *avformat_so = NULL;
static void *avutil_so = NULL;
#ifdef HAVE_FFMPEG_SWSCALE
static void *swscale_so = NULL;
#endif

/* macro for getting functionpointers from avcodec */
#define GET_SYMBOL_AND_TEST_AVCODEC( _name_ ) \
    lib->p_##_name_ = (_name_##_t) vice_dynlib_symbol(avcodec_so, #_name_ ); \
    if (!lib->p_##_name_ ) { \
        log_debug("getting symbol " #_name_ " failed!"); \
        return -1; \
    } 

/* macro for getting functionpointers from avformat */
#define GET_SYMBOL_AND_TEST_AVFORMAT( _name_ ) \
    lib->p_##_name_ = (_name_##_t) vice_dynlib_symbol(avformat_so, #_name_ ); \
    if (!lib->p_##_name_ ) { \
        log_debug("getting symbol " #_name_ " failed!"); \
        return -1; \
    } 

/* macro for getting functionpointers from avutil */
#define GET_SYMBOL_AND_TEST_AVUTIL( _name_ ) \
    lib->p_##_name_ = (_name_##_t) vice_dynlib_symbol(avutil_so, #_name_ ); \
    if (!lib->p_##_name_ ) { \
        log_debug("getting symbol " #_name_ " failed!"); \
        return -1; \
    } 

/* macro for getting functionpointers from swscale */
#define GET_SYMBOL_AND_TEST_SWSCALE( _name_ ) \
    lib->p_##_name_ = (_name_##_t) vice_dynlib_symbol(swscale_so, #_name_ ); \
    if (!lib->p_##_name_ ) { \
        log_debug("getting symbol " #_name_ " failed!"); \
        return -1; \
    } 

static int check_version(const char *lib_name, void *handle, const char *symbol,
                         unsigned ver_inc)
{
    ffmpeg_version_t  version_func;
    unsigned ver_lib;
    const char *result_msgs[] = { "full match","major.minor matches","major matches","unsupported" };
    enum { FULL_MATCH=0, MAJOR_MINOR_MATCH=1, MAJOR_MATCH=2, NO_MATCH=3 } result;
    
    version_func = (ffmpeg_version_t)vice_dynlib_symbol(handle, symbol);
    if (version_func == NULL) {
        log_debug("ffmpeg %s: version function '%s' not found!",lib_name, symbol);
        return -1;
    }

    ver_lib = version_func();
    
    /* version matches exactly */
    if (ver_lib == ver_inc) {
        result = FULL_MATCH;
    } else {
        /* compare major.minor */
        ver_lib >>= 8;
        ver_inc >>= 8;
        if (ver_lib == ver_inc) {
            result = MAJOR_MINOR_MATCH;
        } else {
            /* compare major */
            ver_lib >>= 8;
            ver_inc >>= 8;
            if (ver_lib == ver_inc) {
                result = MAJOR_MATCH;
            } else {
                result = NO_MATCH;
            }
        }
    }
    
    log_debug("ffmpeg %8s lib has version %06x, VICE expects %06x: %s",
              lib_name, ver_lib, ver_inc, result_msgs[result]);

    /* now decide what level of matching fails */
    if (result == NO_MATCH) {
        return -1;
    }
    return 0;
}

static int load_avcodec(ffmpeglib_t *lib)
{
    if (!avcodec_so) {
        avcodec_so = vice_dynlib_open(AVCODEC_SO_NAME);

        if (!avcodec_so) {
            log_debug("opening dynamic library " AVCODEC_SO_NAME " failed!");
            return -1;
        }

        GET_SYMBOL_AND_TEST_AVCODEC(avcodec_open);
        GET_SYMBOL_AND_TEST_AVCODEC(avcodec_close);
        GET_SYMBOL_AND_TEST_AVCODEC(avcodec_find_encoder);
        GET_SYMBOL_AND_TEST_AVCODEC(avcodec_encode_audio);
        GET_SYMBOL_AND_TEST_AVCODEC(avcodec_encode_video);
        GET_SYMBOL_AND_TEST_AVCODEC(avpicture_fill);
        GET_SYMBOL_AND_TEST_AVCODEC(avpicture_get_size);
    }

    return check_version("avcodec",avcodec_so,"avcodec_version",LIBAVCODEC_VERSION_INT);
}

static void free_avcodec(ffmpeglib_t *lib)
{
    if (avcodec_so) {
        if (vice_dynlib_close(avcodec_so) != 0) {
            log_debug("closing dynamic library " AVCODEC_SO_NAME " failed!");
        }
    }
    avcodec_so = NULL;

    lib->p_avcodec_open = NULL;
    lib->p_avcodec_close = NULL;
    lib->p_avcodec_find_encoder = NULL;
    lib->p_avcodec_encode_audio = NULL;
    lib->p_avcodec_encode_video = NULL;
    lib->p_avpicture_fill = NULL;
    lib->p_avpicture_get_size = NULL;
}

static int load_avformat(ffmpeglib_t *lib)
{
    if (!avformat_so) {
        avformat_so = vice_dynlib_open(AVFORMAT_SO_NAME);

        if (!avformat_so) {
            log_debug("opening dynamic library " AVFORMAT_SO_NAME " failed!");
            return -1;
        }

        GET_SYMBOL_AND_TEST_AVFORMAT(av_init_packet);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_register_all);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_new_stream);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_set_parameters);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_write_header);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_write_frame);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_write_trailer);
        GET_SYMBOL_AND_TEST_AVFORMAT(url_fopen);
        GET_SYMBOL_AND_TEST_AVFORMAT(url_fclose);
        GET_SYMBOL_AND_TEST_AVFORMAT(dump_format);
        GET_SYMBOL_AND_TEST_AVFORMAT(av_guess_format);
#ifndef HAVE_FFMPEG_SWSCALE
        GET_SYMBOL_AND_TEST_AVFORMAT(img_convert);
#endif
    }

#ifdef NO_AVFORMAT_CHECK
    return 0;
#else
    return check_version("avformat",avformat_so,"avformat_version",LIBAVFORMAT_VERSION_INT);
#endif
}

static void free_avformat(ffmpeglib_t *lib)
{
    if (avformat_so) {
        if (vice_dynlib_close(avformat_so) != 0) {
            log_debug("closing dynamic library " AVFORMAT_SO_NAME " failed!");
        }
    }
    avformat_so = NULL;

    lib->p_av_init_packet = NULL;
    lib->p_av_register_all = NULL;
    lib->p_av_new_stream = NULL;
    lib->p_av_set_parameters = NULL;
    lib->p_av_write_header = NULL;
    lib->p_av_write_frame = NULL;
    lib->p_av_write_trailer = NULL;
    lib->p_url_fopen = NULL;
    lib->p_url_fclose = NULL;
    lib->p_dump_format = NULL;
    lib->p_av_guess_format = NULL;
#ifndef HAVE_FFMPEG_SWSCALE
    lib->p_img_convert = NULL;
#endif
}

static int load_avutil(ffmpeglib_t *lib)
{
    if (!avutil_so) {
        avutil_so = vice_dynlib_open(AVUTIL_SO_NAME);

        if (!avutil_so) {
            log_debug("opening dynamic library " AVUTIL_SO_NAME " failed!");
            return -1;
        }

        GET_SYMBOL_AND_TEST_AVUTIL(av_free);
    }

#ifdef NO_AVUTIL_CHECK
    return 0;
#else
    return check_version("avutil",avutil_so,"avutil_version",LIBAVUTIL_VERSION_INT);
#endif
}

static void free_avutil(ffmpeglib_t *lib)
{
    if (avutil_so) {
        if (vice_dynlib_close(avutil_so) != 0) {
            log_debug("closing dynamic library " AVUTIL_SO_NAME " failed!");
        }
    }
    avutil_so = NULL;

    lib->p_av_free = NULL;    
}

#ifdef HAVE_FFMPEG_SWSCALE

static int load_swscale(ffmpeglib_t *lib)
{    
    if (!swscale_so) {
        swscale_so = vice_dynlib_open(SWSCALE_SO_NAME);
        
        if (!swscale_so) {
            log_debug("opening dynamic library " SWSCALE_SO_NAME " failed!");
            return -1;
        }
        
        GET_SYMBOL_AND_TEST_SWSCALE(sws_getContext);
        GET_SYMBOL_AND_TEST_SWSCALE(sws_freeContext);
        GET_SYMBOL_AND_TEST_SWSCALE(sws_scale);
    }
    
    return check_version("swscale",swscale_so,"swscale_version",LIBSWSCALE_VERSION_INT);
}

static void free_swscale(ffmpeglib_t *lib)
{
    if (swscale_so) {
        if (vice_dynlib_close(swscale_so) != 0) {
            log_debug("closing dynamic library " SWSCALE_SO_NAME " failed!");
        }
    }
    swscale_so = NULL;
    
    lib->p_sws_getContext = NULL;
    lib->p_sws_freeContext = NULL;
    lib->p_sws_scale = NULL;
}

#endif

int ffmpeglib_open(ffmpeglib_t *lib)
{
    int result;
    
    result = load_avformat(lib);
    if (result != 0) {
        free_avformat(lib);
        return result;
    }

    result = load_avcodec(lib);
    if (result != 0) {
        free_avformat(lib);
        free_avcodec(lib);
        return result;
    }

    result = load_avutil(lib);
    if (result != 0) {
        free_avformat(lib);
        free_avcodec(lib);
        free_avutil(lib);
        return result;
    }

#ifdef HAVE_FFMPEG_SWSCALE
    result = load_swscale(lib);
    if (result != 0) {
        free_avformat(lib);
        free_avcodec(lib);
        free_avutil(lib);
        free_swscale(lib);
        return result;
    }
#endif

    return 0;
}

void ffmpeglib_close(ffmpeglib_t *lib)
{
    free_avformat(lib);
    free_avcodec(lib);
    free_avutil(lib);
#ifdef HAVE_FFMPEG_SWSCALE
    free_swscale(lib);
#endif
}

#endif /* #ifdef HAVE_FFMPEG */
