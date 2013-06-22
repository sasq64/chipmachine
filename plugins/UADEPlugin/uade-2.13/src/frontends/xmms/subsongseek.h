#ifndef _UADE_SUBSONG_SEEK
#define _UADE_SUBSONG_SEEK

int uade_gui_get_cur_subsong(int def);
int uade_gui_get_min_subsong(int def);
int uade_gui_get_max_subsong(int def);

void uade_gui_seek_subsong(int to);
void uade_gui_close_subsong_win(void);
void uade_gui_subsong_changed(int subsong);

#endif
