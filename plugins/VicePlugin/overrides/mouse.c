#include "vice.h"

#include "mouse.h"

/* --------------------------------------------------------- */
/* extern variables */

int _mouse_enabled = 0;
int mouse_port = 1;
int mouse_type;

/* --------------------------------------------------------- */
/* POT input selection */

/* POT input port. Defaults to 1 for xvic. */
static BYTE input_port = 1;

void mouse_set_input(int port)
{
    input_port = port & 3;
}


static BYTE mouse_get_1351_x(void)
{
    return 0;
}

static BYTE mouse_get_1351_y(void)
{
    return 0;
}

/* --------------------------------------------------------- */
/* NEOS mouse */

#define NEOS_RESET_CLK 100
struct alarm_s *neosmouse_alarm;

static void neos_get_new_movement(void)
{
}

void neos_mouse_store(BYTE val)
{
}

BYTE neos_mouse_read(void)
{
    return 0;
}


static void neosmouse_alarm_handler(CLOCK offset, void *data)
{
}

BYTE amiga_mouse_read(void)
{
    return 0;
}

int mouse_resources_init(void)
{
    return 0;
}

int mouse_cmdline_options_init(void)
{
    return 0;
}

void mouse_init(void)
{
}

void mouse_button_left(int pressed)
{
}

void mouse_button_right(int pressed)
{
}

BYTE mouse_get_x(void)
{
    return 0;
}

BYTE mouse_get_y(void)
{
    return 0;
}
