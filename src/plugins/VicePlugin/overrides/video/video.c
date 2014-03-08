#include <stdlib.h>
#include "video.h"
#include "videoarch.h"
#include "viewport.h"
#include "lib.h"

void video_render_initconfig(video_render_config_t *config) {}
void video_render_setphysicalcolor(video_render_config_t *config, int index, DWORD color, int depth) {}
void video_render_setrawrgb(unsigned int index, DWORD r, DWORD g, DWORD b) {}
void video_render_initraw(struct video_render_config_s *videoconfig) {}

/**************************************************************/

int video_init_cmdline_options(void) {return 0;}
int video_init(void) {return 0;}
void video_shutdown(void) {}

struct video_canvas_s *video_canvas_create(struct video_canvas_s *canvas, unsigned int *width, unsigned int *height, int mapped) { return NULL; }
void video_arch_canvas_init(struct video_canvas_s *canvas) {}
void video_canvas_shutdown(struct video_canvas_s *canvas) {
    if (canvas != NULL) {
        lib_free(canvas->videoconfig);
        lib_free(canvas->draw_buffer);
        lib_free(canvas->viewport);
        lib_free(canvas->geometry);
        lib_free(canvas);
    }
}

struct video_canvas_s *video_canvas_init(void) {
    video_canvas_t *canvas = (void*) lib_calloc(1, sizeof(video_canvas_t));
    canvas->videoconfig = (void*) lib_calloc(1, sizeof(video_render_config_t));

    canvas->draw_buffer = (void*) lib_calloc(1, sizeof(draw_buffer_t));
    canvas->viewport = (void*) lib_calloc(1, sizeof(viewport_t));
    canvas->geometry = (void*) lib_calloc(1, sizeof(geometry_t));
    return canvas;
}

void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h) {}

int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette) {return 0;}
/* This will go away.  */
int video_canvas_palette_set(struct video_canvas_s *canvas,
                                    struct palette_s *palette) {return 0;}
void video_canvas_create_set(struct video_canvas_s *canvas) {}
void video_canvas_destroy(struct video_canvas_s *canvas) {}
void video_canvas_map(struct video_canvas_s *canvas) {}
void video_canvas_unmap(struct video_canvas_s *canvas) {}
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas) {}
void video_canvas_render(struct video_canvas_s *canvas, BYTE *trg, int width, int height, int xs, int ys, int xt, int yt, int pitcht, int depth) {}
void video_canvas_refresh_all(struct video_canvas_s *canvas) {}
void video_canvas_redraw_size(struct video_canvas_s *canvas, unsigned int width, unsigned int height) {}
void video_viewport_get(struct video_canvas_s *canvas, struct viewport_s **viewport, struct geometry_s **geometry) {
    *viewport = canvas->viewport;
    *geometry = canvas->geometry;
}
void video_viewport_resize(struct video_canvas_s *canvas, char resize_canvas) {}
void video_viewport_title_set(struct video_canvas_s *canvas, const char *title) {}
void video_viewport_title_free(struct viewport_s *viewport) {}

int video_resources_init(void) { return 0; }
void video_resources_shutdown(void) {}
int video_resources_pal_init(void) { return 0; }
int video_resources_crt_init(void) { return 0; }
int video_resources_chip_init(const char *chipname, struct video_canvas_s **canvas, video_chip_cap_t *video_chip_cap) { return 0; }
void video_resources_chip_shutdown(struct video_canvas_s *canvas) {}
int video_cmdline_options_chip_init(const char *chipname, video_chip_cap_t *video_chip_cap) { return 0; }
int video_arch_resources_init(void) { return 0; }
void video_arch_resources_shutdown(void) {}

void video_color_palette_internal(struct video_canvas_s *canvas, struct video_cbm_palette_s *cbm_palette) {}
int video_color_update_palette(struct video_canvas_s *canvas) { return 0; }
void video_color_palette_free(struct palette_s *palette) {}
void video_color_set_canvas(struct video_canvas_s *canvas) {}

int video_render_get_fake_pal_state(void) { return 0; }
void video_render_1x2_init(void) {}
void video_render_2x2_init(void) {}
void video_render_pal_init(void) {}
void video_render_crt_init(void) {}
