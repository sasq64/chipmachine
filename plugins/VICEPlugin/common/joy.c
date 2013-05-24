#include "joy.h"
#include "userport_joystick.h"

int joystick_arch_init_resources(void)
{
    return 0;
}

int joystick_init_cmdline_options(void)
{
    return 0;
}

int joy_arch_init(void)
{
    return 0;
}

int userport_joystick_enable;
int userport_joystick_type;

void userport_joystick_store_pa2(BYTE value) {}
void userport_joystick_store_pbx(BYTE value) {}
void userport_joystick_store_sdr(BYTE value) {}

BYTE userport_joystick_read_pa2(BYTE orig) { return 0xff; }
BYTE userport_joystick_read_pbx(BYTE orig) { return 0xff; }
BYTE userport_joystick_read_sdr(BYTE orig) { return 0xff; }
