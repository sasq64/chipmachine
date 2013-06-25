/* uadecontrol.c is a helper module to control uade core through IPC:

   Copyright (C) 2005 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <uade/uade.h>
#include <uade/uadecontrol.h>
#include <uade/ossupport.h>
#include <uade/sysincludes.h>
#include <uade/uadeconstants.h>
#include <uade/songdb.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

/* Sends a byte request and returns the number of bytes requested */
int uade_read_request(struct uade_state *state)
{
	struct uade_ipc *ipc = &state->ipc;
	uint32_t left = UADE_MAX_MESSAGE_SIZE - sizeof(struct uade_msg);
	return uade_send_u32(UADE_COMMAND_READ, left, ipc) ? 0 : left;
}

static int send_ep_options(struct uade_ep_options *eo, struct uade_ipc *ipc)
{
	size_t i;
	if (eo->s == 0)
		return 0;
	for (i = 0; i < eo->s; ) {
		char *s = &eo->o[i];
		size_t l = strlen(s) + 1;
		assert((i + l) <= eo->s);
		if (uade_send_string(UADE_COMMAND_SET_PLAYER_OPTION, s, ipc)) {
			fprintf(stderr, "Can not send eagleplayer option.\n");
			return -1;
		}
		i += l;
	}
	return 0;
}

size_t uade_prepare_filter_command(void *space, size_t maxsize,
				   const struct uade_state *state)
{
	const struct uade_config *uc = &state->config;
	int filter_type = uc->no_filter ? 0 : uc->filter_type;
	/* Note: filter (led) state is not normally forced */
	int filter_state = uc->led_forced ? (2 + (uc->led_state & 1)) : 0;
	return uade_ipc_prepare_two_u32s(space, maxsize, UADE_COMMAND_FILTER,
					 filter_type, filter_state);
}

void uade_send_filter_command(struct uade_state *state)
{
	char space[UADE_MAX_MESSAGE_SIZE];
	if (!uade_prepare_filter_command(space, sizeof space, state)) {
		uade_warning("Too small a buffer for filter command\n");
		return;
	}
	if (uade_send_message((struct uade_msg *) space, &state->ipc))
		uade_warning("Can not setup filters\n");
}

static void send_resampling_command(struct uade_ipc *ipc,
				    struct uade_config *uadeconf)
{
	char *mode = uadeconf->resampler;
	if (mode == NULL)
		return;
	if (strlen(mode) == 0) {
		fprintf(stderr, "Resampling mode may not be empty.\n");
		exit(1);
	}
	if (uade_send_string(UADE_COMMAND_SET_RESAMPLING_MODE, mode, ipc))
		uade_warning("Can not set resampling mode\n");
}

void uade_subsong_control(int subsong, int command, struct uade_ipc *ipc)
{
	assert(subsong >= 0 && subsong < 256);
	if (uade_send_u32(command, (uint32_t) subsong, ipc) < 0)
		uade_warning("Could not change subsong\n");
}

int uade_song_initialization(struct uade_file *player, struct uade_file *module,
			     struct uade_state *state)
{
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *)space;
	struct uade_ipc *ipc = &state->ipc;
	struct uade_config *uc = &state->config;
	struct uade_song_state *us = &state->song;

	if (uade_send_string(UADE_COMMAND_SCORE, state->config.score_file.name, ipc)) {
		fprintf(stderr, "Can not send score name.\n");
		goto cleanup;
	}

	if (uade_send_file(player, ipc)) {
		fprintf(stderr, "Can not send player name\n");
		goto cleanup;
	}

	/*
	 * Note, module can be NULL, in which case a message is sent that
	 * indicates no module is given.
	 */
	if (uade_send_file(module, ipc)) {
		fprintf(stderr, "Can not send module\n");
		goto cleanup;
	}

	if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
		fprintf(stderr, "Can not send token after module.\n");
		goto cleanup;
	}

	if (uade_receive_message(um, sizeof(space), ipc) <= 0) {
		fprintf(stderr, "Can not receive acknowledgement.\n");
		goto cleanup;
	}

	if (um->msgtype == UADE_REPLY_CANT_PLAY) {
		if (uade_receive_short_message(UADE_COMMAND_TOKEN, ipc)) {
			uade_warning("Can not receive token in main loop\n");
			goto cleanup;
		}
		return UADECORE_CANT_PLAY;
	}

	if (um->msgtype != UADE_REPLY_CAN_PLAY) {
		fprintf(stderr, "Unexpected reply from uade: %u\n",
			(unsigned int)um->msgtype);
		goto cleanup;
	}

	if (uade_receive_short_message(UADE_COMMAND_TOKEN, ipc) < 0) {
		fprintf(stderr, "Can not receive token after play ack.\n");
		goto cleanup;
	}

	if (uc->ignore_player_check) {
		if (uade_send_short_message(UADE_COMMAND_IGNORE_CHECK, ipc) < 0) {
			fprintf(stderr, "Can not send ignore check message.\n");
			goto cleanup;
		}
	}

	if (uc->no_ep_end) {
		if (uade_send_short_message
		    (UADE_COMMAND_SONG_END_NOT_POSSIBLE, ipc) < 0) {
			fprintf(stderr,
				"Can not send 'song end not possible'.\n");
			goto cleanup;
		}
	}

	uade_send_filter_command(state);

	send_resampling_command(ipc, uc);

	if (uc->speed_hack) {
		if (uade_send_short_message(UADE_COMMAND_SPEED_HACK, ipc)) {
			fprintf(stderr, "Can not send speed hack command.\n");
			goto cleanup;
		}
	}

	if (uc->use_ntsc) {
		if (uade_send_short_message(UADE_COMMAND_SET_NTSC, ipc)) {
			fprintf(stderr, "Can not send ntsc command.\n");
			goto cleanup;
		}
	}

	if (uc->frequency != UADE_DEFAULT_FREQUENCY) {
		if (uade_send_u32(UADE_COMMAND_SET_FREQUENCY, uc->frequency,
				  ipc)) {
			fprintf(stderr, "Can not send frequency.\n");
			goto cleanup;
		}
	}

	if (uc->use_text_scope) {
		if (uade_send_short_message(UADE_COMMAND_USE_TEXT_SCOPE, ipc)) {
			fprintf(stderr,	"Can not send use text scope command.\n");
			goto cleanup;
		}
	}

	if (send_ep_options(&us->ep_options, ipc) ||
	    send_ep_options(&uc->ep_options, ipc))
		goto cleanup;

	return 0;

      cleanup:
	return UADECORE_INIT_ERROR;
}
