#ifndef _UADECONF_STRUCTURE_H_
#define _UADECONF_STRUCTURE_H_

#include <limits.h>

struct uade_path {
	char name[PATH_MAX];
};

struct uade_ep_options {
	char o[256];
	size_t s;
};

struct uade_ao_options {
	char o[256];
};

#define UADE_CHAR_CONFIG(x) char x; char x##_set
#define UADE_FLOAT_CONFIG(x) float x; char x##_set
#define UADE_INT_CONFIG(x) int x; char x##_set
#define UADE_PATH_CONFIG(x) struct uade_path x; char x##_set

/* All the options are put into an instance of this structure.
 * There can be many structures, one for uade.conf and the other for
 * command line options. Then these structures are then merged together
 * to know the complete behavior for each case. Note, these structures
 * can be conflicting, so the options are merged in following order
 * so that the last merge will determine true behavior:
 *
 *     1. set uade.conf options
 *     2. set eagleplayer attributes
 *     3. set song attributes
 *     4. set command line options
 *
 * Merging works by looking at X_set members of this structure. X_set
 * member indicates that feature X has explicitly been set, so the
 * merge will notice the change in value.
 */
struct uade_config {
	UADE_PATH_CONFIG(basedir);
	UADE_PATH_CONFIG(player_file);
	UADE_PATH_CONFIG(score_file);
	UADE_PATH_CONFIG(uadecore_file);
	UADE_PATH_CONFIG(uae_config_file);

	UADE_CHAR_CONFIG(content_detection);

	struct uade_ep_options ep_options;
	char ep_options_set;

	UADE_CHAR_CONFIG(filter_type);
	UADE_INT_CONFIG(frequency);
	UADE_CHAR_CONFIG(led_forced);
	UADE_CHAR_CONFIG(led_state);

	UADE_CHAR_CONFIG(gain_enable);
	/* should be removed of uade_effect integrated */
	UADE_FLOAT_CONFIG(gain);

	UADE_CHAR_CONFIG(headphones);
	UADE_CHAR_CONFIG(headphones2);
	UADE_CHAR_CONFIG(ignore_player_check);

	char *resampler;
	char resampler_set;

	UADE_CHAR_CONFIG(no_ep_end);
	UADE_CHAR_CONFIG(no_filter);
	UADE_CHAR_CONFIG(no_postprocessing);

	UADE_CHAR_CONFIG(one_subsong);
	UADE_FLOAT_CONFIG(panning);		/* should be removed */
	UADE_CHAR_CONFIG(panning_enable);
	UADE_INT_CONFIG(silence_timeout);
	UADE_CHAR_CONFIG(speed_hack);
	UADE_INT_CONFIG(subsong_timeout);
	UADE_INT_CONFIG(timeout);
	UADE_CHAR_CONFIG(use_text_scope);
	UADE_CHAR_CONFIG(use_timeouts);
	UADE_CHAR_CONFIG(use_ntsc);
	UADE_CHAR_CONFIG(verbose);
};

#endif
