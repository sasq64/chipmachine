#ifndef _AUDACIOUS_UADE2_H
#define _AUDACIOUS_UADE2_H

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <audacious/plugin.h>
#include <audacious/util.h>
#include <audacious/output.h> /* for Audacious 2*/

extern int uade_cur_sub;
extern int uade_is_paused;
extern int uade_max_sub;
extern int uade_min_sub;
extern int uade_seek_forward;
extern int uade_select_sub;
extern int uade_thread_running;

InputPlugin *get_iplugin_info(void);
int uade_get_cur_subsong(int def);
int uade_get_max_subsong(int def);
int uade_get_min_subsong(int def);
void uade_lock(void);
void uade_unlock(void);

#endif
