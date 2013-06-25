#ifndef _UADE_FRONTEND_CONFIG_H_
#define _UADE_FRONTEND_CONFIG_H_

#include <string.h>

struct uade_state;
struct uade_config;

double uade_convert_to_double(const char *value, double def,
			      double low, double high, const char *type);

int uade_load_config(struct uade_state *state, const char *filename);

int uade_load_initial_config(struct uade_state *state, const char *bdir);

int uade_load_initial_song_conf(struct uade_state *state);

void uade_merge_configs(struct uade_config *ucd, const struct uade_config *ucs);

char *uade_open_create_home(void);

int uade_parse_subsongs(int **subsongs, char *option);

void uade_set_effects(struct uade_state *state);

void uade_set_options_from_ep_attributes(struct uade_state *state);

int uade_set_options_from_song_attributes(struct uade_state *state,
					  char *playername,
					  size_t playernamelen);

#endif
