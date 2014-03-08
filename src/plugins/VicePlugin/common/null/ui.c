#include <stdlib.h>
#include "ui.h"

#define ALT_AS_META

/* Number of drives we support in the UI.  */
#define NUM_DRIVES 4

/* Tell menu system to ignore a string for translation
   (e.g. filenames in fliplists) */
#define NO_TRANS "no-trans"

void ui_display_speed(float percent, float framerate, int warp_flag) {}
void ui_display_paused(int flag) {}
void ui_dispatch_events(void) {}
void ui_exit(void) {}
void ui_show_text(const char *title, const char *text, int width, int height) {}

extern char *ui_select_file(const char *title, read_contents_func_type read_contents_func, unsigned int allow_autostart, const char *default_dir, uilib_file_filter_enum_t* patterns, int num_patterns, ui_button_t *button_return, unsigned int show_preview, int *attach_wp, ui_filechooser_t action) { return NULL; }

ui_button_t ui_input_string(const char *title, const char *prompt, char *buf, unsigned int buflen) { return UI_BUTTON_CANCEL; }

ui_button_t ui_ask_confirmation(const char *title, const char *text) { return UI_BUTTON_CANCEL; }
void ui_autorepeat_on(void) {}
void ui_autorepeat_off(void) {}
void ui_pause_emulation(int flag) {}
int ui_emulation_is_paused(void) {return 0;}
void ui_check_mouse_cursor(void) {}
void ui_restore_mouse(void) {}

void archdep_ui_init(int argc, char *argv[]) {}
void ui_set_application_icon(const char *icon_data[]) {}
void ui_set_selected_file(int num) {}

extern void ui_update_pal_ctrls(int v) {}

extern void ui_common_init(void) {}
extern void ui_common_shutdown(void) {}
