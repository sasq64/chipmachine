#include "raster-modes.h"
#include "viciitypes.h"
#include "vicii-draw.h"

static int get_null(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    return 0;
}

static void draw_null_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
}

static void draw_null_line(void)
{
}

static void draw_null(unsigned int start_pixel,
                                unsigned int end_pixel)
{
}

static void setup_modes(void)
{
    raster_modes_set(vicii.raster.modes, VICII_NORMAL_TEXT_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_MULTICOLOR_TEXT_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_HIRES_BITMAP_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_MULTICOLOR_BITMAP_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_EXTENDED_TEXT_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_IDLE_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_TEXT_MODE,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_BITMAP_MODE_1,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_BITMAP_MODE_2,
                     get_null,
                     draw_null_cached,
                     draw_null_line,
                     draw_null,
                     draw_null);
}

/* Initialize the drawing tables.  */
static void init_drawing_tables(void)
{
}

void vicii_draw_init(void)
{
    setup_modes();
}

