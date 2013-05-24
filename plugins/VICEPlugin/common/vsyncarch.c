/*
 * vsyncarch.c - End-of-frame handling for Unix
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


#include "vice.h"

#include "kbdbuf.h"
#include "ui.h"
#include "vsyncapi.h"
#include "videoarch.h"

#ifdef HAS_JOYSTICK
#include "joy.h"
#endif

#ifdef HAVE_OPENGL_SYNC
#include "openGL_sync.h"
#endif

#ifdef HAVE_NANOSLEEP
#include <time.h>
#else
#include <unistd.h>
#endif
#include <sys/time.h>

/* hook to ui event dispatcher */
static void_hook_t ui_dispatch_hook;

/* ------------------------------------------------------------------------- */
/* Mac OS X has its own version of these functions. See macosx/vsyncarch.c */
#ifndef MACOSX_SUPPORT

/* Number of timer units per second. */
signed long vsyncarch_frequency(void)
{
    /* Microseconds resolution. */
    return 1000000;
}

/* Get time in timer units. */
unsigned long vsyncarch_gettime(void)
{
    struct timeval now;

    gettimeofday(&now, NULL);

    return 1000000UL * now.tv_sec + now.tv_usec;
}
#endif

void vsyncarch_init(void)
{
    vsync_set_event_dispatcher(ui_dispatch_events);
}

/* Display speed (percentage) and frame rate (frames per second). */
void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled)
{
    ui_display_speed((float)speed, (float)frame_rate, warp_enabled);
}

/* Sleep a number of timer units. */
void vsyncarch_sleep(signed long delay)
{
#ifdef __BEOS__
    snooze(delay);
#else
#ifdef HAVE_NANOSLEEP
    struct timespec ts;
    ts.tv_sec = delay / 1000000;
    ts.tv_nsec = (delay % 1000000) * 1000;
    /* wait until whole interval has elapsed */
    while (nanosleep(&ts, &ts));
#else
    usleep(delay);
#endif
#endif
}

void vsyncarch_presync(void)
{
#if defined(GP2X) || defined(WIZ)
    (*ui_dispatch_hook)();
#endif
#if defined(HAVE_MOUSE) && !defined(GP2X) && !defined(WIZ) && !defined(MACOSX_COCOA)
    {
        extern void x11_lightpen_update(void);
        x11_lightpen_update();
    }
#endif /* HAVE_MOUSE !GP2X !WIZ !MACOSX_COCOA */
    kbdbuf_flush();
#ifdef HAS_JOYSTICK
    joystick();
#endif
}

void_hook_t vsync_set_event_dispatcher(void_hook_t hook)
{
    void_hook_t t = ui_dispatch_hook;

    ui_dispatch_hook = hook;
    return t;
}

void vsyncarch_postsync(void)
{
#if !defined(GP2X) && !defined(WIZ)
    (*ui_dispatch_hook)();
#endif
}

#ifdef HAVE_OPENGL_SYNC
void
vsyncarch_sync_with_raster(video_canvas_t *c)
{
    openGL_sync_with_raster();
}

int vsyncarch_vbl_sync_enabled(void)
{
    return openGL_sync_enabled();
}

#endif
