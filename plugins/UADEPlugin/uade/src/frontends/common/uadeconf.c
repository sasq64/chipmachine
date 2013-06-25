/*
 * Handle uade.conf file
 *
 * Copyright (C) 2005 Heikki Orsila <heikki.orsila@iki.fi>
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any way you
 * want in your projects.
*/

#include <uade/uade.h>
#include <uade/uadeconf.h>
#include <uade/uadeconfstructure.h>
#include <uade/options.h>
#include <uade/ossupport.h>
#include <uade/uadeconstants.h>
#include <uade/amigafilter.h>
#include <uade/effects.h>
#include <uade/uadestate.h>
#include <support.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int uade_set_silence_timeout(struct uade_config *uc, const char *value);
static int uade_set_subsong_timeout(struct uade_config *uc, const char *value);
static int uade_set_timeout(struct uade_config *uc, const char *value);


struct uade_conf_opts {
	char *str;
	int l;
	enum uade_option e;
};

/* List of uade.conf options. The list includes option name, minimum
   string match length for the option name and its enum code. */
static const struct uade_conf_opts uadeconfopts[] = {
	{.str = "detect_format_by_detection", .l = 18, .e = UC_CONTENT_DETECTION},
	{.str = "disable_timeout",       .l = 1,  .e = UC_DISABLE_TIMEOUTS},
	{.str = "enable_timeout",        .l = 2,  .e = UC_ENABLE_TIMEOUTS},
	{.str = "ep_option",             .l = 2,  .e = UC_EAGLEPLAYER_OPTION},
	{.str = "filter_type",           .l = 2,  .e = UC_FILTER_TYPE},
	{.str = "force_led_off",         .l = 12, .e = UC_FORCE_LED_OFF},
	{.str = "force_led_on",          .l = 12, .e = UC_FORCE_LED_ON},
	{.str = "force_led",             .l = 9,  .e = UC_FORCE_LED},
	{.str = "frequency",             .l = 2,  .e = UC_FREQUENCY},
	{.str = "gain",                  .l = 1,  .e = UC_GAIN},
	{.str = "headphones",            .l = 11, .e = UC_HEADPHONES},
	{.str = "headphones2",           .l = 11, .e = UC_HEADPHONES2},
	{.str = "headphone",             .l = 11, .e = UC_HEADPHONES},
	{.str = "ignore_player_check",   .l = 2,  .e = UC_IGNORE_PLAYER_CHECK},
	{.str = "interpolator",          .l = 2,  .e = UC_RESAMPLER},
	{.str = "magic_detection",       .l = 1,  .e = UC_CONTENT_DETECTION},
	{.str = "no_ep_end_detect",      .l = 4,  .e = UC_NO_EP_END},
	{.str = "no_filter",             .l = 4,  .e = UC_NO_FILTER},
	{.str = "no_song_end",           .l = 4,  .e = UC_NO_EP_END},
	{.str = "ntsc",                  .l = 2,  .e = UC_NTSC},
	{.str = "one_subsong",           .l = 1,  .e = UC_ONE_SUBSONG},
	{.str = "pal",                   .l = 3,  .e = UC_PAL},
	{.str = "panning_value",         .l = 3,  .e = UC_PANNING_VALUE},
	{.str = "resampler",             .l = 1,  .e = UC_RESAMPLER},
	{.str = "silence_timeout_value", .l = 2,  .e = UC_SILENCE_TIMEOUT_VALUE},
	{.str = "speed_hack",            .l = 2,  .e = UC_SPEED_HACK},
	{.str = "subsong_timeout_value", .l = 2,  .e = UC_SUBSONG_TIMEOUT_VALUE},
	{.str = "timeout_value",         .l = 1,  .e = UC_TIMEOUT_VALUE},
	{.str = "verbose",               .l = 1,  .e = UC_VERBOSE},
	{.str = NULL} /* END OF LIST */
};


/* Map an uade.conf option to an enum */
static enum uade_option map_str_to_option(const char *key)
{
	size_t i;

	for (i = 0; uadeconfopts[i].str != NULL; i++) {
		if (strncmp(key, uadeconfopts[i].str, uadeconfopts[i].l) == 0)
			return uadeconfopts[i].e;
	}

	return 0;
}

struct uade_config *uade_new_config(void)
{
	struct uade_config *uc = calloc(1, sizeof *uc);
	if (uc)
		uade_config_set_defaults(uc);
	return uc;
}

static void uade_set_filter_type(struct uade_config *uc, const char *model)
{
	uc->filter_type = FILTER_MODEL_A500;

	if (model == NULL)
		return;

	/* a500 and a500e are the same */
	if (strncasecmp(model, "a500", 4) == 0) {
		uc->filter_type = FILTER_MODEL_A500;

		/* a1200 and a1200e are the same */
	} else if (strncasecmp(model, "a1200", 5) == 0) {
		uc->filter_type = FILTER_MODEL_A1200;

	} else {
		fprintf(stderr, "Unknown filter model: %s\n", model);
	}
}

/* The function sets the default options. No *_set variables are set because
   we don't want any option to become mergeable by default. See
   uade_merge_configs(). */
void uade_config_set_defaults(struct uade_config *uc)
{
	memset(uc, 0, sizeof(*uc));
	strlcpy(uc->basedir.name, UADE_CONFIG_BASE_DIR,	sizeof uc->basedir.name);
	uade_set_filter_type(uc, NULL);
	uc->frequency = UADE_DEFAULT_FREQUENCY;
	uc->gain = 1.0;
	uc->panning = 0.7;
	uc->silence_timeout = 20;
	uc->subsong_timeout = 512;
	uc->timeout = -1;
	uc->use_timeouts = 1;
}

double uade_convert_to_double(const char *value, double def, double low,
			      double high, const char *type)
{
	char *convertedvalue = NULL;
	char *endptr;
	char newseparator;
	double v;

	if (value == NULL)
		return def;

	v = strtod(value, &endptr);

	/* Decimal separator conversion, if needed */
	if (*endptr == ',' || *endptr == '.') {
		convertedvalue = strdup(value);
		if (convertedvalue == NULL) {
			uade_warning("Out of memory\n");
			return def;
		}

		newseparator = (*endptr == ',') ? '.' : ',';

		convertedvalue[(intptr_t) endptr - (intptr_t) value] = newseparator;

		v = strtod(convertedvalue, &endptr);
	}

	if (*endptr != 0 || v < low || v > high) {
		uade_warning("Invalid %s value: %s\n", type, value);
		v = def;
	}

	free_and_null(convertedvalue);

	return v;
}

static void uade_add_ep_option(struct uade_ep_options *opts, const char *s)
{
	size_t freespace = sizeof(opts->o) - opts->s;

	if (strlcpy(&opts->o[opts->s], s, freespace) >= freespace) {
		fprintf(stderr, "Warning: uade eagleplayer option overflow: %s\n", s);
		return;
	}

	opts->s += strlen(s) + 1;
}

static int set_options_from_attributes(struct uade_state *state,
				       char *playername,
				       size_t playernamelen,
				       struct uade_attribute *attributes)
{
	struct uade_song_state *us = &state->song;
	struct uade_config *uc = &state->config;

	for (; attributes != NULL; attributes = attributes->next) {
		switch (attributes->flag) {
		case ES_EP_OPTION:
			uade_debug(state, "Using eagleplayer option %s\n", attributes->s);
			uade_add_ep_option(&us->ep_options, attributes->s);
			break;

		case ES_GAIN:
			uade_config_set_option(uc, UC_GAIN, attributes->s);
			break;

		case ES_RESAMPLER:
			uade_config_set_option(uc, UC_RESAMPLER, attributes->s);
			break;

		case ES_PANNING:
			uade_config_set_option(uc, UC_PANNING_VALUE, attributes->s);
			break;

		case ES_PLAYER:
			if (playername) {
				snprintf(playername, playernamelen, "%s/players/%s", uc->basedir.name, attributes->s);
			} else {
				fprintf(stderr, "Error: attribute handling was given playername == NULL.\n");
			}
			break;

		case ES_SILENCE_TIMEOUT:
			uade_config_set_option(uc, UC_SILENCE_TIMEOUT_VALUE, attributes->s);
			break;

		case ES_SUBSONGS:
			fprintf(stderr, "Subsongs not implemented.\n");
			break;

		case ES_SUBSONG_TIMEOUT:
			uade_config_set_option(uc, UC_SUBSONG_TIMEOUT_VALUE, attributes->s);
			break;

		case ES_TIMEOUT:
			uade_config_set_option(uc, UC_TIMEOUT_VALUE, attributes->s);
			break;

		default:
			fprintf(stderr,	"Unknown song attribute flag: 0x%x\n", attributes->flag);
			break;
		}
	}

	return 0;
}

int uade_set_options_from_song_attributes(struct uade_state *state,
					  char *playername,
					  size_t playernamelen)
{
	/* Look at flags and set uade config options accordingly */
	if (uade_set_config_options_from_flags(state, state->song.flags))
		uade_warning("uade_set_song_attributes failed when setting config options from flags\n");

	return set_options_from_attributes(state,
					   playername, playernamelen,
					   state->song.songattributes);
}

int uade_load_config(struct uade_state *state, const char *filename)
{
	char line[256];
	FILE *f;
	char *key, *value;
	int linenumber = 0;
	enum uade_option opt;
	struct uade_config *uc = &state->permconfig;

	state->permconfigname[0] = 0;

	if ((f = fopen(filename, "r")) == NULL)
		return 0;

	uade_config_set_defaults(uc);

	while (uade_xfgets(line, sizeof(line), f) != NULL) {
		linenumber++;

		/* Skip comment lines */
		if (line[0] == '#')
			continue;

		if (!uade_get_two_ws_separated_fields(&key, &value, line))
			continue; /* Skip an empty line */

		opt = map_str_to_option(key);

		if (opt) {
			uade_config_set_option(uc, opt, value);
		} else {
			fprintf(stderr,	"Unknown config key in %s on line %d: %s\n", filename, linenumber, key);
		}
	}

	fclose(f);

	snprintf(state->permconfigname, sizeof(state->permconfigname), "%s", filename);

	return 1;
}

int uade_load_initial_config(struct uade_state *state, const char *bdir)
{
	int loaded = 0;
	char *home;
	char tmpname[PATH_MAX];

	state->permconfigname[0] = 0;
	uade_config_set_defaults(&state->permconfig);

	/* First try to load from forced base dir (testing mode) */
	if (bdir != NULL) {
		snprintf(tmpname, sizeof(tmpname), "%s/uade.conf", bdir);
		loaded = uade_load_config(state, tmpname);
	}

	/* Second, try to load config from ~/.uade/uade.conf */
	home = uade_open_create_home();
	if (loaded == 0 && home != NULL) {
		snprintf(tmpname, sizeof(tmpname), "%s/.uade/uade.conf", home);
		loaded = uade_load_config(state, tmpname);
	}

	/* Third, try to load from install path */
	if (loaded == 0) {
		snprintf(tmpname, sizeof(tmpname), "%s/uade.conf", state->permconfig.basedir.name);
		loaded = uade_load_config(state, tmpname);
	}

	state->config = state->permconfig;

	return loaded;
}

int uade_load_initial_song_conf(struct uade_state *state)
{
	int loaded = 0;
	char *home;
	char tmpname[PATH_MAX];
	struct uade_config *uc = &state->config;

	/* Used for testing */
	if (uc != NULL && uc->basedir_set) {
		snprintf(tmpname, sizeof tmpname, "%s/song.conf",
			 uc->basedir.name);
		loaded = uade_read_song_conf(tmpname, state);
	}

	/* Avoid unwanted home directory creation for test mode */
	if (loaded)
		return loaded;

	home = uade_open_create_home();

	/* Try to load from home dir */
	if (loaded == 0 && home != NULL) {
		snprintf(tmpname, sizeof(tmpname), "%s/.uade/song.conf", home);
		loaded = uade_read_song_conf(tmpname, state);
	}

	/* No? Try install path.. */
	if (loaded == 0) {
		snprintf(tmpname, sizeof(tmpname), "%s/song.conf", state->permconfig.basedir.name);
		loaded = uade_read_song_conf(tmpname, state);
	}

	return loaded;
}

void uade_merge_configs(struct uade_config *ucd, const struct uade_config *ucs)
{
#define MERGE_OPTION(y) do { if (ucs->y##_set) ucd->y = ucs->y; } while (0)

	MERGE_OPTION(basedir);
	MERGE_OPTION(content_detection);
	MERGE_OPTION(ep_options);
	MERGE_OPTION(filter_type);
	MERGE_OPTION(frequency);
	MERGE_OPTION(gain);
	MERGE_OPTION(gain_enable);
	MERGE_OPTION(headphones);
	MERGE_OPTION(headphones2);
	MERGE_OPTION(ignore_player_check);
	MERGE_OPTION(led_forced);
	MERGE_OPTION(led_state);
	MERGE_OPTION(no_ep_end);
	MERGE_OPTION(no_filter);
	MERGE_OPTION(no_postprocessing);

	MERGE_OPTION(one_subsong);
	MERGE_OPTION(panning);
	MERGE_OPTION(panning_enable);
	MERGE_OPTION(player_file);
	MERGE_OPTION(resampler);
	MERGE_OPTION(score_file);
	MERGE_OPTION(silence_timeout);
	MERGE_OPTION(speed_hack);
	MERGE_OPTION(subsong_timeout);

	MERGE_OPTION(timeout);
	MERGE_OPTION(uadecore_file);
	MERGE_OPTION(uae_config_file);
	MERGE_OPTION(use_timeouts);
	if (ucs->timeout_set) {
		ucd->use_timeouts = 1;
		ucd->use_timeouts_set = 1;
	}

	MERGE_OPTION(use_text_scope);
	MERGE_OPTION(use_ntsc);
	MERGE_OPTION(verbose);
}

char *uade_open_create_home(void)
{
	/* Create ~/.uade directory if it does not exist */
	char *home = getenv("HOME");
	if (home) {
		char name[PATH_MAX];
		struct stat st;
		snprintf(name, sizeof name, "%s/.uade", home);
		if (stat(name, &st) != 0)
			mkdir(name, S_IRUSR | S_IWUSR | S_IXUSR);
	}

	return home;
}

int uade_parse_subsongs(int **subsongs, char *option)
{
	char substr[256];
	char *sp, *str;
	size_t pos;
	int nsubsongs;

	nsubsongs = 0;
	*subsongs = NULL;

	if (strlcpy(substr, option, sizeof subsongs) >= sizeof subsongs) {
		fprintf(stderr, "Too long a subsong option: %s\n", option);
		return -1;
	}

	sp = substr;
	while ((str = strsep(&sp, ",")) != NULL) {
		if (*str == 0)
			continue;
		nsubsongs++;
	}

	*subsongs = malloc((nsubsongs + 1) * sizeof((*subsongs)[0]));
	if (*subsongs == NULL) {
		fprintf(stderr, "No memory for subsongs.\n");
		return -1;
	}

	strlcpy(substr, option, sizeof subsongs);

	pos = 0;
	sp = substr;
	while ((str = strsep(&sp, ",")) != NULL) {
		if (*str == 0)
			continue;
		(*subsongs)[pos] = atoi(str);
		pos++;
	}

	(*subsongs)[pos] = -1;
	assert(pos == nsubsongs);

	return nsubsongs;
}

void uade_set_effects(struct uade_state *state)
{
	struct uade_config *uc = &state->config;

	uade_effect_set_defaults(state);

	if (uc->no_postprocessing)
		uade_effect_disable(state, UADE_EFFECT_ALLOW);

	if (uc->gain_enable) {
		uade_effect_gain_set_amount(state, uc->gain);
		uade_effect_enable(state, UADE_EFFECT_GAIN);
	}

	if (uc->headphones)
		uade_effect_enable(state, UADE_EFFECT_HEADPHONES);

	if (uc->headphones2)
		uade_effect_enable(state, UADE_EFFECT_HEADPHONES2);

	if (uc->panning_enable) {
		uade_effect_pan_set_amount(state, uc->panning);
		uade_effect_enable(state, UADE_EFFECT_PAN);
	}

	uade_effect_set_sample_rate(state, uc->frequency);
}

static void handle_config_path(struct uade_path *path, char *set, const char *value)
{
	strlcpy(path->name, value, sizeof path->name);
	*set = 1;
}

void uade_config_set_option(struct uade_config *uc, enum uade_option opt,
			    const char *value)
{
	char *endptr;
	long x;

#define SET_OPTION(opt, value) do { uc->opt = (value); uc->opt##_set = 1; } while (0)

	switch (opt) {
	case UC_BASE_DIR:
		handle_config_path(&uc->basedir, &uc->basedir_set, value);
		break;

	case UC_CONTENT_DETECTION:
		SET_OPTION(content_detection, 1);
		break;

	case UC_DISABLE_TIMEOUTS:
		SET_OPTION(use_timeouts, 0);
		break;

	case UC_ENABLE_TIMEOUTS:
		SET_OPTION(use_timeouts, 1);
		break;

	case UC_EAGLEPLAYER_OPTION:
		if (value != NULL) {
			uade_add_ep_option(&uc->ep_options, value);
			uc->ep_options_set = 1;
		} else {
			fprintf(stderr,
				"uade: Passed NULL to UC_EAGLEPLAYER_OPTION.\n");
		}
		break;

	case UC_FILTER_TYPE:
		SET_OPTION(no_filter, 0);

		if (value != NULL) {
			if (strcasecmp(value, "none") != 0) {
				/* Filter != NONE */
				uade_set_filter_type(uc, value);
				uc->filter_type_set = 1;
			} else {
				/* Filter == NONE */
				uc->no_filter = 1;
			}
		}
		break;

	case UC_FORCE_LED:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_FORCE_LED value is NULL\n");
			break;
		}
		if (strcasecmp(value, "off") == 0 || strcmp(value, "0") == 0) {
			uc->led_state = 0;
		} else if (strcasecmp(value, "on") == 0 ||
			   strcmp(value, "1") == 0) {
			uc->led_state = 1;
		} else {
			fprintf(stderr, "Unknown force led argument: %s\n",
				value);
			break;
		}
		uc->led_state_set = 1;
		SET_OPTION(led_forced, 1);
		break;

	case UC_FORCE_LED_OFF:
		SET_OPTION(led_forced, 1);
		SET_OPTION(led_state, 0);
		break;

	case UC_FORCE_LED_ON:
		SET_OPTION(led_forced, 1);
		SET_OPTION(led_state, 1);
		break;

	case UC_FREQUENCY:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_FREQUENCY value is NULL\n");
			break;
		}
		x = strtol(value, &endptr, 10);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid frequency number: %s\n",
				value);
			break;
		}
		/* The upper bound is NTSC Amigas bus freq */
		if (x < 1 || x > 3579545) {
			fprintf(stderr, "Frequency out of bounds: %ld\n", x);
			x = UADE_DEFAULT_FREQUENCY;
		}
		SET_OPTION(frequency, x);
		break;

	case UC_GAIN:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_GAIN value is NULL\n");
			break;
		}
		SET_OPTION(gain_enable, 1);
		SET_OPTION(gain, uade_convert_to_double(value, 1.0, 0.0, 128.0, "gain"));
		break;

	case UC_HEADPHONES:
		SET_OPTION(headphones, 1);
		break;

	case UC_HEADPHONES2:
		SET_OPTION(headphones2, 1);
		break;

	case UC_IGNORE_PLAYER_CHECK:
		SET_OPTION(ignore_player_check, 1);
		break;

	case UC_RESAMPLER:
		if (value == NULL) {
			fprintf(stderr, "uade.conf: No resampler given.\n");
			break;
		}
		uc->resampler = strdup(value);
		if (uc->resampler != NULL) {
			uc->resampler_set = 1;
		} else {
			fprintf(stderr,	"uade.conf: no memory for resampler.\n");
		}
		break;

	case UC_NO_EP_END:
		SET_OPTION(no_ep_end, 1);
		break;

	case UC_NO_FILTER:
		SET_OPTION(no_filter, 1);
		break;

	case UC_NO_HEADPHONES:
		SET_OPTION(headphones, 0);
		SET_OPTION(headphones2, 0);
		break;

	case UC_NO_PANNING:
		SET_OPTION(panning_enable, 0);
		break;

	case UC_NO_POSTPROCESSING:
		SET_OPTION(no_postprocessing, 1);
		break;

	case UC_NTSC:
		SET_OPTION(use_ntsc, 1);
		break;

	case UC_ONE_SUBSONG:
		SET_OPTION(one_subsong, 1);
		break;

	case UC_PAL:
		SET_OPTION(use_ntsc, 0);
		break;

	case UC_PANNING_VALUE:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_PANNING_VALUE is NULL\n");
			break;
		}
		SET_OPTION(panning_enable, 1);
		SET_OPTION(panning, uade_convert_to_double(value, 0.0, 0.0, 2.0, "panning"));
		break;

	case UC_PLAYER_FILE:
		handle_config_path(&uc->player_file, &uc->player_file_set, value);
		break;

	case UC_SCORE_FILE:
		handle_config_path(&uc->score_file, &uc->score_file_set, value);
		break;

	case UC_SILENCE_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr,
				"uade: UC_SILENCE_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_silence_timeout(uc, value);
		break;

	case UC_SPEED_HACK:
		SET_OPTION(speed_hack, 1);
		break;

	case UC_SUBSONG_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr,
				"uade: UC_SUBSONG_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_subsong_timeout(uc, value);
		break;

	case UC_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_timeout(uc, value);
		break;

	case UC_UADECORE_FILE:
		handle_config_path(&uc->uadecore_file, &uc->uadecore_file_set, value);
		break;

	case UC_UAE_CONFIG_FILE:
		handle_config_path(&uc->uae_config_file, &uc->uae_config_file_set, value);
		break;

	case UC_USE_TEXT_SCOPE:
		SET_OPTION(use_text_scope, 1);
		break;

	case UC_VERBOSE:
		SET_OPTION(verbose, 1);
		break;

	default:
		fprintf(stderr, "uade_config_set_option(): unknown enum: %d\n",
			opt);
		exit(1);
	}
}

int uade_config_toggle_boolean(struct uade_config *uc, enum uade_option opt)
{
	if (opt == UC_VERBOSE) {
		uc->verbose ^= 1;
		return uc->verbose;
	} else if (opt == UC_FORCE_LED) {
		uade_config_set_option(uc, UC_FORCE_LED,
				       uc->led_state ? "off" : "on");
		return uc->led_state;
	}
	return -1;
}

void uade_set_options_from_ep_attributes(struct uade_state *state)
{
	struct eagleplayer *ep = state->song.info.detectioninfo.ep;

	/* Look at flags and set uade config options accordingly */
	if (uade_set_config_options_from_flags(state, ep->flags))
		uade_warning("uade_set_ep_attributes failed with setting config options from flags\n");

	if (set_options_from_attributes(state, NULL, 0, ep->attributelist))
		uade_warning("uade_set_ep_attributes failed with setting config options from eagleplayer attributes\n");
}

static int uade_set_silence_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid silence timeout value: %s\n", value);
		return -1;
	}
	uc->silence_timeout = t;
	uc->silence_timeout_set = 1;
	return 0;
}

static int uade_set_subsong_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid subsong timeout value: %s\n", value);
		return -1;
	}
	uc->subsong_timeout = t;
	uc->subsong_timeout_set = 1;
	return 0;
}

static int uade_set_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid timeout value: %s\n", value);
		return -1;
	}
	uc->timeout = t;
	uc->timeout_set = 1;
	return 0;
}
