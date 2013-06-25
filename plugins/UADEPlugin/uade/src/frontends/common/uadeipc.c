/* UADE
 *
 * Copyright 2005 Heikki Orsila <heikki.orsila@iki.fi>
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any way you
 * want in your projects.
 */

#include <uade/uade.h>
#include <uade/uadeipc.h>
#include <uade/ossupport.h>
#include <uade/sysincludes.h>
#include <uade/unixatomic.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

static int valid_message(struct uade_msg *uc);

void uade_check_fix_string(struct uade_msg *um, size_t maxlen)
{
	uint8_t *s = (uint8_t *) um->data;
	size_t len;

	assert(maxlen > 0);

	if (um->size == 0 || um->size > maxlen) {
		fprintf(stderr, "uade_check_fix_string: Bad string size: %u\n", um->size);
		s[0] = 0;
		return;
	}

	len = 0;
	while (s[len] != 0 && len < maxlen)
		len++;

	s[maxlen - 1] = 0;

	if (len == maxlen) {
		fprintf(stderr, "uade_check_fix_string: Too long a string\n");
		return;
	}

	if (um->size != (len + 1)) {
		fprintf(stderr, "uade_check_fix_string: String size does not match\n");
		s[len] = 0;
	}
}

static ssize_t get_more(size_t bytes, struct uade_ipc *ipc)
{
	ssize_t s;
	if (ipc->inputbytes >= bytes)
		return 0;
	if (bytes > sizeof ipc->inputbuffer) {
		fprintf(stderr, "ipc: Internal error: bytes > inputbuffer\n");
		return -1;
	}
	s = uade_atomic_read(ipc->in_fd, &ipc->inputbuffer[ipc->inputbytes],
			     bytes - ipc->inputbytes);
	if (s <= 0)
		return -1;
	ipc->inputbytes += s;
	return 0;
}

static void copy_from_inputbuffer(void *dst, int bytes, struct uade_ipc *ipc)
{
	if (ipc->inputbytes < bytes) {
		fprintf(stderr, "not enough bytes in input buffer\n");
		exit(1);
	}
	memcpy(dst, ipc->inputbuffer, bytes);
	memmove(ipc->inputbuffer, &ipc->inputbuffer[bytes], ipc->inputbytes - bytes);
	ipc->inputbytes -= bytes;
}

int uade_parse_u32_message(uint32_t *u1, struct uade_msg *um)
{
	if (um->size != 4)
		return -1;
	*u1 = read_be_u32(um->data);
	return 0;
}

int uade_parse_two_u32s_message(uint32_t *u1, uint32_t *u2,
				struct uade_msg *um)
{
	if (um->size != 8)
		return -1;
	*u1 = read_be_u32(um->data);
	*u2 = read_be_u32(um->data + 4);
	return 0;
}

static int valid_name(const uint8_t *name, size_t maxlen)
{
	size_t s;
	for (s = 0; s < maxlen; s++)
		if (name[s] == 0)
			return 1;
	return 0;
}

struct uade_file *uade_receive_file(struct uade_ipc *ipc)
{
	uint8_t msgdata[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *) &msgdata;
	struct uade_msg_file *meta;
	struct uade_msg_file_data *data;
	uint32_t filesize;
	uint32_t readbytes = 0;
	struct uade_file *f = calloc(1, sizeof(struct uade_file));

	if (f == NULL) {
		fprintf(stderr, "uade_receive_file(): No memory for struct\n");
		return NULL;
	}

	if (uade_receive_message(um, sizeof msgdata, ipc) <= 0) {
		fprintf(stderr, "uade_receive_file(): Can not get meta\n");
		return NULL;
	}
	meta = (struct uade_msg_file *) &msgdata;
	if (meta->msgtype != UADE_COMMAND_FILE) {
		fprintf(stderr, "uade_receive_file(): Expected UADE_COMMAND_FILE\n");
		return NULL;
	}

	filesize = htonl(meta->filesize);
	/*
	 * filesize == -1 indicates that the file does not exist, it is not an
	 * error.
	 */
	if (filesize == -1)
		return f;

	if (!valid_name(meta->filename, sizeof meta->filename)) {
		fprintf(stderr, "uade_receive_file(): Invalid name\n");
		return NULL;
	}
	if (meta->filename[0] == 0) {
		f->name = NULL;
	} else {
		f->name = strdup((const char *) meta->filename);
		if (f->name == NULL) {
			fprintf(stderr, "uade_receive_file(): No memory for name\n");
			return NULL;
		}
	}
	f->data = malloc(filesize);
	if (f->data == NULL) {
		fprintf(stderr, "uade_receive_file(): Can not allocate memory\n");
		goto err;
	}

	while (readbytes < filesize) {
		if (uade_receive_message(um, sizeof msgdata, ipc) <= 0) {
			fprintf(stderr, "uade_receive_file(): Can not read data\n");
			goto err;
		}
		data = (struct uade_msg_file_data *) um;
		if (data->msgtype != UADE_COMMAND_FILE_DATA) {
			fprintf(stderr, "uade_receive_file(): Expected UADE_COMMAND_FILE_DATA\n");
			goto err;
		}
		if (data->size > (filesize - readbytes)) {
			fprintf(stderr, "uade_receive_file(): Too much data\n");
			goto err;
		}
		memcpy(((uint8_t *) f->data) + readbytes, data->data, data->size);
		readbytes += data->size;
	}
	f->size = filesize;
	return f;

err:
	uade_file_free(f);
	return NULL;
}

int uade_receive_message(struct uade_msg *um, size_t maxbytes,
			 struct uade_ipc *ipc)
{
	if (ipc->state == UADE_INITIAL_STATE) {
		ipc->state = UADE_R_STATE;
	} else if (ipc->state == UADE_S_STATE) {
		fprintf(stderr, "protocol error: receiving in S state is forbidden\n");
		return -1;
	}

	if (ipc->inputbytes < sizeof(*um)) {
		if (get_more(sizeof(*um), ipc))
			return 0;
	}

	copy_from_inputbuffer(um, sizeof(*um), ipc);

	um->msgtype = ntohl(um->msgtype);
	um->size = ntohl(um->size);

	if (!valid_message(um))
		return -1;

	if (ipc->inputbytes < um->size) {
		if (get_more(um->size, ipc))
			return -1;
	}
	copy_from_inputbuffer(&um->data, um->size, ipc);

	if (um->msgtype == UADE_COMMAND_TOKEN)
		ipc->state = UADE_S_STATE;

	return 1;
}

int uade_receive_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc)
{
	struct uade_msg um;

	if (ipc->state == UADE_INITIAL_STATE) {
		ipc->state = UADE_R_STATE;
	} else if (ipc->state == UADE_S_STATE) {
		fprintf(stderr, "protocol error: receiving (%d) in S state is forbidden\n", msgtype);
		return -1;
	}

	if (uade_receive_message(&um, sizeof(um), ipc) <= 0) {
		fprintf(stderr, "can not receive short message: %d\n", msgtype);
		return -1;
	}
	return (um.msgtype == msgtype) ? 0 : -1;
}

int uade_receive_string(char *s, enum uade_msgtype com, size_t maxlen,
			struct uade_ipc *ipc)
{
	uint8_t commandbuf[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *) commandbuf;
	int ret;

	if (ipc->state == UADE_INITIAL_STATE) {
		ipc->state = UADE_R_STATE;
	} else if (ipc->state == UADE_S_STATE) {
		fprintf(stderr, "protocol error: receiving in S state is forbidden\n");
		return -1;
	}

	ret = uade_receive_message(um, UADE_MAX_MESSAGE_SIZE, ipc);
	if (ret <= 0)
		return ret;
	if (um->msgtype != com)
		return -1;
	if (um->size == 0)
		return -1;
	if (um->size != (strlen((char *) um->data) + 1))
		return -1;
	strlcpy(s, (char *) um->data, maxlen);
	return 1;
}

struct uade_file *uade_request_amiga_file(const char *name, struct uade_ipc *ipc)
{
	struct uade_file *f;
	if (uade_send_string(UADE_COMMAND_REQUEST_AMIGA_FILE, name, ipc)) {
		fprintf(stderr, "Can not request amiga file: %s\n", name);
		return NULL;
	}
	/* Hack, change to receiving state, and back to sending state */
	assert(ipc->state == UADE_S_STATE);
	ipc->state = UADE_R_STATE;
	f = uade_receive_file(ipc);
	ipc->state = UADE_S_STATE;
	return f;
}

int uade_send_file(const struct uade_file *f, struct uade_ipc *ipc)
{
	size_t sent = 0;
	size_t sendbytes;
	struct uade_msg_file_data data;
	const size_t maxtransfer = sizeof(data.data);
	struct uade_msg_file meta = {.msgtype = UADE_COMMAND_FILE,
				     .size = UADE_MAX_NAME_SIZE,
				     .filesize = -1,
				    };
	if (f != NULL) {
		if (f->name != NULL)
			strlcpy((char *) meta.filename, f->name, sizeof meta.filename);
		meta.filesize = htonl(f->size);
	}
	if (uade_send_message((struct uade_msg *) &meta, ipc)) {
		fprintf(stderr, "Can not send file meta\n");
		return -1;
	}
	if (f == NULL)
		return 0;

	while (sent < f->size) {
		data.msgtype = UADE_COMMAND_FILE_DATA;
		sendbytes = f->size - sent;
		if (sendbytes > maxtransfer)
			sendbytes = maxtransfer;
		data.size = sendbytes;
		memcpy(data.data, ((uint8_t *) f->data) + sent, sendbytes);
		if (uade_send_message((struct uade_msg *) &data, ipc)) {
			fprintf(stderr, "Can not send file data\n");
			return -1;
		}
		sent += sendbytes;
	}
	return 0;
}

int uade_send_message(struct uade_msg *um, struct uade_ipc *ipc)
{
	uint32_t size = um->size;

	if (ipc->state == UADE_INITIAL_STATE) {
		ipc->state = UADE_S_STATE;
	} else if (ipc->state == UADE_R_STATE) {
		fprintf(stderr, "protocol error: sending in R state is forbidden\n");
		return -1;
	}
	if (!valid_message(um)) {
		fprintf(stderr, "uadeipc: Tried to send an invalid message\n");
		return -1;
	}
	if (um->msgtype == UADE_COMMAND_TOKEN)
		ipc->state = UADE_R_STATE;
	um->msgtype = htonl(um->msgtype);
	um->size = htonl(um->size);
	if (uade_atomic_write(ipc->out_fd, um, sizeof(*um) + size) < 0) {
		fprintf(stderr, "uade_atomic_write() failed\n");
		return -1;
	}
	um->msgtype = -1; /* POISON */
	um->size = -1; /* POISON */
	return 0;
}

int uade_send_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc)
{
	struct uade_msg msg = {.msgtype = msgtype};

	if (uade_send_message(&msg, ipc)) {
		fprintf(stderr, "can not send short message: %d\n", msgtype);
		return -1;
	}
	return 0;
}

int uade_send_string(enum uade_msgtype com, const char *str, struct uade_ipc *ipc)
{
	uint32_t size = strlen(str) + 1;
	struct uade_msg um = {.msgtype = ntohl(com), .size = ntohl(size)};

	if (ipc->state == UADE_INITIAL_STATE) {
		ipc->state = UADE_S_STATE;
	} else if (ipc->state == UADE_R_STATE) {
		fprintf(stderr, "protocol error: sending in R state is forbidden\n");
		return -1;
	}

	if ((sizeof(um) + size) > UADE_MAX_MESSAGE_SIZE)
		return -1;
	if (uade_atomic_write(ipc->out_fd, &um, sizeof(um)) < 0)
		return -1;
	if (uade_atomic_write(ipc->out_fd, str, size) < 0)
		return -1;
	return 0;
}

int uade_send_u32(enum uade_msgtype com, uint32_t u, struct uade_ipc *ipc)
{
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *) space;
	um->msgtype = com;
	um->size = 4;
	write_be_u32(um->data, u);
	return uade_send_message(um, ipc);
}

static size_t prepare_message(void *space, size_t maxsize,
			      enum uade_msgtype com, size_t payloadsize)
{
	struct uade_msg *um = space;
	size_t size = sizeof(*um) + payloadsize;
	if (maxsize < size) {
		fprintf(stderr, "ipc: Not enough space to prepare a msg\n");
		return 0;
	}
	um->msgtype = com;
	um->size = payloadsize;
	return size;
}

size_t uade_ipc_prepare_two_u32s(void *space, size_t maxsize,
				 enum uade_msgtype com,
				 uint32_t u1, uint32_t u2)
{
	struct uade_msg *um = (struct uade_msg *) space;
	size_t size = prepare_message(um, maxsize, com, 8);
	if (!size)
		return 0;
	write_be_u32(um->data, u1);
	write_be_u32(um->data + 4, u2);
	return size;
}

int uade_send_two_u32s(enum uade_msgtype com, uint32_t u1, uint32_t u2,
		       struct uade_ipc *ipc)
{
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	if (!uade_ipc_prepare_two_u32s(space, sizeof space, com, u1, u2))
		return -1;
	return uade_send_message((struct uade_msg *) space, ipc);
}

void uade_set_peer(struct uade_ipc *ipc, int peer_is_client,
		   int in_fd, int out_fd)
{
	assert(peer_is_client == 0 || peer_is_client == 1);
	assert(in_fd >= 0);
	assert(out_fd >= 0);
	*ipc = (struct uade_ipc) {.state = UADE_INITIAL_STATE,
				  .in_fd = in_fd,
				  .out_fd = out_fd};
}

static int valid_message(struct uade_msg *um)
{
	size_t len;
	if (um->msgtype <= UADE_MSG_FIRST || um->msgtype >= UADE_MSG_LAST) {
		fprintf(stderr, "Unknown command: %u\n",
			(unsigned int) um->msgtype);
		return 0;
	}
	len = sizeof(*um) + um->size;
	if (um->size > UADE_MAX_MESSAGE_SIZE || len > UADE_MAX_MESSAGE_SIZE) {
		fprintf(stderr, "Too long a message: payload %u\n", um->size);
		return 0;
	}
	return 1;
}
