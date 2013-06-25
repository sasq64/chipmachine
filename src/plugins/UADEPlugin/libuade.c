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


#include "uadeconstants.h"
#include "songinfo.h"
#include "sysincludes.h"

#include "uadecontrol.h"
#include "ossupport.h"
#include "uadeconfig.h"
#include "uadeconf.h"
#include "sysincludes.h"
#include "songdb.h"
#include "uadestate.h"
#include "uadesimple.h"
//#include "playloop.h"
//#include "audio.h"
#include "amigafilter.h"

#include "Fifo.h"

//#include "../../log.h"

int uade_song_end_trigger;

static char uadename[PATH_MAX];

static void cleanup(void);
void init_uade();
int play_song(const char *name);
int play();
int exit_song();


//struct play_state {
	char configname[PATH_MAX] = "";
	char playername[PATH_MAX] = "";
	char scorename[PATH_MAX] = "";
	char uadeconfname[PATH_MAX];
	struct uade_state state;
	FILE *playerfile;
 //};


	uint16_t *sm;
  int i;
  uint32_t *u32ptr;

  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;

  uint8_t sampledata[UADE_MAX_MESSAGE_SIZE];
  int left = 0;
  int what_was_left = 0;

  int subsong_end = 0;
  //int next_song = 0;
  int ret;
  int tailbytes = 0;
  int playbytes;
  char *reason;

  int64_t subsong_bytes = 0;

  const int framesize = UADE_BYTES_PER_SAMPLE * UADE_CHANNELS;
  int bytes_per_second;

  enum uade_control_state controlstate;

  struct uade_ipc *ipc;
  struct uade_song *us;
  struct uade_effect *ue;
  struct uade_config *uc;


/*
int main(int argc, char *argv[])
{

	init_uade();

	audio_init(state.config.frequency, state.config.buffer_time);

	play_song(argv[1]);

	short samples [ 4096 ];

	while(1) {
		int rc = get_samples(samples, 8192);
		if(rc <= 0)
			break;
		if (!audio_play(samples, rc)) {
			fprintf(stderr, "\nlibao error detected.\n");
			return 0;
		}
	}
	exit_song();
} */

int get_samples(uint8_t *target, int bytes) {

	while(1) {
		int rc = play();
		if(rc == 1)
			return 0;
		else if(rc < 0)
			return rc;
		if(filled() >= bytes) {
			getBytes(target, bytes);
			return bytes;
		}
	}
}

void init_uade()
{
	int ret;
	int uadeconf_loaded;
	int songindex;

	memset(&state, 0, sizeof state);
	uadeconf_loaded = uade_load_initial_config(uadeconfname,
											   sizeof uadeconfname,
											   &state.config, NULL);

	state.config.verbose = 1;
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

	initFifo(128*1024);

	uade_spawn(&state, uadename, configname);

	return;
}

int play_song(const char *name) {
	char modulename[PATH_MAX];
	char songname[PATH_MAX];

	strlcpy(modulename, name, sizeof modulename);

	state.song = NULL;
	state.ep = NULL;

	if (!uade_is_our_file(modulename, 0, &state)) {
		fprintf(stderr, "Unknown format: %s\n", modulename);
		return 0;
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
	   // goto cleanup;
	}

	/* If no modulename given, try the playername as it can be a custom
	   song */
	strlcpy(songname, modulename[0] ? modulename : playername,
			sizeof songname);

	if (!uade_alloc_song(&state, songname)) {
		fprintf(stderr, "Can not read %s: %s\n", songname,
				strerror(errno));
		return 0;
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
		return 0;
	}
	fclose(playerfile);

	debug(state.config.verbose, "Player: %s\n", playername);

	fprintf(stderr, "Song: %s (%zd bytes)\n",
			state.song->module_filename, state.song->bufsize);

	int ret = uade_song_initialization(scorename, playername, modulename,
								   &state);
	if (ret) {
		if (ret == UADECORE_INIT_ERROR) {
			uade_unalloc_song(&state);
			return 0;

		} else if (ret == UADECORE_CANT_PLAY) {
			debug(state.config.verbose, "Uadecore refuses to play the song.\n");
			uade_unalloc_song(&state);
			return 0;
		}

		fprintf(stderr, "Unknown error from uade_song_initialization()\n");
		return 0;
	}

	fprintf(stderr, "Setting up song\n");

	uade_effect_reset_internals();


	left = 0;
	what_was_left = 0;

	subsong_end = 0;
	//next_song = 0;
	tailbytes = 0;

  	subsong_bytes = 0;
	bytes_per_second = UADE_BYTES_PER_FRAME * state.config.frequency;
	controlstate = UADE_S_STATE;

	ipc = &state.ipc;
	us = state.song;
	ue = &state.effects;
	uc = &state.config;

	return 1;
}

static int wait_token() {
	do
	{
		int ret = uade_receive_message(um, sizeof(space), ipc);
		if(ret < 0)
		{
			fprintf(stderr, "\nCan not receive events (TOKEN) from uade.\n");
			return 0;
		}
		if (ret == 0)
		{
			fprintf(stderr, "\nEnd of input after reboot.\n");
			return 0;
		}
	} while (um->msgtype != UADE_COMMAND_TOKEN);
	return 1;
}

int exit_song() {

	fprintf(stderr, "Ending UADE song");

	uade_unalloc_song(&state);
	uade_song_end_trigger = 0;

	if(controlstate == UADE_R_STATE)
		wait_token();

	fprintf(stderr, "close2\n");
	if(uade_send_short_message(UADE_COMMAND_REBOOT, ipc))
	{
		fprintf(stderr, "\nCan not send reboot\n");
		return;
	}
	fprintf(stderr, "close3\n");
    if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc))
	{
		fprintf(stderr, "\nCan not send token\n");
		return;
	}
 	fprintf(stderr, "close4\n");

	wait_token();

	controlstate = UADE_S_STATE;


	/*
  do {
	int ret = uade_receive_message(um, sizeof(space), ipc);
	if (ret < 0) {
	  fprintf(stderr, "\nCan not receive events (TOKEN) from uade.\n");
	  return 0;
	}
	if (ret == 0) {
	  fprintf(stderr, "\nEnd of input after reboot.\n");
	  return 0;
	}
  } while (um->msgtype != UADE_COMMAND_TOKEN);

  tprintf("\n"); */
  return 0;
}

//void cleanup(void) {
	//audio_close();
//}

static int new_subsong = -1;
void set_song(int song) {
	new_subsong = song;
}

// Return 0 = OK, -1 = ERROR, 1 = SONG ENDED
int play() {

	int next_song = 0;

	if (controlstate == UADE_S_STATE) {

	  if (subsong_end && uade_song_end_trigger == 0) {

		if (uc->one_subsong == 0 && us->cur_subsong != -1 && us->max_subsong != -1) {

		  us->cur_subsong++;

		  if (us->cur_subsong > us->max_subsong) {
			uade_song_end_trigger = 1;
		  } else {
			subsong_end = 0;
			subsong_bytes = 0;

			uade_change_subsong(&state);

			fprintf(stderr, "\nChanging to subsong %d from range [%d, %d]\n", us->cur_subsong, us->min_subsong, us->max_subsong);
		  }
		} else {
		  uade_song_end_trigger = 1;
		}
	  }


		if(new_subsong >= 0) {
			state.song->cur_subsong = state.song->min_subsong + new_subsong;
			uade_change_subsong(&state);
			new_subsong = -1;
		}

		if(state.config.no_filter_set) {
			uade_send_filter_command(&state);
			state.config.no_filter_set = 0;
		}

		if(state.config.panning_enable_set || state.config.panning_set) {
			uade_set_effects(&state);
			state.config.no_filter_set = 0;
			state.config.panning_set = 0;
			state.config.panning_enable_set = 0;
		}


	  if (uade_song_end_trigger) {
		next_song = 1;
		if (uade_send_short_message(UADE_COMMAND_REBOOT, ipc)) {
		  fprintf(stderr, "\nCan not send reboot\n");
		  return -1;
		}
		goto sendtoken;
	  }

	  left = uade_read_request(ipc);

	sendtoken:
	  if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
		fprintf(stderr, "\nCan not send token\n");
		return -1;
	  }

	  controlstate = UADE_R_STATE;

	  if (what_was_left) {
		if (subsong_end) {
		  /* We can only rely on 'tailbytes' amount which was determined
			 earlier when UADE_REPLY_SONG_END happened */
		  playbytes = tailbytes;
		  tailbytes = 0;
		} else {
		  playbytes = what_was_left;
		}

		us->out_bytes += playbytes;
		subsong_bytes += playbytes;

		uade_effect_run(ue, (int16_t *) sampledata, playbytes / framesize);

		//printf("Playing %d bytes\n", playbytes);
		putBytes(sampledata, playbytes);
/*
		if (!audio_play(sampledata, playbytes)) {
		  fprintf(stderr, "\nlibao error detected.\n");
		  return 0;
		}
*/
		/* FIX ME */
		if (uc->timeout != -1 && uc->use_timeouts) {
		  if (uade_song_end_trigger == 0) {
			if (us->out_bytes / bytes_per_second >= uc->timeout) {
			  fprintf(stderr, "\nSong end (timeout %ds)\n", uc->timeout);
			  uade_song_end_trigger = 1;
			}
		  }
		}

		if (uc->subsong_timeout != -1 && uc->use_timeouts) {
		  if (subsong_end == 0 && uade_song_end_trigger == 0) {
			if (subsong_bytes / bytes_per_second >= uc->subsong_timeout) {
			  fprintf(stderr, "\nSong end (subsong timeout %ds)\n", uc->subsong_timeout);
			  subsong_end = 1;
			}
		  }
		}
	  }

	} else {

	  /* receive state */
	  if (uade_receive_message(um, sizeof(space), ipc) <= 0) {
		fprintf(stderr, "\nCan not receive events from uade\n");
		return -1;
	  }
	  
	  switch (um->msgtype) {

	  case UADE_COMMAND_TOKEN:
		controlstate = UADE_S_STATE;
		break;

	  case UADE_REPLY_DATA:
		sm = (uint16_t *) um->data;
		for (i = 0; i < um->size; i += 2) {
		  *sm = ntohs(*sm);
		  sm++;
		}

		assert (left == um->size);
		assert (sizeof sampledata >= um->size);

		memcpy(sampledata, um->data, um->size);

		what_was_left = left;
		left = 0;
		break;
		
	  case UADE_REPLY_FORMATNAME:
		uade_check_fix_string(um, 128);
		debug(uc->verbose, "\nFormat name: %s\n", (uint8_t *) um->data);
		break;
		
	  case UADE_REPLY_MODULENAME:
		uade_check_fix_string(um, 128);
		debug(uc->verbose, "\nModule name: %s\n", (uint8_t *) um->data);
		break;

	  case UADE_REPLY_MSG:
		uade_check_fix_string(um, 128);
		debug(uc->verbose, "\nMessage: %s\n", (char *) um->data);
		break;

	  case UADE_REPLY_PLAYERNAME:
		uade_check_fix_string(um, 128);
		debug(uc->verbose, "\nPlayer name: %s\n", (uint8_t *) um->data);
		break;

	  case UADE_REPLY_SONG_END:
		if (um->size < 9) {
		  fprintf(stderr, "\nInvalid song end reply\n");
		  exit(-1);
		}
		tailbytes = ntohl(((uint32_t *) um->data)[0]);
		/* next ntohl() is only there for a principle. it is not useful */
		if (ntohl(((uint32_t *) um->data)[1]) == 0) {
		  /* normal happy song end. go to next subsong if any */
		  subsong_end = 1;
		} else {
		  /* unhappy song end (error in the 68k side). skip to next song
			 ignoring possible subsongs */
		  uade_song_end_trigger = 1;
		}
		i = 0;
		reason = (char *) &um->data[8];
		while (reason[i] && i < (um->size - 8))
		  i++;
		if (reason[i] != 0 || (i != (um->size - 9))) {
		  fprintf(stderr, "\nbroken reason string with song end notice\n");
		  exit(-1);
		}
		fprintf(stderr, "\nSong end (%s)\n", reason);
		break;

	  case UADE_REPLY_SUBSONG_INFO:
		if (um->size != 12) {
		  fprintf(stderr, "\nsubsong info: too short a message\n");
		  exit(-1);
		}

		u32ptr = (uint32_t *) um->data;
		us->min_subsong = ntohl(u32ptr[0]);
		us->max_subsong = ntohl(u32ptr[1]);
		us->cur_subsong = ntohl(u32ptr[2]);

		debug(uc->verbose, "\nsubsong: %d from range [%d, %d]\n", us->cur_subsong, us->min_subsong, us->max_subsong);

		if (!(-1 <= us->min_subsong && us->min_subsong <= us->cur_subsong && us->cur_subsong <= us->max_subsong)) {
		  int tempmin = us->min_subsong, tempmax = us->max_subsong;
		  fprintf(stderr, "\nThe player is broken. Subsong info does not match.\n");
		  us->min_subsong = tempmin <= tempmax ? tempmin : tempmax;
		  us->max_subsong = tempmax >= tempmin ? tempmax : tempmin;
		  if (us->cur_subsong > us->max_subsong)
			us->max_subsong = us->cur_subsong;
		  else if (us->cur_subsong < us->min_subsong)
			us->min_subsong = us->cur_subsong;
		}

		if ((us->max_subsong - us->min_subsong) != 0)
		  fprintf(stderr, "\nThere are %d subsongs in range [%d, %d].\n", 1 + us->max_subsong - us->min_subsong, us->min_subsong, us->max_subsong);
		break;
		
	  default:
		fprintf(stderr, "\nExpected sound data. got %u.\n", (unsigned int) um->msgtype);
		return -1;
	  }
	}

	return next_song;
}
