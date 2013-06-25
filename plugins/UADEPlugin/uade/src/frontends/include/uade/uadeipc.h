#ifndef _UADEIPC_H_
#define _UADEIPC_H_

#include <uade/uadeutils.h>

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define UADE_MAX_MESSAGE_SIZE (8 + 4096)
#define UADE_MAX_NAME_SIZE 4000

enum uade_msgtype {
	UADE_MSG_FIRST = 0,
	UADE_COMMAND_ACTIVATE_DEBUGGER,
	UADE_COMMAND_CHANGE_SUBSONG,
	UADE_COMMAND_CONFIG,
	UADE_COMMAND_SCORE,
	UADE_COMMAND_FILE,
	UADE_COMMAND_FILE_DATA,
	UADE_COMMAND_REQUEST_AMIGA_FILE, /* sent from the uadecore */
	UADE_COMMAND_READ,
	UADE_COMMAND_REBOOT,
	UADE_COMMAND_SET_SUBSONG,
	UADE_COMMAND_IGNORE_CHECK,
	UADE_COMMAND_SONG_END_NOT_POSSIBLE,
	UADE_COMMAND_SET_NTSC,
	UADE_COMMAND_FILTER,
	UADE_COMMAND_SET_FREQUENCY,
	UADE_COMMAND_SET_PLAYER_OPTION,
	UADE_COMMAND_SET_RESAMPLING_MODE,
	UADE_COMMAND_SPEED_HACK,
	UADE_COMMAND_TOKEN,
	UADE_COMMAND_USE_TEXT_SCOPE,
	UADE_REPLY_MSG,
	UADE_REPLY_CANT_PLAY,
	UADE_REPLY_CAN_PLAY,
	UADE_REPLY_SONG_END,
	UADE_REPLY_SUBSONG_INFO,
	UADE_REPLY_PLAYERNAME,
	UADE_REPLY_MODULENAME,
	UADE_REPLY_FORMATNAME,
	UADE_REPLY_DATA,
	UADE_MSG_LAST
};

struct uade_msg {
	uint32_t msgtype;
	uint32_t size;
	uint8_t data[0];
} __attribute__((packed));

/*
 * uade_msg_file is a valid uade_msg struct. It is followed by file data in
 * separate messages.
 */
struct uade_msg_file {
	uint32_t msgtype;
	uint32_t size;
	uint32_t filesize; /* network byte order */
	uint8_t filename[UADE_MAX_NAME_SIZE];
} __attribute__((packed));

struct uade_msg_file_data {
	uint32_t msgtype;
	uint32_t size;
	uint8_t data[4096];
} __attribute__((packed));

enum uade_control_state {
	UADE_INITIAL_STATE = 0,
	UADE_R_STATE,
	UADE_S_STATE
};

struct uade_ipc {
	int in_fd;
	int out_fd;
	unsigned int inputbytes;
	char inputbuffer[UADE_MAX_MESSAGE_SIZE];
	enum uade_control_state state;
};

void uade_check_fix_string(struct uade_msg *um, size_t maxlen);
size_t uade_ipc_prepare_two_u32s(void *space, size_t maxsize,
				 enum uade_msgtype com,
				 uint32_t u1, uint32_t u2);
int uade_parse_u32_message(uint32_t *u1, struct uade_msg *um);
int uade_parse_two_u32s_message(uint32_t *u1, uint32_t *u2, struct uade_msg *um);
struct uade_file *uade_receive_file(struct uade_ipc *ipc);
int uade_receive_message(struct uade_msg *um, size_t maxbytes, struct uade_ipc *ipc);
int uade_receive_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc);
int uade_receive_string(char *s, enum uade_msgtype msgtype, size_t maxlen, struct uade_ipc *ipc);
struct uade_file *uade_request_amiga_file(const char *name, struct uade_ipc *ipc);
int uade_send_file(const struct uade_file *f, struct uade_ipc *ipc);
int uade_send_message(struct uade_msg *um, struct uade_ipc *ipc);
int uade_send_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc);
int uade_send_string(enum uade_msgtype msgtype, const char *str, struct uade_ipc *ipc);
int uade_send_u32(enum uade_msgtype com, uint32_t u, struct uade_ipc *ipc);
int uade_send_two_u32s(enum uade_msgtype com, uint32_t u1, uint32_t u2, struct uade_ipc *ipc);
void uade_set_peer(struct uade_ipc *ipc, int peer_is_client,
		   int in_fd, int out_fd);

#endif
