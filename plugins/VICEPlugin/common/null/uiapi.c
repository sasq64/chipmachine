#include <stdlib.h>
#include "uiapi.h"

int ui_resources_init(void) { return 0; }
void ui_resources_shutdown(void) {}
int ui_cmdline_options_init(void) { return 0; }
int ui_init(int *argc, char **argv) { return 0; }
int ui_init_finish(void) { return 0; }
int ui_init_finalize(void) { return 0; }
void ui_shutdown(void) {}
void ui_message(const char *format,...) {}
void ui_error(const char *format,...) {}
void ui_display_statustext(const char *text, int fade_out) {}
char* ui_get_file(const char *format,...) { return NULL; }
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color) {}
void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number) {}
/* The pwm value will vary between 0 and 1000.  */
void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2) {}
void ui_display_drive_current_image(unsigned int drive_number, const char *image) {}
int ui_extend_image_dialog(void) {}
void ui_set_tape_status(int tape_status) {}
void ui_display_tape_motor_status(int motor) {}
void ui_display_tape_control_status(int control) {}
void ui_display_tape_counter(int counter) {}
void ui_display_tape_current_image(const char *image) {}
ui_jam_action_t ui_jam_dialog(const char *format, ...) { return UI_JAM_NONE; }
void ui_update_menus(void) {}
void ui_display_playback(int playback_status, char *version) {}
void ui_display_recording(int recording_status) {}
void ui_display_event_time(unsigned int current, unsigned int total) {}
void ui_display_joyport(BYTE *joyport) {}
void ui_display_volume(int vol) {}
extern void ui_dispatch_next_event(void) {}
