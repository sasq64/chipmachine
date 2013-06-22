/* uade123 - a simple command line frontend for uadecore.

   Copyright (C) 2005 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <uadecontrol.h>
#include <uadeconstants.h>
#include <songinfo.h>
#include <sysincludes.h>

#include "playloop.h"
#include "uadesimple.h"
#include "audio.h"

  uint16_t *sm;
  int i;
  uint32_t *u32ptr;

  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;

  uint8_t sampledata[UADE_MAX_MESSAGE_SIZE];
  int left = 0;
  int what_was_left = 0;

  int subsong_end = 0;
  int next_song = 0;
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


//  while (next_song == 0) {

int play(struct uade_state *state)
{

    if (controlstate == UADE_S_STATE) {

      if (subsong_end && uade_song_end_trigger == 0) {

		if (uc->one_subsong == 0 && us->cur_subsong != -1 && us->max_subsong != -1) {

		  us->cur_subsong++;

		  if (us->cur_subsong > us->max_subsong) {
		    uade_song_end_trigger = 1;
		  } else {
		    subsong_end = 0;
		    subsong_bytes = 0;

		    uade_change_subsong(state);

		    fprintf(stderr, "\nChanging to subsong %d from range [%d, %d]\n", us->cur_subsong, us->min_subsong, us->max_subsong);
		  }
		} else {
		  uade_song_end_trigger = 1;
		}
      }

      if (uade_song_end_trigger) {
		next_song = 1;
		if (uade_send_short_message(UADE_COMMAND_REBOOT, ipc)) {
		  fprintf(stderr, "\nCan not send reboot\n");
		  return 0;
		}
		goto sendtoken;
      }

      left = uade_read_request(ipc);

    sendtoken:
      if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
		fprintf(stderr, "\nCan not send token\n");
		return 0;
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

		if (!audio_play(sampledata, playbytes)) {
		  fprintf(stderr, "\nlibao error detected.\n");
		  return 0;
		}

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
		return 0;
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
		return 0;
      }
    }
}

int play_loop(struct uade_state *state) {

	bytes_per_second = UADE_BYTES_PER_FRAME * state->config.frequency;
	controlstate = UADE_S_STATE;

  ipc = &state->ipc;
  us = state->song;
  ue = &state->effects;
  uc = &state->config;


	while (next_song == 0) {
		play(state);
	}

	return done();
}

int done() {
  do {
    ret = uade_receive_message(um, sizeof(space), ipc);
    if (ret < 0) {
      fprintf(stderr, "\nCan not receive events (TOKEN) from uade.\n");
      return 0;
    }
    if (ret == 0) {
      fprintf(stderr, "\nEnd of input after reboot.\n");
      return 0;
    }
  } while (um->msgtype != UADE_COMMAND_TOKEN);

  tprintf("\n");
  return 0;
}
