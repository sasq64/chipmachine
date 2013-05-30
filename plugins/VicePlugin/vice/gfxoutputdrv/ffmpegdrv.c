/*
 * ffmpegdrv.c - Movie driver using FFMPEG library and screenshot API.
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "ffmpegdrv.h"
#include "ffmpeglib.h"
#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "translate.h"
#include "uiapi.h"
#include "util.h"
#include "vsync.h"
#include "../sounddrv/soundmovie.h"

static gfxoutputdrv_codec_t avi_audio_codeclist[] = { 
    { CODEC_ID_MP2, "MP2" },
    { CODEC_ID_MP3, "MP3" },
    { CODEC_ID_FLAC, "FLAC" },
    { CODEC_ID_PCM_S16LE, "PCM uncompressed" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t avi_video_codeclist[] = { 
    { CODEC_ID_MPEG4, "MPEG4 (DivX)" },
    { CODEC_ID_MPEG1VIDEO, "MPEG1" },
    { CODEC_ID_FFV1, "FFV1 (lossless)" },
    { CODEC_ID_H264, "H264" },
    { CODEC_ID_THEORA, "Theora" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t ogg_audio_codeclist[] = { 
    { CODEC_ID_FLAC, "FLAC" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t ogg_video_codeclist[] = { 
    { CODEC_ID_THEORA, "Theora" },
    { 0, NULL }
};

gfxoutputdrv_format_t ffmpegdrv_formatlist[] =
{
    { "avi", avi_audio_codeclist, avi_video_codeclist },
    { "ogg", ogg_audio_codeclist, ogg_video_codeclist },
    { "wav", NULL, NULL },
    { "mp3", NULL, NULL },
    { "mp2", NULL, NULL },
    { NULL, NULL, NULL }
};

/* general */
static ffmpeglib_t ffmpeglib;
static AVFormatContext *ffmpegdrv_oc;
static AVOutputFormat *ffmpegdrv_fmt;
static int file_init_done;

/* audio */
static AVStream *audio_st;
static soundmovie_buffer_t ffmpegdrv_audio_in;
static int audio_init_done;
static int audio_is_open;
static unsigned char *audio_outbuf;
static int audio_outbuf_size;
static double audio_pts;

/* video */
static AVStream *video_st;
static int video_init_done;
static int video_is_open;
static AVFrame *picture, *tmp_picture;
static unsigned char *video_outbuf;
static int video_outbuf_size;
static int video_width, video_height;
static AVFrame *picture, *tmp_picture;
static double video_pts;
static unsigned int framecounter;
#ifdef HAVE_FFMPEG_SWSCALE
static struct SwsContext *sws_ctx;
#endif

/* resources */
static char *ffmpeg_format = NULL;
static int format_index;
static int audio_bitrate;
static int video_bitrate;
static int audio_codec;
static int video_codec;
static int video_halve_framerate;

static int ffmpegdrv_init_file(void);

static int set_format(const char *val, void *param)
{
    int i;

    format_index = -1;
    util_string_set(&ffmpeg_format, val);
    for (i = 0; ffmpegdrv_formatlist[i].name != NULL; i++) {
        if (strcmp(ffmpeg_format, ffmpegdrv_formatlist[i].name) == 0) {
            format_index = i;
        }
    }

    if (format_index < 0) {
        return -1;
    } else {
        return 0;
    }
}

static int set_audio_bitrate(int val, void *param)
{
    audio_bitrate = (CLOCK)val;

    if ((audio_bitrate < VICE_FFMPEG_AUDIO_RATE_MIN)
     || (audio_bitrate > VICE_FFMPEG_AUDIO_RATE_MAX)) {
        audio_bitrate = VICE_FFMPEG_AUDIO_RATE_DEFAULT;
    }
    return 0;
}

static int set_video_bitrate(int val, void *param)
{
    video_bitrate = (CLOCK)val;

    if ((video_bitrate < VICE_FFMPEG_VIDEO_RATE_MIN)
     || (video_bitrate > VICE_FFMPEG_VIDEO_RATE_MAX)) {
        video_bitrate = VICE_FFMPEG_VIDEO_RATE_DEFAULT;
    }
    return 0;
}

static int set_audio_codec(int val, void *param)
{
    audio_codec = val;
    return 0;
}

static int set_video_codec(int val, void *param)
{
    video_codec = val;
    return 0;
}

static int set_video_halve_framerate(int val, void *param)
{
    if (video_halve_framerate != val && screenshot_is_recording()) {
        ui_error("Can't change framerate while recording. Try again later.");
        return 0;
    }
    video_halve_framerate = val;
    return 0;
}

/*---------- Resources ------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "FFMPEGFormat", "avi", RES_EVENT_NO, NULL,
      &ffmpeg_format, set_format, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "FFMPEGAudioBitrate", VICE_FFMPEG_AUDIO_RATE_DEFAULT,
      RES_EVENT_NO, NULL,
      &audio_bitrate, set_audio_bitrate, NULL },
    { "FFMPEGVideoBitrate", VICE_FFMPEG_VIDEO_RATE_DEFAULT,
      RES_EVENT_NO, NULL,
      &video_bitrate, set_video_bitrate, NULL },
    { "FFMPEGAudioCodec", CODEC_ID_MP3, RES_EVENT_NO, NULL,
      &audio_codec, set_audio_codec, NULL },
    { "FFMPEGVideoCodec", CODEC_ID_MPEG4, RES_EVENT_NO, NULL,
      &video_codec, set_video_codec, NULL },
    { "FFMPEGVideoHalveFramerate", 0, RES_EVENT_NO, NULL,
      &video_halve_framerate, set_video_halve_framerate, NULL },
    { NULL }
};

static int ffmpegdrv_resources_init(void)
{
    if (resources_register_string(resources_string) < 0)
        return -1;

    return resources_register_int(resources_int);
}
/*---------------------------------------------------------------------*/


/*---------- Commandline options --------------------------------------*/

static const cmdline_option_t cmdline_options[] = {
    { "-ffmpegaudiobitrate", SET_RESOURCE, 1,
      NULL, NULL, "FFMPEGAudioBitrate", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_AUDIO_STREAM_BITRATE,
      NULL, NULL },
    { "-ffmpegvideobitrate", SET_RESOURCE, 1,
      NULL, NULL, "FFMPEGVideoBitrate", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_VIDEO_STREAM_BITRATE,
      NULL, NULL },
    { NULL }
};

static int ffmpegdrv_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/*---------------------------------------------------------------------*/



/*-----------------------*/
/* audio stream encoding */
/*-----------------------*/
static int ffmpegdrv_open_audio(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVCodec *codec;
    int audio_inbuf_samples;

    c = st->codec;

    /* find the audio encoder */
    codec = (*ffmpeglib.p_avcodec_find_encoder)(c->codec_id);
    if (!codec) {
        log_debug("ffmpegdrv: audio codec not found");
        return -1;
    }

    /* open it */
    if ((*ffmpeglib.p_avcodec_open)(c, codec) < 0) {
        log_debug("ffmpegdrv: could not open audio codec");
        return -1;
    }
    
    audio_is_open = 1;
    audio_outbuf_size = 100000;
    audio_outbuf = lib_malloc(audio_outbuf_size);

    /* ugly hack for PCM codecs (will be removed ASAP with new PCM
       support to compute the input frame size in samples */
    if (c->frame_size <= 1) {
        audio_inbuf_samples = audio_outbuf_size;
        switch(st->codec->codec_id) {
        case CODEC_ID_PCM_S16LE:
        case CODEC_ID_PCM_S16BE:
        case CODEC_ID_PCM_U16LE:
        case CODEC_ID_PCM_U16BE:
            audio_inbuf_samples >>= 1;
            break;
        default:
            break;
        }
    } else {
        audio_inbuf_samples = c->frame_size * c->channels;
    }
    ffmpegdrv_audio_in.size = audio_inbuf_samples;
    ffmpegdrv_audio_in.buffer = lib_malloc(audio_inbuf_samples
                                                    * sizeof(SWORD));

    return 0;
}


static void ffmpegdrv_close_audio(void)
{
    if (audio_st == NULL)
        return;

    if (audio_is_open)
        (*ffmpeglib.p_avcodec_close)(audio_st->codec);

    audio_is_open = 0;
    lib_free(ffmpegdrv_audio_in.buffer);
    ffmpegdrv_audio_in.buffer = NULL;
    ffmpegdrv_audio_in.size = 0;
    lib_free(audio_outbuf);
    audio_outbuf = NULL;
}


static int ffmpegmovie_init_audio(int speed, int channels,
                                 soundmovie_buffer_t ** audio_in)
{
    AVCodecContext *c;
    AVStream *st;

    if (ffmpegdrv_oc == NULL || ffmpegdrv_fmt == NULL)
        return -1;

    audio_init_done = 1;

    if (ffmpegdrv_fmt->audio_codec == CODEC_ID_NONE)
        return -1;

    *audio_in = &ffmpegdrv_audio_in;
    
    (*audio_in)->size = 0; /* not allocated yet */
    (*audio_in)->used = 0;

    st = (*ffmpeglib.p_av_new_stream)(ffmpegdrv_oc, 1);
    if (!st) {
        log_debug("ffmpegdrv: Could not alloc audio stream\n");
        return -1;
    }

    c = st->codec;
    c->codec_id = ffmpegdrv_fmt->audio_codec;
    c->codec_type = AVMEDIA_TYPE_AUDIO;
    c->sample_fmt = SAMPLE_FMT_S16;

    /* put sample parameters */
    c->bit_rate = audio_bitrate;
    c->sample_rate = speed;
    c->channels = channels;
    audio_st = st;
    audio_pts = 0;

    if (video_init_done)
        ffmpegdrv_init_file();

    return 0;
}


/* triggered by soundffmpegaudio->write */
static int ffmpegmovie_encode_audio(soundmovie_buffer_t *audio_in)
{
    if (audio_st) {
        AVPacket pkt;
        AVCodecContext *c;
        (*ffmpeglib.p_av_init_packet)(&pkt);
        c = audio_st->codec;
        pkt.size = (*ffmpeglib.p_avcodec_encode_audio)(c, 
                        audio_outbuf, audio_outbuf_size, audio_in->buffer);
        pkt.pts = c->coded_frame->pts;
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = audio_st->index;
        pkt.data = audio_outbuf;

        if ((*ffmpeglib.p_av_write_frame)(ffmpegdrv_oc, &pkt) != 0)
            log_debug("ffmpegdrv_encode_audio: Error while writing audio frame");

        audio_pts = (double)audio_st->pts.val * audio_st->time_base.num 
                    / audio_st->time_base.den;
    }

    audio_in->used = 0;
    return 0;
}

static void ffmpegmovie_close(void)
{
    /* just stop the whole recording */
    screenshot_stop_recording();
}

static soundmovie_funcs_t ffmpegdrv_soundmovie_funcs = {
    ffmpegmovie_init_audio,
    ffmpegmovie_encode_audio,
    ffmpegmovie_close
};

/*-----------------------*/
/* video stream encoding */
/*-----------------------*/
static int ffmpegdrv_fill_rgb_image(screenshot_t *screenshot, AVFrame *pic)
{ 
    int x, y;
    int colnum;
    int bufferoffset;
    int x_dim = screenshot->width;
    int y_dim = screenshot->height;
    int pix;

    /* center the screenshot in the video */
    bufferoffset = screenshot->x_offset 
                    + screenshot->y_offset * screenshot->draw_buffer_line_size;

    pix = 3 * ((video_width - x_dim) / 2 + (video_height - y_dim) / 2 * video_width);

    for (y = 0; y < y_dim; y++) {
        for (x=0; x < x_dim; x++) {
            colnum = screenshot->draw_buffer[bufferoffset + x];
            pic->data[0][pix++] = screenshot->palette->entries[colnum].red;
            pic->data[0][pix++] = screenshot->palette->entries[colnum].green;
            pic->data[0][pix++] = screenshot->palette->entries[colnum].blue;
        }
        pix += (3 * (video_width - x_dim));

        bufferoffset += screenshot->draw_buffer_line_size;
    }

    return 0;
}


static AVFrame* ffmpegdrv_alloc_picture(int pix_fmt, int width, int height)
{
    AVFrame *picture;
    unsigned char *picture_buf;
    int size;
    
    picture = lib_malloc(sizeof(AVFrame));
    memset(picture, 0, sizeof(AVFrame));

    picture->pts = AV_NOPTS_VALUE;

    size = (*ffmpeglib.p_avpicture_get_size)(pix_fmt, width, height);
    picture_buf = lib_malloc(size);
    memset(picture_buf, 0, size);
    if (!picture_buf) {
        lib_free(picture);
        return NULL;
    }
    (*ffmpeglib.p_avpicture_fill)((AVPicture *)picture, picture_buf, 
                   pix_fmt, width, height);

    return picture;
}


static int ffmpegdrv_open_video(AVFormatContext *oc, AVStream *st)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

    /* find the video encoder */
    codec = (*ffmpeglib.p_avcodec_find_encoder)(c->codec_id);
    if (!codec) {
        log_debug("ffmpegdrv: video codec not found");
        return -1;
    }

    /* open the codec */
    if ((*ffmpeglib.p_avcodec_open)(c, codec) < 0) {
        log_debug("ffmpegdrv: could not open video codec");
        return -1;
    }

    video_is_open = 1;
    video_outbuf = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        video_outbuf_size = 200000;
        video_outbuf = lib_malloc(video_outbuf_size);
    }

    /* allocate the encoded raw picture */
    picture = ffmpegdrv_alloc_picture(c->pix_fmt, c->width, c->height);
    if (!picture) {
        log_debug("ffmpegdrv: could not allocate picture");
        return -1;
    }

    /* if the output format is not RGB24, then a temporary YUV420P
       picture is needed too. It is then converted to the required
       output format */
    tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_RGB24) {
        tmp_picture = ffmpegdrv_alloc_picture(PIX_FMT_RGB24, 
                                                c->width, c->height);
        if (!tmp_picture) {
            log_debug("ffmpegdrv: could not allocate temporary picture");
            return -1;
        }
    }
    return 0;
}


static void ffmpegdrv_close_video(void)
{
    if (video_st == NULL)
        return;

    if (video_is_open)
        (*ffmpeglib.p_avcodec_close)(video_st->codec);

    video_is_open = 0;
    lib_free(video_outbuf);
    video_outbuf = NULL;
    if (picture) {
	lib_free(picture->data[0]);
	lib_free(picture);
	picture = NULL;
    }
    if (tmp_picture) {
	lib_free(tmp_picture->data[0]);
	lib_free(tmp_picture);
	tmp_picture = NULL;
    }
    
#ifdef HAVE_FFMPEG_SWSCALE
    if (sws_ctx != NULL) {
        (*ffmpeglib.p_sws_freeContext)(sws_ctx);
    }
#endif
}


static void ffmpegdrv_init_video(screenshot_t *screenshot)
{
    AVCodecContext *c;
    AVStream *st;

    if (ffmpegdrv_oc == NULL || ffmpegdrv_fmt == NULL)
        return;

     video_init_done = 1;

     if (ffmpegdrv_fmt->video_codec == CODEC_ID_NONE)
        return;

    st = (*ffmpeglib.p_av_new_stream)(ffmpegdrv_oc, 0);
    if (!st) {
        log_debug("ffmpegdrv: Could not alloc video stream\n");
        return;
    }

    c = st->codec;
    c->codec_id = ffmpegdrv_fmt->video_codec;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = video_bitrate;
    /* resolution should be a multiple of 16 */
    video_width = c->width = (screenshot->width + 15) & ~0xf; 
    video_height = c->height = (screenshot->height + 15) & ~0xf;
    /* frames per second */
    c->time_base.den = (int)(vsync_get_refresh_frequency() + 0.5);
    if (video_halve_framerate) {
        c->time_base.den /= 2;
    }
    c->time_base.num = 1;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = PIX_FMT_YUV420P;

    /* Avoid format conversion which would lead to loss of quality */
    if (c->codec_id == CODEC_ID_FFV1) {
        c->pix_fmt = PIX_FMT_RGB32;
    }

    /* Use XVID instead of FMP4 FOURCC for better compatibility */
    if (c->codec_id == CODEC_ID_MPEG4) {
        c->codec_tag = MKTAG('X','V','I','D');
    }

#ifdef HAVE_FFMPEG_SWSCALE
    /* setup scaler */
    if (c->pix_fmt != PIX_FMT_RGB24) {
        sws_ctx = (*ffmpeglib.p_sws_getContext)
            (video_width, video_height, PIX_FMT_RGB24, 
             video_width, video_height, c->pix_fmt, 
             SWS_BICUBIC, 
             NULL, NULL, NULL);
        if (sws_ctx == NULL) {
            log_debug("ffmpegdrv: Can't create Scaler!\n");
        }
    }
#endif

    video_st = st;
    video_pts = 0;
    framecounter = 0;

    if (audio_init_done)
        ffmpegdrv_init_file();
}


static int ffmpegdrv_init_file(void)
{
    if (!video_init_done || !audio_init_done)
        return 0;

    if ((*ffmpeglib.p_av_set_parameters)(ffmpegdrv_oc, NULL) < 0) {
        log_debug("ffmpegdrv: Invalid output format parameters");
            return -1;
    }

    (*ffmpeglib.p_dump_format)(ffmpegdrv_oc, 0, ffmpegdrv_oc->filename, 1);

    if (video_st && (ffmpegdrv_open_video(ffmpegdrv_oc, video_st) < 0)) {
        ui_error(translate_text(IDGS_FFMPEG_CANNOT_OPEN_VSTREAM));
        screenshot_stop_recording();
        return -1;
    }
    if (audio_st && (ffmpegdrv_open_audio(ffmpegdrv_oc, audio_st) < 0)) {
        ui_error(translate_text(IDGS_FFMPEG_CANNOT_OPEN_ASTREAM));
        screenshot_stop_recording();
        return -1;
    }

    if (!(ffmpegdrv_fmt->flags & AVFMT_NOFILE)) {
        if ((*ffmpeglib.p_url_fopen)(&ffmpegdrv_oc->pb, ffmpegdrv_oc->filename,
                            URL_WRONLY) < 0) 
        {
            ui_error(translate_text(IDGS_FFMPEG_CANNOT_OPEN_S), ffmpegdrv_oc->filename);
            screenshot_stop_recording();
            return -1;
        }

    }

    (*ffmpeglib.p_av_write_header)(ffmpegdrv_oc);

    log_debug("ffmpegdrv: Initialized file successfully");

    file_init_done = 1;

    return 0;
}


static int ffmpegdrv_save(screenshot_t *screenshot, const char *filename)
{
    video_st = NULL;
    audio_st = NULL;

    audio_init_done = 0;
    video_init_done = 0;
    file_init_done = 0;

    ffmpegdrv_fmt = (*ffmpeglib.p_av_guess_format)(ffmpeg_format, NULL, NULL);

    if (!ffmpegdrv_fmt)
        ffmpegdrv_fmt = (*ffmpeglib.p_av_guess_format)("mpeg", NULL, NULL);

    if (!ffmpegdrv_fmt) {
        log_debug("ffmpegdrv: Cannot find suitable output format");
        return -1;
    }

    if (format_index >= 0) {

        gfxoutputdrv_format_t *format = &ffmpegdrv_formatlist[format_index];

        if (format->audio_codecs !=NULL
            && (*ffmpeglib.p_avcodec_find_encoder)(audio_codec) != NULL)
        {
            ffmpegdrv_fmt->audio_codec = audio_codec;
        }

        if (format->video_codecs !=NULL
            && (*ffmpeglib.p_avcodec_find_encoder)(video_codec) != NULL)
        {
            ffmpegdrv_fmt->video_codec = video_codec;
        }
    }

    ffmpegdrv_oc = lib_malloc(sizeof(AVFormatContext));
    memset(ffmpegdrv_oc, 0, sizeof(AVFormatContext));

    if (!ffmpegdrv_oc) {
        log_debug("ffmpegdrv: Cannot allocate format context");
        return -1;
    }

    ffmpegdrv_oc->oformat = ffmpegdrv_fmt;
    snprintf(ffmpegdrv_oc->filename, sizeof(ffmpegdrv_oc->filename), 
             "%s", filename);

    ffmpegdrv_init_video(screenshot);

    soundmovie_start(&ffmpegdrv_soundmovie_funcs);

    return 0;
}


static int ffmpegdrv_close(screenshot_t *screenshot)
{
    unsigned int i;

    soundmovie_stop();

    if (video_st)
        ffmpegdrv_close_video();
    if (audio_st)
        ffmpegdrv_close_audio();

    /* write the trailer, if any */
    if (file_init_done) {
        (*ffmpeglib.p_av_write_trailer)(ffmpegdrv_oc);
        if (!(ffmpegdrv_fmt->flags & AVFMT_NOFILE)) {
            /* close the output file */
            (*ffmpeglib.p_url_fclose)(ffmpegdrv_oc->pb);
        }
    }
    
    /* free the streams */
    for (i = 0; i < ffmpegdrv_oc->nb_streams; i++) {
        (*ffmpeglib.p_av_free)((void *)ffmpegdrv_oc->streams[i]);
        ffmpegdrv_oc->streams[i] = NULL;
    }

    /* free the stream */
    lib_free(ffmpegdrv_oc);
    log_debug("ffmpegdrv: Closed successfully");

    file_init_done = 0;

    return 0;
}

#if FFMPEG_ALIGNMENT_HACK
__declspec(naked) static int ffmpeg_avcodec_encode_video(AVCodecContext* c, uint8_t* video_outbuf, int video_outbuf_size, const AVFrame* picture)
{
    _asm {
        /*
         * Create a standard stack frame.
         * This way, we can be sure that we
         * can restore ESP afterwards.
         */
        push ebp
        mov ebp,esp
        sub esp, __LOCAL_SIZE /* not needed, but safer against errors when changing this function */

        /* adjust stack to 16 byte boundary */
        and esp,~0x0f
    }

    /* execute the command */

    (*ffmpeglib.p_avcodec_encode_video)(c, video_outbuf, video_outbuf_size, picture);

    _asm {
        /* undo the stack frame, restoring ESP and EBP */
        mov esp,ebp
        pop ebp
        ret
    }
}
#endif

/* triggered by screenshot_record */
static int ffmpegdrv_record(screenshot_t *screenshot)
{
    AVCodecContext *c;
    int out_size;
    int ret;

    if (audio_init_done && video_init_done && !file_init_done)
        ffmpegdrv_init_file();

    if (video_st == NULL || !file_init_done)
        return 0;

    if (audio_st && video_pts > audio_pts) {
        /* drop this frame */
        return 0;
    }
    
    framecounter++;
    if (video_halve_framerate && (framecounter & 1)) {
        /* drop every second frame */
        return 0;
    }

    c = video_st->codec;

    if (c->pix_fmt != PIX_FMT_RGB24) {
        ffmpegdrv_fill_rgb_image(screenshot, tmp_picture);
#ifdef HAVE_FFMPEG_SWSCALE
        if (sws_ctx != NULL) {
            (*ffmpeglib.p_sws_scale)(sws_ctx, 
                tmp_picture->data, tmp_picture->linesize, 0, c->height,
                picture->data, picture->linesize);
        }
#else
        (*ffmpeglib.p_img_convert)((AVPicture *)picture, c->pix_fmt,
                    (AVPicture *)tmp_picture, PIX_FMT_RGB24,
                    c->width, c->height);
#endif
    } else {
        ffmpegdrv_fill_rgb_image(screenshot, picture);
    }

    if (ffmpegdrv_oc->oformat->flags & AVFMT_RAWPICTURE) {
        AVPacket pkt;
        (*ffmpeglib.p_av_init_packet)(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = video_st->index;
        pkt.data = (uint8_t*)picture;
        pkt.size = sizeof(AVPicture);

        ret = (*ffmpeglib.p_av_write_frame)(ffmpegdrv_oc, &pkt);
    } else {
        /* encode the image */
#if FFMPEG_ALIGNMENT_HACK
        out_size = ffmpeg_avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
#else
        out_size = (*ffmpeglib.p_avcodec_encode_video)(c, video_outbuf, 
                                                video_outbuf_size, picture);
#endif
        /* if zero size, it means the image was buffered */
        if (out_size != 0) {
            /* write the compressed frame in the media file */
            AVPacket pkt;
            (*ffmpeglib.p_av_init_packet)(&pkt);
            pkt.pts = c->coded_frame->pts;
            if (c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index = video_st->index;
            pkt.data = video_outbuf;
            pkt.size = out_size;
            ret = (*ffmpeglib.p_av_write_frame)(ffmpegdrv_oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        log_debug("Error while writing video frame");
        return -1;
    }

    video_pts = (double)video_st->pts.val * video_st->time_base.num 
                    / video_st->time_base.den;

    return 0;
}


static int ffmpegdrv_write(screenshot_t *screenshot)
{
    return 0;
}

static void ffmpegdrv_shutdown(void)
{
    ffmpeglib_close(&ffmpeglib);
    lib_free(ffmpeg_format);
}

static gfxoutputdrv_t ffmpeg_drv = {
    "FFMPEG",
    "FFMPEG",
    NULL,
    ffmpegdrv_formatlist,
    NULL, /* open */
    ffmpegdrv_close,
    ffmpegdrv_write,
    ffmpegdrv_save,
    NULL,
    ffmpegdrv_record,
    ffmpegdrv_shutdown,
    ffmpegdrv_resources_init,
    ffmpegdrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    ,NULL
#endif
};

void gfxoutput_init_ffmpeg(void)
{
    if (ffmpeglib_open(&ffmpeglib) < 0)
        return;

    gfxoutput_register(&ffmpeg_drv);

    (*ffmpeglib.p_av_register_all)();
}

