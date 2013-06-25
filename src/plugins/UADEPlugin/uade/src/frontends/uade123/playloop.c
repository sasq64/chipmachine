/* uade123 - a simple command line frontend for uadecore.

   Copyright (C) 2005-2010 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <uade/uade.h>

#include "playloop.h"
#include "uade123.h"
#include "audio.h"
#include "terminal.h"
#include "playlist.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

static void print_song_info(const struct uade_state *state,
			    enum uade_song_info_type t)
{
	char infotext[16384];
	const struct uade_song_info *info = uade_get_song_info(state);
	if (!uade_song_info(infotext, sizeof infotext, info->modulefname, t))
		tprintf("\n%s\n", infotext);
}

static void print_info(struct uade_state *state)
{
	const struct uade_song_info *info = uade_get_song_info(state);

	if (uade_info_mode) {
		printf("formatname: %s\n", info->formatname);
		printf("modulename: %s\n", info->modulename);
		printf("playername: %s\n", info->playername);
		printf("subsongs: cur %d min %d max %d\n",
		       info->subsongs.cur, info->subsongs.min,
		       info->subsongs.max);
	} else {
		int n = 1 + info->subsongs.max - info->subsongs.min;
		uade_debug(state, "Format name: %s\n", info->formatname);
		uade_debug(state, "Module name: %s\n", info->modulename);
		uade_debug(state, "Player name: %s\n", info->playername);
		if (n > 1)
			tprintf("There are %d subsongs in range [%d, %d].\n",
				n, info->subsongs.min, info->subsongs.max);
	}
}

static void print_time(struct uade_state *state)
{
	const struct uade_song_info *info = uade_get_song_info(state);
	int bytespersecond = UADE_BYTES_PER_FRAME *
		             uade_get_sampling_rate(state);
	int deciseconds = (info->subsongbytes * 10) / bytespersecond;
	if (uade_no_text_output)
		return;
	tprintf("Playing time position %d.%ds in subsong %d / %d ",
		deciseconds / 10,
		deciseconds % 10,
		info->subsongs.cur == -1 ? 0 : info->subsongs.cur,
		info->subsongs.max);
	if (info->duration > 0) {
		int ptimesecs = info->duration;
		int ptimesubsecs = ((int64_t) (info->duration * 10)) % 10;
		tprintf("(all subs %d.%ds)  \r", ptimesecs, ptimesubsecs);
	} else {
		tprintf("                   \r");
	}
	fflush(stdout);
}

static const char *effect_enabled_notice(const struct uade_state *state,
					 uade_effect_t effect)
{
	if (uade_effect_is_enabled(state, effect)) {
		if (uade_effect_is_enabled(state, UADE_EFFECT_ALLOW) == 0)
			return "ON (Remember to turn ON postprocessing!)";
		return "ON";
	}
	return "OFF";
}

int terminal_input(int *plistdir, struct uade_state *state)
{
	int filterstate;
	int newsub;
	int ret;
	const struct uade_song_info *info = uade_get_song_info(state);
	struct uade_config *config = uade_get_effective_config(state);
	const char *onoff;

	ret = read_terminal();
	switch (ret) {
	case 0:
		break;
	case '<':
		*plistdir = UADE_PLAY_PREVIOUS;
		return -1;
	case '>':
		*plistdir = UADE_PLAY_NEXT;
		return -1;

	case UADE_CURSOR_LEFT:
		uade_seek(UADE_SEEK_POSITION_RELATIVE, -10, 0, state);
		break;
	case UADE_CURSOR_RIGHT:
	case '.':
		uade_seek(UADE_SEEK_POSITION_RELATIVE, 10, 0, state);
		break;
	case UADE_CURSOR_DOWN:
		uade_seek(UADE_SEEK_POSITION_RELATIVE, -60, 0, state);
		break;
	case UADE_CURSOR_UP:
		uade_seek(UADE_SEEK_POSITION_RELATIVE, 60, 0, state);
		break;

	case 'b':
		newsub = info->subsongs.cur + 1;
		if (newsub > info->subsongs.max) {
			*plistdir = UADE_PLAY_NEXT;
			return -1;
		}
		if (uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, newsub, state))
			tprintf("\nBad subsong number: %d\n", newsub);
		break;
	case ' ':
	case 'c':
		pause_terminal();
		break;
	case 'f':
		filterstate = uade_config_toggle_boolean(config, UC_FORCE_LED);
		tprintf("\nForcing LED %s\n", filterstate ? "ON" : "OFF");
		uade_set_filter_state(state, filterstate);
		break;
	case 'g':
		uade_effect_toggle(state, UADE_EFFECT_GAIN);
		onoff = effect_enabled_notice(state, UADE_EFFECT_GAIN);
		tprintf("\nGain effect %s\n", onoff);
		break;
	case 'h':
		tprintf("\n\n");
		print_action_keys();
		tprintf("\n");
		break;
	case 'H':
		uade_effect_toggle(state, UADE_EFFECT_HEADPHONES);
		onoff = effect_enabled_notice(state, UADE_EFFECT_HEADPHONES);
		tprintf("\nHeadphones effect %s\n", onoff);
		break;
	case 'i':
		print_song_info(state, UADE_MODULE_INFO);
		break;
	case 'I':
		print_song_info(state, UADE_HEX_DUMP_INFO);
		break;
	case '\n':
		*plistdir = UADE_PLAY_NEXT;
		return -1;
	case 'p':
		uade_effect_toggle(state, UADE_EFFECT_ALLOW);
		onoff = effect_enabled_notice(state, UADE_EFFECT_ALLOW);
		tprintf("\nPostprocessing effects %s\n", onoff);
		break;
	case 'P':
		uade_effect_toggle(state, UADE_EFFECT_PAN);
		onoff = effect_enabled_notice(state, UADE_EFFECT_PAN);
		tprintf("\nPanning effect %s\n", onoff);
		break;
	case 'q':
		*plistdir = UADE_PLAY_EXIT;
		return -1;
	case 's':
		playlist_random(&uade_playlist, -1);
		tprintf("\n%s mode\n", uade_playlist.randomize ? "Shuffle" : "Normal");
		break;
	case 'v':
		uade_config_toggle_boolean(config, UC_VERBOSE);
		tprintf("\nVerbose mode %s\n",
			uade_is_verbose(state) ? "ON" : "OFF");
		break;
	case 'x':
		uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, info->subsongs.cur,
			  state);
		break;
	case 'z':
		newsub = info->subsongs.cur - 1;
		if (newsub < 0 || info->subsongs.cur <= info->subsongs.min) {
			*plistdir = UADE_PLAY_PREVIOUS;
			return -1;
		}
		if (uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, newsub, state))
			tprintf("\nBad subsong number: %d\n", newsub);
		break;
	default:
		if (isdigit(ret)) {
			newsub = ret - '0';
			if (uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, newsub,
				      state)) {
				tprintf("\nBad subsong number\n");
			}
		} else if (!isspace(ret)) {
			fprintf(stderr, "\nKey '%c' is not a valid command "
				"(hex 0x%.2x)\n", ret, ret);
		}
	}
	return 0;
}

static void handle_notification(struct uade_notification *n)
{
	switch (n->type) {
	case UADE_NOTIFICATION_MESSAGE:
		tprintf("\nAmiga message: %s\n", n->msg);
		break;
	case UADE_NOTIFICATION_SONG_END:
		tprintf("\n%s: %s\n",
			n->song_end.happy ? "song end" : "bad song end",
			n->song_end.reason);
		break;
	default:
		tprintf("\nUnknown notification type from libuade\n");
	}
}

int uade_input(int *plistdir, struct uade_state *state)
{
	struct uade_notification n;
	size_t nbytes;
	char buf[4096];

	test_and_trigger_debug(state);

	nbytes = uade_read(buf, sizeof buf, state);

	while (uade_read_notification(&n, state)) {
		handle_notification(&n);
		uade_cleanup_notification(&n);
	}

	if (nbytes < 0) {
		tprintf("\nPlayback error.\n");
		*plistdir = UADE_PLAY_FAILURE;
		return -1;
	}

	if (nbytes == 0)
		return -1;

	audio_play(buf, nbytes);
	print_time(state);
	return 0;
}

int play_loop(struct uade_state *state)
{
	int plistdir = UADE_PLAY_NEXT;
	int fds[2] = {uade_get_fd(state), -1};
	int maxfd = fds[0];
	fd_set fdset;
	int ret;

	if (terminal_fd >= 0 && actionkeys) {
		fds[1] = terminal_fd;
		if (fds[1] > maxfd)
			maxfd = fds[1];
	}

	print_info(state);
	if (uade_info_mode)
		return plistdir;

	if (uade_jump_pos > 0)
		uade_seek(UADE_SEEK_SONG_RELATIVE, uade_jump_pos, 0, state);

	while (1) {
		FD_ZERO(&fdset);
		FD_SET(fds[0], &fdset);
		if (fds[1] >= 0)
			FD_SET(fds[1], &fdset);

		ret = select(maxfd + 1, &fdset, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			uade_warning("select error: %s\n", strerror(errno));
			break;
		}
		if (ret == 0)
			continue;
		if (FD_ISSET(fds[0], &fdset) && uade_input(&plistdir, state))
			break;
		if (fds[1] >= 0 && FD_ISSET(fds[1], &fdset)) {
			if (terminal_input(&plistdir, state))
				break;
		}
	}

	tprintf("\n");

	return plistdir;
}
