/* 
 * Loads contents of 'eagleplayer.conf'. The file formats are
 * specified in doc/uade123.1.
 *
 * Copyright 2005-2007 Heikki Orsila <heikki.orsila@iki.fi>
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any you
 * want in your projects.
 */

#include <uade/uade.h>
#include <uade/uadestate.h>
#include <uade/unixsupport.h>
#include <uade/unixatomic.h>
#include <uade/ossupport.h>
#include <uade/amifilemagic.h>
#include "support.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define OPTION_DELIMITER ","

/* Table for associating eagleplayer.conf, song.conf and uade.conf options
 * together.
 */
static const struct epconfattr boolean_options[] = {
	{.s = "a500",               .e = ES_A500,                .o = UC_FILTER_TYPE, .c = "a500"},
	{.s = "a1200",              .e = ES_A1200,               .o = UC_FILTER_TYPE, .c = "a1200"},
	{.s = "always_ends",        .e = ES_ALWAYS_ENDS,         .o = UC_DISABLE_TIMEOUTS},
	{.s = "broken_song_end",    .e = ES_BROKEN_SONG_END,     .o = UC_NO_EP_END},
	{.s = "detect_format_by_content", .e = ES_CONTENT_DETECTION,   .o = UC_CONTENT_DETECTION},
	{.s = "ignore_player_check",.e = ES_IGNORE_PLAYER_CHECK, .o = UC_IGNORE_PLAYER_CHECK},
	{.s = "led_off",            .e = ES_LED_OFF,             .o = UC_FORCE_LED_OFF},
	{.s = "led_on",             .e = ES_LED_ON,              .o = UC_FORCE_LED_ON},
	{.s = "never_ends",         .e = ES_NEVER_ENDS,          .o = 0},
	{.s = "no_ep_end_detect",   .e = ES_BROKEN_SONG_END,     .o = UC_NO_EP_END},
	{.s = "no_filter",          .e = ES_NO_FILTER,           .o = UC_NO_FILTER},
	{.s = "no_headphones",      .e = ES_NO_HEADPHONES,       .o = UC_NO_HEADPHONES},
	{.s = "no_panning",         .e = ES_NO_PANNING,          .o = UC_NO_PANNING},
	{.s = "no_postprocessing",  .e = ES_NO_POSTPROCESSING,   .o = UC_NO_POSTPROCESSING},
	{.s = "ntsc",               .e = ES_NTSC,                .o = UC_NTSC},
	{.s = "one_subsong",        .e = ES_ONE_SUBSONG,         .o = UC_ONE_SUBSONG},
	{.s = "pal",                .e = ES_PAL,                 .o = UC_PAL},
	{.s = "reject",             .e = ES_REJECT,              .o = 0},
	{.s = "speed_hack",         .e = ES_SPEED_HACK,          .o = UC_SPEED_HACK},
	{.s = NULL}
};


/*
 * Variables for eagleplayer.conf and song.conf. ".s" must be presented with
 * shortest prefix first. If there is "ab" and "abc", "ab" must be the first.
 */
static const struct epconfattr string_options[] = {
	{.s = "epopt",           .e = ES_EP_OPTION},
	{.s = "gain",            .e = ES_GAIN},
	{.s = "interpolator",    .e = ES_RESAMPLER},
	{.s = "panning",         .e = ES_PANNING},
	{.s = "player",          .e = ES_PLAYER},
	{.s = "resampler",       .e = ES_RESAMPLER},
	{.s = "silence_timeout", .e = ES_SILENCE_TIMEOUT},
	{.s = "subsong_timeout", .e = ES_SUBSONG_TIMEOUT},
	{.s = "subsongs",        .e = ES_SUBSONGS},
	{.s = "timeout",         .e = ES_TIMEOUT},
	{.s = NULL}
};


static int ufcompare(const void *a, const void *b);
static struct eagleplayerstore *read_eagleplayer_conf(const char *filename);


static struct eagleplayer *get_eagleplayer(const char *extension,
					   struct eagleplayerstore *playerstore);

static struct eagleplayerstore *try_playerstore_path(const char *path)
{
	char formatsfile[PATH_MAX];
	snprintf(formatsfile, sizeof formatsfile, "%s/eagleplayer.conf", path);
	return read_eagleplayer_conf(formatsfile);
}

static int load_playerstore(struct uade_state *state)
{
	const char *basedir = state->config.basedir.name;
	if (state->playerstore == NULL)
		state->playerstore = try_playerstore_path(basedir);

	if (state->playerstore == NULL)
		uade_warning("Tried to load eagleplayer.conf from %s, "
			     "but failed\n", basedir);

	return state->playerstore != NULL;
}

void uade_free_playerstore(struct eagleplayerstore *ps)
{
	size_t i;
	struct uade_attribute *node;

	if (ps == NULL)
		return;
	for (i = 0; i < ps->nplayers; i++) {
		struct eagleplayer *p = &ps->players[i];
		size_t j;
		free_and_null(p->playername);
		for (j = 0; j < p->nextensions; j++) {
			if (p->extensions[j] != NULL)
				free_and_null(p->extensions[j]);
		}

		node = p->attributelist;
		while (node != NULL) {
			struct uade_attribute *nextnode = node->next;
			free_and_null(node->s);
			free(node);
			node = nextnode;
		}

		free_and_null(p->extensions);
	}
	free_and_null(ps->players);
	free_and_null(ps->map);
	memset(ps, 0, sizeof ps[0]);
	free_and_null(ps);
}

static void try_extension(struct uade_detection_info *detectioninfo,
			  const char *ext, struct uade_state *state)
{
	if (strlen(ext) >= UADE_MAX_EXT_LEN)
		return;
	detectioninfo->ep = get_eagleplayer(ext, state->playerstore);
	if (detectioninfo->ep != NULL)
		strlcpy(detectioninfo->ext, ext, sizeof detectioninfo->ext);
}

static void custom_check(struct uade_detection_info *detectioninfo)
{
	if (detectioninfo->ep != NULL)
		detectioninfo->custom = (strcmp(detectioninfo->ep->playername,
						"custom") == 0);
}

int uade_analyze_eagleplayer(struct uade_detection_info *detectioninfo,
			     const void *ibuf, size_t ibytes,
			     const char *fname, size_t fsize,
			     struct uade_state *state)
{
	char *prefix;
	char *postfix;
	char *t;
	unsigned char buf[8192];
	size_t bufsize;

	memset(detectioninfo, 0, sizeof *detectioninfo);

	if (fname == NULL)
		fname = "";

	if (ibytes == 0)
		return -1;
	if (ibytes > sizeof buf)
		bufsize = sizeof buf;
	else
		bufsize = ibytes;
	memcpy(buf, ibuf, bufsize);
	memset(&buf[bufsize], 0, sizeof buf - bufsize);

	uade_filemagic(buf, bufsize, detectioninfo->ext, fsize, fname,
		       state->config.verbose);

	if (strcmp(detectioninfo->ext, "reject") == 0)
		return -1;

	if (detectioninfo->ext[0] != 0 && state->config.verbose)
		fprintf(stderr, "Content recognized: %s (%s)\n",
			detectioninfo->ext, fname);

	if (strcmp(detectioninfo->ext, "packed") == 0)
		return -1;

	if (!load_playerstore(state))
		return -1;

	/*
	 * If filemagic found a match, we'll use player plugins associated with
	 * that extension
	 */
	if (detectioninfo->ext[0]) {
		detectioninfo->ep = get_eagleplayer(detectioninfo->ext,
						    state->playerstore);
		if (detectioninfo->ep != NULL) {
			custom_check(detectioninfo);
			detectioninfo->content = 1;
			return 0;
		}
		uade_warning("%s not in eagleplayer.conf\n",
			     detectioninfo->ext);
	}
	detectioninfo->ext[0] = 0;

	if (strlen(fname) == 0)
		return -1;

	/* First do filename detection (we'll later do content detection) */
	t = uade_xbasename(fname);

	if (strlcpy((char *) buf, t, sizeof buf) >= sizeof buf)
		return -1;

	t = strchr((char *) buf, '.');
	if (t == NULL)
		return -1;

	*t = 0;
	prefix = (char *) buf;
	try_extension(detectioninfo, prefix, state);

	if (detectioninfo->ep == NULL) {
		/* Try postfix */
		t = uade_xbasename(fname);
		strlcpy((char *) buf, t, sizeof buf);
		postfix = strrchr((char *) buf, '.') + 1; /* postfix != NULL */
		try_extension(detectioninfo, postfix, state);
	}

	uade_debug(state, "Format detection by filename: %s\n", fname);

	custom_check(detectioninfo);

	return detectioninfo->ep != NULL ? 0 : -1;
}

/* Returns 1 on success, 0 otherwise. */
static int store_attribute_into_list(struct uade_attribute **attributelist,
				     int es_flag, const char *value)
{
	struct uade_attribute *a = malloc(sizeof a[0]);
	if (a == NULL) {
		uade_warning("No memory for song attribute.\n");
		return 0;
	}
	*a = (struct uade_attribute) {.s = strdup(value),
				      .flag = es_flag};
	if (a->s == NULL) {
		free_and_poison(a);
		uade_warning("Out of memory allocating string option for "
			     "song\n");
		return 0;
	}

	a->next = *attributelist;
	*attributelist = a;
	return 1;
}

int uade_set_config_options_from_flags(struct uade_state *state, int flags)
{
	size_t i;
	for (i = 0; boolean_options[i].s != NULL; i++) {
		if (boolean_options[i].o == 0)
			continue;
		if ((flags & boolean_options[i].e) == 0)
			continue;
		uade_debug(state, "Boolean option %s set.\n",
			   boolean_options[i].s);
		uade_config_set_option(&state->config, boolean_options[i].o,
				       boolean_options[i].c);
	}

	if (flags & ES_NEVER_ENDS) {
		uade_warning("ES_NEVER_ENDS is not implemented.\n");
		return -1;
	}
	if (flags & ES_REJECT) {
		uade_warning("ES_REJECT is not implemented.\n");
		return -1;
	}

	return 0;
}

int uade_parse_attribute_from_string(struct uade_attribute **attributelist,
				     int *flags, char *item, size_t lineno)
{
	size_t i;
	for (i = 0; boolean_options[i].s != NULL; i++) {
		if (strcasecmp(item, boolean_options[i].s) == 0) {
			*flags |= boolean_options[i].e;
			return 1;
		}
	}

	for (i = 0; string_options[i].s != NULL; i++) {
		char *value;
		size_t len = strlen(string_options[i].s);
		if (strncasecmp(item, string_options[i].s, len) != 0)
			continue;
		if (item[len] != '=') {
			fprintf(stderr, "Invalid song item: %s on line %zu\n",
				item, lineno);
			return 0;
		}
		value = item + len + 1;
		return store_attribute_into_list(attributelist,
						 string_options[i].e, value);
	}

	return 0;
}

/* Compare function for bsearch() and qsort() to sort eagleplayers with
   respect to name extension. */
static int ufcompare(const void *a, const void *b)
{
	const struct eagleplayermap *ua = a;
	const struct eagleplayermap *ub = b;

	return strcasecmp(ua->extension, ub->extension);
}

static struct eagleplayer *get_eagleplayer(const char *extension,
					   struct eagleplayerstore *ps)
{
	struct eagleplayermap *uf = ps->map;
	struct eagleplayermap *f;
	struct eagleplayermap key = {.extension = (char *)extension };

	f = bsearch(&key, uf, ps->nextensions, sizeof(uf[0]), ufcompare);
	if (f == NULL)
		return NULL;

	return f->player;
}

/*
 * Read eagleplayer.conf. XXX: Clean up this function.
 */
static struct eagleplayerstore *read_eagleplayer_conf(const char *filename)
{
	FILE *f;
	struct eagleplayer *p;
	size_t allocated;
	size_t lineno = 0;
	struct eagleplayerstore *ps = NULL;
	size_t exti;
	size_t i;
	size_t j;
	int epwarning;
	int parse_error = 0;

	f = fopen(filename, "r");
	if (f == NULL)
		goto error;

	ps = calloc(1, sizeof ps[0]);
	if (ps == NULL) {
		uade_warning("No memory for ps.");
		goto error;
	}

	allocated = 16;
	ps->players = malloc(allocated * sizeof(ps->players[0]));
	if (ps->players == NULL) {
		uade_warning("No memory for ps->players.\n");
		goto error;
	}

	while (1) {
		char **items;
		size_t nitems;

		items = uade_read_and_split_lines(&nitems, &lineno, f,
						  UADE_WS_DELIMITERS);
		if (items == NULL)
			break;

		assert(nitems > 0);

		if (ps->nplayers == allocated) {
			void *players;
			allocated *= 2;
			players = realloc(ps->players,
					  allocated * sizeof(ps->players[0]));
			if (players == NULL) {
				uade_warning("No memory for ps->players.");
				goto parse_error;
			}
			ps->players = players;
		}

		p = &ps->players[ps->nplayers];
		ps->nplayers++;

		memset(p, 0, sizeof p[0]);

		p->playername = strdup(items[0]);
		if (p->playername == NULL) {
			uade_warning("No memory for playername.\n");
			goto parse_error;
		}

		for (i = 1; i < nitems; i++) {
			if (strncasecmp(items[i], "prefixes=", 9) == 0) {
				char prefixes[UADE_LINESIZE];
				char *prefixstart = items[i] + 9;
				char *sp;
				char *s;
				size_t pos;

				assert(p->nextensions == 0 &&
				       p->extensions == NULL);

				p->nextensions = 0;
				strlcpy(prefixes, prefixstart, sizeof prefixes);
				sp = prefixes;
				while ((s = strsep(&sp, OPTION_DELIMITER)) != NULL) {
					if (*s == 0)
						continue;
					p->nextensions++;
				}

				p->extensions = calloc(p->nextensions + 1,
						       sizeof p->extensions[0]);
				if (p->extensions == NULL) {
					uade_warning("No memory for "
						     "extensions.\n");
					goto parse_error;
				}

				pos = 0;
				sp = prefixstart;
				while (1) {
					s = strsep(&sp, OPTION_DELIMITER);
					if (s == NULL)
						break;
					if (*s == 0)
						continue;

					p->extensions[pos] = strdup(s);
					if (s == NULL) {
						uade_warning("No memory for "
							     "extension/"
							     "prefix.\n");
						goto parse_error;
					}
					pos++;
				}
				p->extensions[pos] = NULL;
				assert(pos == p->nextensions);

				continue;
			}

			if (strncasecmp(items[i], "comment:", 7) == 0)
				break;

			if (!uade_parse_attribute_from_string(&p->attributelist,
							     &p->flags,
							     items[i],
							     lineno))
				fprintf(stderr, "Unrecognized option: %s\n",
					items[i]);
		}
		goto parse_ok;

	parse_error:
		parse_error = 1;
	parse_ok:
		for (i = 0; items[i] != NULL; i++)
			free_and_null(items[i]);
		free_and_null(items);

		if (parse_error)
			goto error;
	}

	fclose_and_null(f);

	if (ps->nplayers == 0)
		goto error;

	for (i = 0; i < ps->nplayers; i++)
		ps->nextensions += ps->players[i].nextensions;

	ps->map = malloc(sizeof(ps->map[0]) * ps->nextensions);
	if (ps->map == NULL) {
		uade_warning("No memory for extension map.");
		goto error;
	}

	exti = 0;
	epwarning = 0;
	for (i = 0; i < ps->nplayers; i++) {
		p = &ps->players[i];
		if (p->nextensions == 0) {
			if (epwarning)
				continue;
			uade_warning(
				"%s eagleplayer lacks prefixes "
				"in eagleplayer.conf, which makes it unusable "
				"for any kind of file type detection. If you "
				"don't want name based file type detection for "
				"a particular format, use content_detection "
				"option for the line in eagleplayer.conf.\n",
				ps->players[i].playername);
			epwarning = 1;
			continue;
		}
		for (j = 0; j < p->nextensions; j++) {
			assert(exti < ps->nextensions);
			ps->map[exti].player = p;
			ps->map[exti].extension = p->extensions[j];
			exti++;
		}
	}

	assert(exti == ps->nextensions);

	/* Make the extension map bsearch() ready */
	qsort(ps->map, ps->nextensions, sizeof(ps->map[0]), ufcompare);

	return ps;

 error:
	uade_free_playerstore(ps);
	if (f != NULL)
		fclose_and_null(f);
	return NULL;
}
