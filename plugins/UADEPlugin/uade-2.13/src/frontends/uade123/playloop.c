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

#include "uadecontrol.h"
#include "uadeconstants.h"
#include "songinfo.h"
#include "sysincludes.h"
#include "songdb.h"
#include "uadeconf.h"

#include "playloop.h"
#include "uade123.h"
#include "audio.h"
#include "terminal.h"
#include "playlist.h"


static void print_song_info(struct uade_song *us, enum song_info_type t)
{
  const size_t infosize = 16384;
  char *info = malloc(infosize);
  FILE *f = uade_terminal_file ? uade_terminal_file : stdout;

  if (info == NULL)
    return;

  if (!uade_song_info(info, infosize, us->module_filename, t))
    fprintf(f, "\n%s\n", info);

  free(info);
}


int play_loop(struct uade_state *state)
{
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
  int new_sub;
  int tailbytes = 0;
  int playbytes;
  char *reason;
  int64_t skip_bytes;

  int record_playtime = 1;

  int64_t subsong_bytes = 0;
  int deciseconds;
  int jump_sub = 0;
  int have_subsong_info = 0;

  const int framesize = UADE_BYTES_PER_SAMPLE * UADE_CHANNELS;
  const int bytes_per_second = UADE_BYTES_PER_FRAME * state->config.frequency;

  enum uade_control_state controlstate = UADE_S_STATE;

  int plistdir = UADE_PLAY_NEXT;

  struct uade_ipc *ipc = &state->ipc;
  struct uade_song *us = state->song;
  struct uade_effect *ue = &state->effects;
  struct uade_config *uc = &state->config;

  uade_effect_reset_internals();

  /* Skip bytes must be a multiple of audio frame size */
  skip_bytes = uade_jump_pos * bytes_per_second;
  skip_bytes = (skip_bytes / framesize) * framesize;

  test_song_end_trigger(); /* clear a pending SIGINT */

  while (next_song == 0) {

    if (uade_terminated) {
      if (!uade_no_text_output)
	tprintf("\n");

      return UADE_PLAY_FAILURE;
    }

    if (controlstate == UADE_S_STATE) {

      if (skip_bytes == 0) {
	deciseconds = subsong_bytes * 10 / bytes_per_second;
	if (!uade_no_text_output) {
	  if (us->playtime >= 0) {
	    int ptimesecs = us->playtime / 1000;
	    int ptimesubsecs = (us->playtime / 100) % 10;
	    tprintf("Playing time position %d.%ds in subsong %d (all subs %d.%ds)  \r", deciseconds / 10, deciseconds % 10, us->cur_subsong == -1 ? 0 : us->cur_subsong, ptimesecs, ptimesubsecs);
	  } else {
	    tprintf("Playing time position %d.%ds in subsong %d                \r", deciseconds / 10, deciseconds % 10,  us->cur_subsong == -1 ? 0 : us->cur_subsong);
	  }
	  fflush(stdout);
	}
      }

      if (uc->action_keys) {
	switch ((ret = poll_terminal())) {
	case 0:
	  break;
	case '<':
	  plistdir = UADE_PLAY_PREVIOUS;
	  uade_song_end_trigger = 1;
	  record_playtime = 0;
	  break;
	case '.':
	  if (skip_bytes == 0) {
	    fprintf(stderr, "\nSkipping 10 seconds\n");
	    skip_bytes = bytes_per_second * 10;
	  }
	  break;
	case ' ':
	case 'b':
	  subsong_end = 1;
	  record_playtime = 0;
	  break;
	case 'c':
	  pause_terminal();
	  break;
	case 'f':
	  uade_set_config_option(uc, UC_FORCE_LED, uc->led_state ? "off" : "on");
	  tprintf("\nForcing LED %s\n", (uc->led_state & 1) ? "ON" : "OFF");
	  uade_send_filter_command(state);
	  break;
	case 'g':
	  uade_effect_toggle(ue, UADE_EFFECT_GAIN);
	  tprintf("\nGain effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_GAIN) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_GAIN)) ? "(Remember to turn ON postprocessing!)" : "");
	  break;
	case 'h':
	  tprintf("\n\n");
	  print_action_keys();
	  tprintf("\n");
	  break;
	case 'H':
	  uade_effect_toggle(ue, UADE_EFFECT_HEADPHONES);
	  tprintf("\nHeadphones effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_HEADPHONES) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_HEADPHONES) == 1) ? "(Remember to turn ON postprocessing!)" : "");
	  break;
	case 'i':
	  if (!uade_no_text_output)
	    print_song_info(us, UADE_MODULE_INFO);
	  break;
	case 'I':
	  if (!uade_no_text_output)
	    print_song_info(us, UADE_HEX_DUMP_INFO);
	  break;
	case '\n':
	case 'n':
	  uade_song_end_trigger = 1;
	  record_playtime = 0;
	  break;
	case 'N':
	  uade_effect_toggle(ue, UADE_EFFECT_NORMALISE);
	  tprintf("\nNormalise effect %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_NORMALISE) ? "ON" : "OFF");
	  break;
	case 'p':
	  uade_effect_toggle(ue, UADE_EFFECT_ALLOW);
	  tprintf("\nPostprocessing effects %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) ? "ON" : "OFF");
	  break;
	case 'P':
	  uade_effect_toggle(ue, UADE_EFFECT_PAN);
	  tprintf("\nPanning effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_PAN) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_PAN) == 1) ? "(Remember to turn ON postprocessing!)" : "");
	  break;

	case 'q':
	  tprintf("\n");
	  return UADE_PLAY_EXIT;

	case 's':
	  playlist_random(&uade_playlist, -1);
	  tprintf("\n%s mode\n", uade_playlist.randomize ? "Shuffle" : "Normal");
	  break;
	case 'v':
	  uc->verbose ^= 1;
	  tprintf("\nVerbose mode %s\n", uc->verbose ? "ON" : "OFF");
	  break;
	case 'x':
	  us->cur_subsong--;
	  subsong_end = 1;
	  jump_sub = 1;
	  record_playtime = 0;
	  break;
	case 'z':
	  record_playtime = 0;
	  if (us->cur_subsong == 0 ||
	      (us->min_subsong >= 0 && us->cur_subsong == us->min_subsong)) {
	    plistdir = UADE_PLAY_PREVIOUS;
	    uade_song_end_trigger = 1;
	    break;
	  }
	  new_sub = us->cur_subsong - 2;
	  if (new_sub < 0)
	    new_sub = -1;
	  if (us->min_subsong >= 0 && new_sub < us->min_subsong)
	    new_sub = us->min_subsong - 1;
	  us->cur_subsong = new_sub;
	  subsong_end = 1;
	  jump_sub = 1;
	  break;
	default:
	  if (isdigit(ret)) {
	    new_sub = ret - '0';
	    if (us->min_subsong >= 0 && new_sub < us->min_subsong) {
	      fprintf(stderr, "\ntoo low a subsong number\n");
	      break;
	    }
	    if (us->max_subsong >= 0 && new_sub > us->max_subsong) {
	      fprintf(stderr, "\ntoo high a subsong number\n");
	      break;
	    }
	    us->cur_subsong = new_sub - 1;
	    subsong_end = 1;
	    jump_sub = 1;
	    us->out_bytes = 0; /* to prevent timeout */
	    record_playtime = 0; /* to not record playtime */
	  } else if (!isspace(ret)) {
	    fprintf(stderr, "\n%c is not a valid command\n", ret);
	  }
	}
      }

      if (uade_debug_trigger == 1) {
	if (uade_send_short_message(UADE_COMMAND_ACTIVATE_DEBUGGER, ipc)) {
	  fprintf(stderr, "\nCan not active debugger\n");
	  return UADE_PLAY_FAILURE;
	}
	uade_debug_trigger = 0;
      }
      
      if (uade_info_mode && have_subsong_info) {
	/* we assume that subsong info is the last info we get */
	uade_song_end_trigger = 1;
	subsong_end = 0;
      }

      if (uade_song_end_trigger)
	record_playtime = 0;

      if (subsong_end && uade_song_end_trigger == 0) {

	if (jump_sub || (uc->one_subsong == 0 && us->cur_subsong != -1 && us->max_subsong != -1)) {

	  us->cur_subsong++;

	  jump_sub = 0;

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
	  return UADE_PLAY_FAILURE;
	}
	goto sendtoken;
      }

      left = uade_read_request(ipc);

    sendtoken:
      if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
	fprintf(stderr, "\nCan not send token\n");
	return UADE_PLAY_FAILURE;
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

	if (subsong_end == 0 && uade_song_end_trigger == 0 &&
	    uade_test_silence(um->data, playbytes, state)) {
	    
	  fprintf(stderr, "\nsilence detected (%d seconds)\n", uc->silence_timeout);
	  subsong_end = 1;
	}

	us->out_bytes += playbytes;
	subsong_bytes += playbytes;

	if (skip_bytes > 0) {
	  if (playbytes <= skip_bytes) {
	    skip_bytes -= playbytes;
	    playbytes = 0;
	  } else {
	    playbytes -= skip_bytes;
	    memmove(sampledata, sampledata + skip_bytes, playbytes);
	    skip_bytes = 0;
	  }
	}

	uade_effect_run(ue, (int16_t *) sampledata, playbytes / framesize);

	if (!audio_play(sampledata, playbytes)) {
	  fprintf(stderr, "\nlibao error detected.\n");
	  return UADE_PLAY_FAILURE;
	}

	if (uc->timeout != -1 && uc->use_timeouts) {
	  if (uade_song_end_trigger == 0) {
	    if (us->out_bytes / bytes_per_second >= uc->timeout) {
	      fprintf(stderr, "\nSong end (timeout %ds)\n", uc->timeout);
	      uade_song_end_trigger = 1;
	      record_playtime = 0;
	    }
	  }
	}

	if (uc->subsong_timeout != -1 && uc->use_timeouts) {
	  if (subsong_end == 0 && uade_song_end_trigger == 0) {
	    if (subsong_bytes / bytes_per_second >= uc->subsong_timeout) {
	      fprintf(stderr, "\nSong end (subsong timeout %ds)\n", uc->subsong_timeout);
	      subsong_end = 1;
	      record_playtime = 0;
	    }
	  }
	}
      }

    } else {

      /* receive state */

      if (uade_receive_message(um, sizeof(space), ipc) <= 0) {
	fprintf(stderr, "\nCan not receive events from uade\n");
	return UADE_PLAY_FAILURE;
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
	if (uade_info_mode)
	  tprintf("formatname: %s\n", (char *) um->data);
	break;
	
      case UADE_REPLY_MODULENAME:
	uade_check_fix_string(um, 128);
	debug(uc->verbose, "\nModule name: %s\n", (uint8_t *) um->data);
	if (uade_info_mode)
	  tprintf("modulename: %s\n", (char *) um->data);
	break;

      case UADE_REPLY_MSG:
	uade_check_fix_string(um, 128);
	debug(uc->verbose, "\nMessage: %s\n", (char *) um->data);
	break;

      case UADE_REPLY_PLAYERNAME:
	uade_check_fix_string(um, 128);
	debug(uc->verbose, "\nPlayer name: %s\n", (uint8_t *) um->data);
	if (uade_info_mode)
	  tprintf("playername: %s\n", (char *) um->data);
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

	uade_lookup_volume_normalisation(state);

	have_subsong_info = 1;

	if (uade_info_mode)
	  tprintf("subsong_info: %d %d %d (cur, min, max)\n", us->cur_subsong, us->min_subsong, us->max_subsong);
	break;
	
      default:
	fprintf(stderr, "\nExpected sound data. got %u.\n", (unsigned int) um->msgtype);
	return UADE_PLAY_FAILURE;
      }
    }
  }

  if (record_playtime && us->md5[0] != 0) {
    uint32_t playtime = (us->out_bytes * 1000) / bytes_per_second;
    uade_add_playtime(us->md5, playtime);
  }

  do {
    ret = uade_receive_message(um, sizeof(space), ipc);
    if (ret < 0) {
      fprintf(stderr, "\nCan not receive events (TOKEN) from uade.\n");
      return UADE_PLAY_FAILURE;
    }
    if (ret == 0) {
      fprintf(stderr, "\nEnd of input after reboot.\n");
      return UADE_PLAY_FAILURE;
    }
  } while (um->msgtype != UADE_COMMAND_TOKEN);

  tprintf("\n");

  return plistdir;
}
