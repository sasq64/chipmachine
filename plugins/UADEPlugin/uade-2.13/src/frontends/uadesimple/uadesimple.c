/* uade123 - a very very stupid simple command line frontend for uadecore.

   Copyright (C) 2007 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "uadecontrol.h"
#include "ossupport.h"
#include "uadeconfig.h"
#include "uadeconf.h"
#include "sysincludes.h"
#include "songdb.h"
#include "uadestate.h"
#include "uadesimple.h"
#include "playloop.h"
#include "audio.h"
#include "amigafilter.h"

int uade_song_end_trigger;

static char uadename[PATH_MAX];

static void cleanup(void);


int main(int argc, char *argv[])
{
    FILE *playerfile;
    char configname[PATH_MAX] = "";
    char playername[PATH_MAX] = "";
    char scorename[PATH_MAX] = "";
    int ret;
    int uadeconf_loaded;
    char uadeconfname[PATH_MAX];
    int songindex;
    struct uade_state state;

    memset(&state, 0, sizeof state);

    uadeconf_loaded = uade_load_initial_config(uadeconfname,
					       sizeof uadeconfname,
					       &state.config, NULL);

    if (uadeconf_loaded == 0) {
	debug(state.config.verbose,
	      "Not able to load uade.conf from ~/.uade2/ or %s/.\n",
	      state.config.basedir.name);
    } else {
	debug(state.config.verbose, "Loaded configuration: %s\n", uadeconfname);
    }

    do {
	DIR *bd;
	if ((bd = opendir(state.config.basedir.name)) == NULL) {
	    fprintf(stderr, "Could not access dir %s: %s\n",
		    state.config.basedir.name, strerror(errno));
	    exit(1);
	}
	closedir(bd);

	snprintf(configname, sizeof configname, "%s/uaerc",
		 state.config.basedir.name);

	if (scorename[0] == 0)
	    snprintf(scorename, sizeof scorename, "%s/score",
		     state.config.basedir.name);

	if (uadename[0] == 0)
	    strlcpy(uadename, UADE_CONFIG_UADE_CORE, sizeof uadename);

	if (access(configname, R_OK)) {
	    fprintf(stderr, "Could not read %s: %s\n", configname,
		    strerror(errno));
	    exit(1);
	}
	if (access(scorename, R_OK)) {
	    fprintf(stderr, "Could not read %s: %s\n", scorename,
		    strerror(errno));
	    exit(1);
	}
	if (access(uadename, X_OK)) {
	    fprintf(stderr, "Could not execute %s: %s\n", uadename,
		    strerror(errno));
	    exit(1);
	}
    } while (0);

    uade_spawn(&state, uadename, configname);

    if (!audio_init(state.config.frequency, state.config.buffer_time))
	goto cleanup;

    for (songindex = 1; songindex < argc; songindex++) {
	/* modulename and songname are a bit different. modulename is the name
	   of the song from uadecore's point of view and songname is the
	   name of the song from user point of view. Sound core considers all
	   custom songs to be players (instead of modules) and therefore modulename
	   will become a zero-string with custom songs. */
	char modulename[PATH_MAX];
	char songname[PATH_MAX];

	strlcpy(modulename, argv[songindex], sizeof modulename);

	state.song = NULL;
	state.ep = NULL;

	if (!uade_is_our_file(modulename, 0, &state)) {
	    fprintf(stderr, "Unknown format: %s\n", modulename);
	    continue;
	}

	debug(state.config.verbose, "Player candidate: %s\n", state.ep->playername);

	if (strcmp(state.ep->playername, "custom") == 0) {
	    strlcpy(playername, modulename, sizeof playername);
	    modulename[0] = 0;
	} else {
	  snprintf(playername, sizeof playername, "%s/players/%s", state.config.basedir.name, state.ep->playername);
	}

	if (strlen(playername) == 0) {
	    fprintf(stderr, "Error: an empty player name given\n");
	    goto cleanup;
	}

	/* If no modulename given, try the playername as it can be a custom
	   song */
	strlcpy(songname, modulename[0] ? modulename : playername,
		sizeof songname);

	if (!uade_alloc_song(&state, songname)) {
	    fprintf(stderr, "Can not read %s: %s\n", songname,
		    strerror(errno));
	    continue;
	}

	/* The order of parameter processing is important:
	 * 0. set uade.conf options (done before this)
	 * 1. set eagleplayer attributes
	 * 2. set song attributes
	 * 3. set command line options
	 */

	if (state.ep != NULL)
	    uade_set_ep_attributes(&state);

	/* Now we have the final configuration in "uc". */
	uade_set_effects(&state);

	playerfile = fopen(playername, "r");
	if (playerfile == NULL) {
	    fprintf(stderr, "Can not find player: %s (%s)\n", playername,
		    strerror(errno));
	    uade_unalloc_song(&state);
	    continue;
	}
	fclose(playerfile);

	debug(state.config.verbose, "Player: %s\n", playername);

	fprintf(stderr, "Song: %s (%zd bytes)\n",
		state.song->module_filename, state.song->bufsize);

	ret = uade_song_initialization(scorename, playername, modulename,
				       &state);
	if (ret) {
	    if (ret == UADECORE_INIT_ERROR) {
		uade_unalloc_song(&state);
		goto cleanup;

	    } else if (ret == UADECORE_CANT_PLAY) {
		debug(state.config.verbose, "Uadecore refuses to play the song.\n");
		uade_unalloc_song(&state);
		continue;
	    }

	    fprintf(stderr, "Unknown error from uade_song_initialization()\n");
	    exit(1);
	}

	play_loop(&state);

	uade_unalloc_song(&state);

	uade_song_end_trigger = 0;
    }

    cleanup();
    return 0;

  cleanup:
    cleanup();
    return 1;
}


static void cleanup(void)
{
    audio_close();
}
