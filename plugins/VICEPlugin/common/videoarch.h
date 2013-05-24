#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

#include "vice.h"
#include "types.h"
#include "video.h"

struct palette_s;
struct video_draw_buffer_callback_s;

typedef struct video_canvas_s {
    int initialized;
    struct video_render_config_s *videoconfig;
    struct draw_buffer_s *draw_buffer;
    struct viewport_s *viewport;
    struct geometry_s *geometry;
    struct palette_s *palette;
    char *pixels;
    struct video_draw_buffer_callback_s *video_draw_buffer_callback;
} video_canvas_t;

#endif
