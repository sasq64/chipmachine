/* uadesimple - a very very stupid simple command line frontend for uadecore.

   Copyright (C) 2007-2010 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include "audio.h"

#include <uade/uade.h>

#include <assert.h>
#include <stdlib.h>

static void cleanup(struct uade_state *state)
{
	uade_cleanup_state(state);
	audio_close();
}

static void play_loop(struct uade_state *state)
{
	char buf[4096];
	while (1) {
		ssize_t nbytes = uade_read(buf, sizeof buf, state);
		if (nbytes < 0) {
			fprintf(stderr, "Playback error.\n");
			break;
		} else if (nbytes == 0) {
			fprintf(stderr, "Song end.\n");
			break;
		}
		audio_play(buf, nbytes);
	}
}

int main(int argc, char *argv[])
{
	int i;
	const char *fname;
	const struct uade_song_info *info;
	struct uade_state *state = uade_new_state(NULL);
	int ret;
	size_t size;
	void *buf;

	if (state == NULL)
		goto error;

	if (!audio_init(uade_get_sampling_rate(state)))
		goto error;

	for (i = 1; i < argc; i++) {
		fname = argv[i];
		buf = uade_read_file(&size, fname);
		if (buf == NULL) {
			fprintf(stderr, "Can not read file %s\n", fname);
			continue;
		}

		ret = uade_play_from_buffer(NULL, buf, size, -1, state);

		free(buf);
		buf = NULL;

		if (ret < 0) {
			goto error;
		} else if (ret == 0) {
			fprintf(stderr, "Can not play %s\n", fname);
			continue;
		}

		info = uade_get_song_info(state);
		if (info->formatname[0])
			printf("Format name: %s\n", info->formatname);
		if (info->modulename[0])
			printf("Module name: %s\n", info->modulename);
		if (info->playername[0])
			printf("Player name: %s\n", info->playername);
		printf("subsongs: cur %d min %d max %d\n", info->subsongs.cur,
		       info->subsongs.min, info->subsongs.max);

		play_loop(state);

		if (uade_stop(state))
			goto error;
	}
	
	cleanup(state);
	return 0;

error:
	cleanup(state);
	return 1;
}
