#ifndef _UADE_MAIN_H_
#define _UADE_MAIN_H_

#include <limits.h>
#include <stdlib.h>

#include <uade/uadeipc.h>

struct uade_song {
  char playername[PATH_MAX];       /* filename of eagleplayer */
  char modulename[PATH_MAX];       /* filename of song */
  char scorename[PATH_MAX];        /* filename of score file */

  int min_subsong;
  int max_subsong;
  int cur_subsong;
};

void uadecore_check_sound_buffers(int bytes);
void uadecore_send_debug(const char *fmt, ...);
void uadecore_get_amiga_message(void);
void uadecore_handle_r_state(void);
void uadecore_option(int, char**); /* handles command line parameters */
void uadecore_reset(void);
void uadecore_send_amiga_message(int msgtype);
void uadecore_set_automatic_song_end(int song_end_possible);
void uadecore_set_ntsc(int usentsc);
void uadecore_song_end(char *reason, int kill_it);
void uadecore_swap_buffer_bytes(void *data, int bytes);

extern int uadecore_audio_output;
extern int uadecore_audio_skip;
extern int uadecore_debug;
extern int uadecore_local_sound;
extern int uadecore_read_size;
extern int uadecore_reboot;
extern int uadecore_time_critical;

extern struct uade_ipc uadecore_ipc;

#endif
