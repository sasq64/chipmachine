#include "fullscreen.h"
#include "fullscreenarch.h"
#include "videoarch.h"

int fullscreen_available(void) 
{
    return 0;
}

void fullscreen_shutdown(void)
{
}

void fullscreen_suspend(int level)
{
}

void fullscreen_resume(void)
{
}

void fullscreen_set_mouse_timeout(void)
{
}

void fullscreen_mouse_moved(struct video_canvas_s *canvas, int x, int y, int leave)
{
}

void fullscreen_mode_callback(const char *device, void *callback)
{
}

void fullscreen_menu_create(struct ui_menu_entry_s *menu)
{
}

void fullscreen_menu_shutdown(struct ui_menu_entry_s *menu)
{
}

int fullscreen_init(void)
{
    return 0;
}

int fullscreen_init_alloc_hooks(struct video_canvas_s *canvas)
{
    return 0;
}

void fullscreen_shutdown_alloc_hooks(struct video_canvas_s *canvas)
{
}

/*
    resize the fullscreen canvas, taking eventually visible gui
    elements into account to maintain aspect ratio
*/
void fullscreen_resize(struct video_canvas_s *canvas, int uienable)
{
}

/* enable/disable statusbar and menubar */
static int fullscreen_statusbar(struct video_canvas_s *canvas, int enable)
{
    return 0;
}

static int fullscreen_enable(struct video_canvas_s *canvas, int enable)
{
    return 0;
}

static int fullscreen_double_size(struct video_canvas_s *canvas, int double_size)
{
    return 0;
}

static int fullscreen_double_scan(struct video_canvas_s *canvas, int double_scan)
{
    return 0;
}

static int fullscreen_device(struct video_canvas_s *canvas, const char *device)
{
    return 0;
}

void fullscreen_capability(cap_fullscreen_t *cap_fullscreen)
{
    cap_fullscreen->device_num = 0;
}
