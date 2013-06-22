#ifndef _UADE123_H_
#define _UADE123_H_

#include <limits.h>
#include <stdio.h>

#include "playlist.h"
#include <effects.h>


#define debug(verbose, fmt, args...) if (verbose) { fprintf(stderr, fmt, ## args); }
#define tprintf(fmt, args...) do {fprintf(uade_terminal_file ? uade_terminal_file : stdout, fmt, ## args); } while (0)

extern int uade_debug_trigger;
extern int uade_info_mode;
extern double uade_jump_pos;
extern int uade_no_audio_output;
extern int uade_no_text_output;
extern char uade_output_file_format[16];
extern char uade_output_file_name[PATH_MAX];
extern struct playlist uade_playlist;
extern int uade_song_end_trigger;
extern int uade_terminated;
extern FILE *uade_terminal_file;


void print_action_keys(void);
void set_filter_on(const char *model);
void set_interpolation_mode(const char *value);
int test_song_end_trigger(void);

#endif
