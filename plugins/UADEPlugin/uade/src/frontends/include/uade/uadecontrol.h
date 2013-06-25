#ifndef _UADE_CONTROL_
#define _UADE_CONTROL_

#include <sys/types.h>

#include <uade/uadestate.h>

enum {
	UADECORE_INIT_OK = 0,
	UADECORE_INIT_ERROR,
	UADECORE_CANT_PLAY
};

size_t uade_prepare_filter_command(void *space, size_t maxsize,
				   const struct uade_state *state);
int uade_read_request(struct uade_state *state);
void uade_send_filter_command(struct uade_state *state);
int uade_song_initialization(struct uade_file *player, struct uade_file *module, struct uade_state *state);
void uade_subsong_control(int subsong, int command, struct uade_ipc *ipc);

#endif
