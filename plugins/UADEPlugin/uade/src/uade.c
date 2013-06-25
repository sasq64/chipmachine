/* UADE - Unix Amiga Delitracker Emulator
 * Copyright 2000-2006, Heikki Orsila
 */

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "readcpu.h"
#include "newcpu.h"
#include "debug.h"
#include "gensound.h"
#include "cia.h"
#include "sd-sound.h"
#include "audio.h"

#include "uadectl.h"
#include "amigamsg.h"

#include <uade/uade.h>
#include <uade/ossupport.h>
#include <uade/sysincludes.h>
#include <uade/uadeconstants.h>
#include <uade/uadeutils.h>

#define UADE_SAFE_BUFSIZE 4096

enum print_help {
  OPTION_HELP = 1,
  OPTION_ILLEGAL_PARAMETERS = 2,
  OPTION_NO_SONGS = 3
};


static void change_subsong(int subsong);

static void uade_print_help(enum print_help problemcode, char *progname);
static void uade_put_long(int addr, int val);
static int uade_safe_load(int dst, FILE *file, int maxlen);
static int uade_valid_string(uae_u32 address);


static const int SCORE_MODULE_ADDR   = 0x100;
static const int SCORE_MODULE_LEN    = 0x104;
static const int SCORE_PLAYER_ADDR   = 0x108;
static const int SCORE_RELOC_ADDR    = 0x10C;
static const int SCORE_USER_STACK    = 0x110;
static const int SCORE_SUPER_STACK   = 0x114;
static const int SCORE_FORCE         = 0x118;
static const int SCORE_SET_SUBSONG   = 0x11c;
static const int SCORE_SUBSONG       = 0x120;
static const int SCORE_NTSC          = 0x124;
static const int SCORE_MODULE_NAME_ADDR = 0x128;
static const int SCORE_HAVE_SONGEND  = 0x12C;
static const int SCORE_POSTPAUSE     = 0x180;
static const int SCORE_PREPAUSE      = 0x184;
static const int SCORE_DELIMON       = 0x188;
static const int SCORE_EXEC_DEBUG    = 0x18C;
static const int SCORE_VOLUME_TEST   = 0x190;
static const int SCORE_DMA_WAIT      = 0x194;
static const int SCORE_MODULECHANGE  = 0x198;

static const int SCORE_INPUT_MSG     = 0x200;
static const int SCORE_MIN_SUBSONG   = 0x204;
static const int SCORE_MAX_SUBSONG   = 0x208;
static const int SCORE_CUR_SUBSONG   = 0x20C;

static const int SCORE_OUTPUT_MSG    = 0x300;


struct uade_ipc uadecore_ipc;

int uadecore_audio_skip;
int uadecore_audio_output;
int uadecore_debug;
int uadecore_read_size;
int uadecore_reboot;
int uadecore_time_critical;


static int disable_modulechange;
static int old_ledstate;
static int big_endian;
static int dmawait;
static int execdebug;
static int highmem;
static struct uade_song song;
static int speed_hack;
static int voltestboolean;

static char epoptions[256];
static size_t epoptionsize;

static struct uade_file *cachedfile;
static char cachedfilename[PATH_MAX];

static void add_ep_option(const char *s)
{
  size_t bufsize, l, i;

  bufsize = sizeof epoptions;
  l = strlen(s) + 1;
  i = epoptionsize;

  if (strlcpy(&epoptions[i], s, bufsize - i) >= (bufsize - i)) {
    fprintf(stderr, "Warning: uade eagleplayer option overflow: %s\n", s);
    return;
  }

  epoptionsize += l;
}


/* This is called when an eagleplayer queries for attributes. The query result
   is returned through 'dst', and the result is at most maxlen bytes long.
   'src' contains the full query. */
static int get_info_for_ep(char *dst, char *src, int maxlen)
{
  int ret = -1;
  if (strcasecmp(src, "eagleoptions") == 0) {
    if (epoptionsize > 0) {
      if (epoptionsize <= maxlen) {
	ret = epoptionsize;
	memcpy(dst, epoptions, ret);
      } else {
	fprintf(stderr, "uadecore: too long options: %s maxlen = %d\n",
		epoptions, maxlen);
      }
    } else {
      ret = 0;
    }
  } else {
    uadecore_send_debug("Unknown eagleplayer attribute queried: %s", src);
  }
  return ret;
}

static int amiga_get_u32(int addr)
{
  uae_u32 *ptr;
  int x;
  if (!valid_address(addr, 4)) {
    fprintf(stderr, "uadecore: Invalid amiga_get_u32 (0x%x).\n", addr);
    return 0;
  }
  ptr = (uae_u32 *) get_real_address(addr);
  return ntohl(*ptr);
}

static void change_subsong(int subsong)
{
  song.cur_subsong = subsong;
  uade_put_long(SCORE_SUBSONG, subsong);
  uadecore_send_amiga_message(AMIGAMSG_SETSUBSONG);
  flush_sound();
}


static int calc_reloc_size(uae_u32 *src, uae_u32 *end)
{
  uae_u32 offset;
  int i, nhunks;

  if (ntohl(*src) != 0x000003f3)
    return 0;
  src++;

  if (src >= end)
    return 0;
  if (ntohl(*src))
    return 0;
  src++;

  if (src >= end)
    return 0;
  /* take number of hunks, and apply the undocumented 16-bit mask feature */
  nhunks = ntohl(*src) & 0xffff;
  if (nhunks == 0)
    return 0;
  src += 3;          /* skip number of hunks, and first & last hunk indexes */

  offset = 0;

  for (i = 0; i < nhunks; i++) {
    if (src >= end)
      return 0;
    offset += 4 * (ntohl(*src) & 0x00FFFFFF);
    src++;
  }
  if (((int) offset) <= 0 || ((int) offset) >= highmem)
    return 0;
  return ((int) offset);
}


/* last part of the audio system pipeline */
void uadecore_check_sound_buffers(int bytes)
{
  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;

  /* transmit in big endian format, so swap if little endian */
  if (big_endian == 0)
    uadecore_swap_buffer_bytes(sndbuffer, bytes);

  /* LED state changes are reported here because we are in send state and
     this place is heavily rate limited. */
  if (old_ledstate != gui_ledstate) {
    old_ledstate = gui_ledstate;
    uadecore_send_debug("LED is %s", gui_ledstate ? "ON" : "OFF");
  }

  um->msgtype = UADE_REPLY_DATA;
  um->size = bytes;
  memcpy(um->data, sndbuffer, bytes);
  if (uade_send_message(um, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Could not send sample data.\n");
    exit(1);
  }

  uadecore_read_size -= bytes;
  assert(uadecore_read_size >= 0);

  if (uadecore_read_size == 0) {
    /* if all requested data has been sent, move to S state */
    if (uade_send_short_message(UADE_COMMAND_TOKEN, &uadecore_ipc)) {
      fprintf(stderr, "uadecore: Could not send token (after samples).\n");
      exit(1);
    }
    uadecore_handle_r_state();
  }
}


/* Send debug messages back to uade frontend, which either prints
   the message for user or not. "-v" option can be used in uade123 to see all
   these messages. */
void uadecore_send_debug(const char *fmt, ...)
{
  char dmsg[256];
  va_list ap;
  va_start (ap, fmt);
  vsnprintf(dmsg, sizeof(dmsg), fmt, ap);
  if (uade_send_string(UADE_REPLY_MSG, dmsg, &uadecore_ipc)) {
    fprintf(stderr, "uadecore %s:%d: Could not send debug message.\n", __FILE__, __LINE__);
  }
}

static int uade_safe_copy(int dst, const void *buf, size_t buflen)
{
	int maxlen = highmem - dst;
	if (maxlen <= 0)
		return 0;
	if (buflen > (size_t) maxlen) {
		fprintf(stderr, "uadecore: Too long a copy: %zu\n", buflen);
		return 0;
	}
	memcpy(get_real_address(dst), buf, buflen);
	return (int) buflen;
}

static void invalidate_amiga_file_cache(void)
{
	uade_file_free(cachedfile);
	cachedfile = NULL;
	cachedfilename[0] = 0;
}

static struct uade_file *lookup_amiga_file_cache(const char *filename)
{
	struct uade_file *f;

	/* Return cached file, if filename matches */
	if (cachedfile != NULL && strcmp(filename, cachedfilename) == 0)
		return cachedfile;

	f = uade_request_amiga_file(filename, &uadecore_ipc);
	if (f == NULL)
		return NULL;

	/* Free old cached file, if exists */
	invalidate_amiga_file_cache();

	/* Set the new file to be cached */
	cachedfile = f;
	strlcpy(cachedfilename, filename, sizeof cachedfilename);

	return f;
}

void uadecore_get_amiga_message(void)
{
  uae_u8 *ptr;
  uae_u8 *nameptr;
  int x;
  unsigned int mins, maxs, curs;
  int status;
  int src, dst, len;
  size_t off;
  char tmpstr[256];
  char *srcstr, *dststr;
  struct uade_file *f;
  uint32_t *u32ptr;
  uint8_t space[256];
  struct uade_msg *um = (struct uade_msg *) space;

  x = amiga_get_u32(SCORE_INPUT_MSG);  /* message type from amiga */

  switch (x) {

  case AMIGAMSG_SONG_END:
    uadecore_song_end("player", 0);
    break;

  case AMIGAMSG_SUBSINFO:
    mins = amiga_get_u32(SCORE_MIN_SUBSONG);
    maxs = amiga_get_u32(SCORE_MAX_SUBSONG);
    curs = amiga_get_u32(SCORE_CUR_SUBSONG);
    /* Brain damage in TFMX BC Kid Despair */
    if (maxs < mins) {
      uadecore_send_debug("Odd subsongs. Eagleplayer reported (min, cur, max) == (%u, %u, %u)", mins, curs, maxs);
      maxs = mins;
    }
    /* Brain damage in Bubble bobble custom */
    if (curs > maxs) {
      uadecore_send_debug("Odd subsongs. Eagleplayer reported (min, cur, max) == (%u, %u, %u)", mins, curs, maxs);
      maxs = curs;
    }
    um->msgtype = UADE_REPLY_SUBSONG_INFO;
    um->size = 12;
    u32ptr = (uint32_t *) um->data;
    u32ptr[0] = htonl(mins);
    u32ptr[1] = htonl(maxs);
    u32ptr[2] = htonl(curs);
    if (uade_send_message(um, &uadecore_ipc)) {
      fprintf(stderr, "uadecore: Could not send subsong info message.\n");
      exit(1);
    }
    break;

  case AMIGAMSG_PLAYERNAME:
    strlcpy(tmpstr, (char *) get_real_address(0x204), sizeof tmpstr);
    uade_send_string(UADE_REPLY_PLAYERNAME, tmpstr, &uadecore_ipc);
    break;

  case AMIGAMSG_MODULENAME:
    strlcpy(tmpstr, (char *) get_real_address(0x204), sizeof tmpstr);
    uade_send_string(UADE_REPLY_MODULENAME, tmpstr, &uadecore_ipc);
    break;

  case AMIGAMSG_FORMATNAME:
    strlcpy(tmpstr, (char *) get_real_address(0x204), sizeof tmpstr);
    uade_send_string(UADE_REPLY_FORMATNAME, tmpstr, &uadecore_ipc);
    break;

  case AMIGAMSG_GENERALMSG:
    uadecore_send_debug((char *) get_real_address(0x204));
    break;

  case AMIGAMSG_CHECKERROR:
    uadecore_song_end("module check failed", 1);
    break;

  case AMIGAMSG_SCORECRASH:
    if (uadecore_debug) {
      fprintf(stderr, "uadecore: Score crashed.\n");
      activate_debugger();
      break;
    }
    uadecore_song_end("score crashed", 1);
    break;

  case AMIGAMSG_SCOREDEAD:
     if (uadecore_debug) {
      fprintf(stderr, "uadecore: Score is dead.\n"); 
      activate_debugger();
      break;
    }
     uadecore_song_end("score died", 1);
    break;

  case AMIGAMSG_LOADFILE:
    /*
     * Load a file named at 0x204 (name pointer) to address pointed by
     * 0x208 and insert the length to 0x20C.
     * For example, R-Type (TFMX format) uses this.
     */
    src = amiga_get_u32(0x204);
    if (!uade_valid_string(src)) {
	    fprintf(stderr, "uadecore: Load name in invalid address range.\n");
	    break;
    }
    nameptr = get_real_address(src);
    f = lookup_amiga_file_cache((const char *) nameptr);
    if (f == NULL) {
	    uadecore_send_debug("load: request error: %s", nameptr);
	    exit(1);
    }
    if (f->data == NULL) {
	    /* File not found */
	    uadecore_send_debug("load: file not found: %s", nameptr);
	    break;
    }
    dst = amiga_get_u32(0x208);
    len = uade_safe_copy(dst, f->data, f->size);
    if (len == 0 && f->size > 0)
	    uadecore_send_debug("load: too long a file to copy");
    uade_put_long(0x20C, len);
    uadecore_send_debug("load: %s ptr 0x%x size 0x%x", nameptr, dst, len);
    break;

  case AMIGAMSG_READ:
    /* Used by "mdat.Crystal_Palace-1", for example */
    src = amiga_get_u32(0x204);
    if (!uade_valid_string(src)) {
	    fprintf(stderr, "uadecore: Read name in invalid address range.\n");
	    break;
    }
    nameptr = get_real_address(src);
    f = lookup_amiga_file_cache((const char *) nameptr);
    if (f == NULL) {
	    uadecore_send_debug("read: request error: %s", nameptr);
	    exit(1);
    }
    x = 0;
    if (f->data != NULL) {
	    dst = amiga_get_u32(0x208);
	    off = amiga_get_u32(0x20C);
	    len = amiga_get_u32(0x210);
	    if (off >= f->size) {
		    uadecore_send_debug("read: file offset over the file end");
	    } else {
		    size_t endpos = off + ((size_t) len);
		    size_t tocopy = len;
		    if (endpos > f->size)
			    tocopy = f->size - off;
		    x = uade_safe_copy(dst, f->data + off, tocopy);
	    }
	    uadecore_send_debug("read: %s dst 0x%x off 0x%x len 0x%x bytesread 0x%x", nameptr, dst, off, len, x);
    } else {
	    uadecore_send_debug("read: file not found: %s", nameptr);
    }
    uade_put_long(0x214, x);
    break;

  case AMIGAMSG_FILESIZE:
    /* Used by "mdat.Crystal_Palace-1", for example */
    src = amiga_get_u32(0x204);
    if (!uade_valid_string(src)) {
      fprintf(stderr, "uadecore: Filesize name in invalid address range.\n");
      break;
    }
    nameptr = get_real_address(src);
    f = lookup_amiga_file_cache((const char *) nameptr);
    if (f == NULL) {
	    uadecore_send_debug("filesize: request error: %s", nameptr);
	    exit(1);
    }
    len = 0;
    x = 0;
    if (f->data != NULL) {
	    len = f->size;
	    x = -1;
	    uadecore_send_debug("filesize: file %s res 0x%x", nameptr, len);
    } else {
	    /* Note, f->size == -1 if file does not exist */
	    uadecore_send_debug("filesize: file not found: %s", nameptr);
    }
    uade_put_long(0x208, len);
    uade_put_long(0x20C, x);
    break;

  case AMIGAMSG_TIME_CRITICAL:
    uadecore_time_critical = amiga_get_u32(0x204) ? 1 : 0;
    if (speed_hack < 0) {
      /* a negative value forbids use of speed hack */
      uadecore_time_critical = 0;
    }
    break;

  case AMIGAMSG_GET_INFO:
    src = amiga_get_u32(0x204);
    dst = amiga_get_u32(0x208);
    len = amiga_get_u32(0x20C);
    if (!uade_valid_string(src)) {
      fprintf(stderr, "uadecore: get info: Invalid src: 0x%x\n", src);
      break;
    }
    if (len <= 0) {
      fprintf(stderr, "uadecore: get info: len = %d\n", len);
      break;
    }
    if (!valid_address(dst, len)) {
      fprintf(stderr, "uadecore: get info: Invalid dst: 0x%x\n", dst);
      break;
    }
    srcstr = (char *) get_real_address(src);
    dststr = (char *) get_real_address(dst);
    uadecore_send_debug("score issued an info request: %s (maxlen %d)", srcstr, len);
    len = get_info_for_ep(dststr, srcstr, len);
    /* Send printable debug */
    do {
      size_t i;
      size_t maxspace = sizeof space;
      if (len <= 0) {
	maxspace = 1;
      } else {
	if (len < maxspace)
	  maxspace = len;
      }
      for (i = 0; i < maxspace; i++) {
	space[i] = dststr[i];
	if (space[i] == 0)
	  space[i] = ' ';
      }
      if (i < maxspace) {
	space[i] = 0;
      } else {
	space[maxspace - 1] = 0;
      }
      uadecore_send_debug("reply to score: %s (total len %d)", space, len);
    } while (0);
    uade_put_long(0x20C, len);
    break;

  case AMIGAMSG_START_OUTPUT:
    uadecore_audio_output = 1;
    break;

  default:
    fprintf(stderr,"uadecore: Unknown message from score (%d)\n", x);
    break;
  }
}


void uadecore_handle_r_state(void)
{
  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;
  int ret;
  uint32_t x, y;

  while (1) {

    ret = uade_receive_message(um, sizeof(space), &uadecore_ipc);
    if (ret == 0) {
      /*
       * Terminate uadecore when libuade closes the control socket.
       * This is the usual (intended) place where uadecore terminates itself.
       */
      exit(0);
    } else if (ret < 0) {
      fprintf(stderr, "uadecore: Error on input. Exiting with error.\n");
      exit(1);
    }

    if (um->msgtype == UADE_COMMAND_TOKEN)
      break;

    switch (um->msgtype) {

    case UADE_COMMAND_ACTIVATE_DEBUGGER:
      fprintf(stderr, "uadecore: Received activate debugger message.\n");
      activate_debugger();
      uadecore_debug = 1;
      break;

    case UADE_COMMAND_CHANGE_SUBSONG:
      if (uade_parse_u32_message(&x, um)) {
	fprintf(stderr, "uadecore: Invalid size with change subsong.\n");
	exit(1);
      }
      change_subsong(x);
      break;

    case UADE_COMMAND_FILTER:
      if (uade_parse_two_u32s_message(&x, &y, um)) {
	fprintf(stderr, "uadecore: Invalid size with filter command\n");
	exit(1);
      }
      audio_set_filter(x, y);
      break;

    case UADE_COMMAND_IGNORE_CHECK:
      /* override bit for sound format checking */
      uade_put_long(SCORE_FORCE, 1);
      break;

    case UADE_COMMAND_SET_FREQUENCY:
      if (uade_parse_u32_message(&x, um)) {
	fprintf(stderr, "Invalid frequency message size: %u\n", um->size);
	exit(1);
      }
      set_sound_freq(x);
      break;

    case UADE_COMMAND_SET_PLAYER_OPTION:
      uade_check_fix_string(um, 256);
      add_ep_option((char *) um->data);
      break;

    case UADE_COMMAND_SET_RESAMPLING_MODE:
      uade_check_fix_string(um, 16);
      audio_set_resampler((char *) um->data);
      break;

    case UADE_COMMAND_SPEED_HACK:
      uadecore_time_critical = 1;
      break;

    case UADE_COMMAND_READ:
      if (uadecore_read_size != 0) {
	fprintf(stderr, "uadecore: Read not allowed when uadecore_read_size > 0.\n");
	exit(1);
      }
      if (uade_parse_u32_message(&x, um)) {
	fprintf(stderr, "uadecore: Invalid size on read command.\n");
	exit(1);
      }
      uadecore_read_size = x;
      if (uadecore_read_size == 0 || uadecore_read_size > MAX_SOUND_BUF_SIZE || (uadecore_read_size & 3) != 0) {
	fprintf(stderr, "uadecore: Invalid read size: %d\n", uadecore_read_size);
	exit(1);
      }
      break;

    case UADE_COMMAND_REBOOT:
      uadecore_reboot = 1;
      break;

    case UADE_COMMAND_SET_NTSC:
      fprintf(stderr, "\nuadecore: Changing to NTSC mode.\n");
      uadecore_set_ntsc(1);
      break;

    case UADE_COMMAND_SONG_END_NOT_POSSIBLE:
      uadecore_set_automatic_song_end(0);
      break;

    case UADE_COMMAND_SET_SUBSONG:
      if (uade_parse_u32_message(&x, um)) {
	fprintf(stderr, "uadecore: Invalid size on set subsong command.\n");
	exit(1);
      }
      uade_put_long(SCORE_SET_SUBSONG, 1);
      uade_put_long(SCORE_SUBSONG, x);
      break;

    case UADE_COMMAND_USE_TEXT_SCOPE:
      audio_use_text_scope();
      break;

    default:
      fprintf(stderr, "uadecore: Received invalid command %d\n", um->msgtype);
      exit(1);
    }
  }
}


void uadecore_option(int argc, char **argv)
{
  int i, j, no_more_opts;
  char **s_argv;
  int s_argc;
  int cfg_loaded = 0;
  char optionsfile[PATH_MAX];
  int ret;
  int in_fd = -1;
  int out_fd = -1;
  char *endptr;

  /* network byte order is the big endian order */
  big_endian = (htonl(0x1234) == 0x1234);

  memset(&song, 0, sizeof(song));

  no_more_opts = 0;

  s_argv = malloc(sizeof(argv[0]) * (argc + 1));
  if (!s_argv) {
    fprintf (stderr, "uadecore: Out of memory for command line parsing.\n");
    exit(1);
  }
  s_argc = 0;
  s_argv[s_argc++] = argv[0];

  for (i = 1; i < argc;) {

    j = i;

    /* if argv[i] begins with '-', see if it is a switch that we should
       handle here. */
    
    if (argv[i][0] == '-') {

      if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
	uade_print_help(OPTION_HELP, argv[0]);
	exit(0);

      } else if (!strcmp(argv[i], "-i")) {
	if ((i + 1) >= argc) {
	  fprintf(stderr, "uadecore: %s parameter missing\n", argv[i]);
	  uade_print_help(OPTION_ILLEGAL_PARAMETERS, argv[0]);
	  exit(1);
	}
	in_fd = strtol(argv[i + 1], &endptr, 10);
	if (in_fd < 0 || *endptr != 0) {
		fprintf(stderr, "uadecore: Invalid -i parameter: %s\n",
			argv[i + 1]);
		exit(1);
	}
	i += 2;

      } else if (!strcmp(argv[i], "-o")) {
	if ((i + 1) >= argc) {
	  fprintf(stderr, "uadecore: %s parameter missing\n", argv[i]);
	  uade_print_help(OPTION_ILLEGAL_PARAMETERS, argv[0]);
	  exit(1);
	}
	out_fd = strtol(argv[i + 1], &endptr, 10);
	if (out_fd < 0 || *endptr != 0) {
		fprintf(stderr, "uadecore: Invalid -o parameter: %s\n",
			argv[i + 1]);
		exit(1);
	}
	i += 2;

      } else if (!strcmp(argv[i], "--")) {
	for (i = i + 1; i < argc ; i++)
	  s_argv[s_argc++] = argv[i];
	break;
      }
    }

    if (i == j) {
      s_argv[s_argc++] = argv[i];
      i++;
    }
  }
  s_argv[s_argc] = NULL;

  if (in_fd < 0 || out_fd < 0) {
	  fprintf(stderr, "uadecore: Must have -i and -o parameters\n");
	  exit(1);
  }

  uade_set_peer(&uadecore_ipc, 0, in_fd, out_fd);

  ret = uade_receive_string(optionsfile, UADE_COMMAND_CONFIG, sizeof(optionsfile), &uadecore_ipc);
  if (ret == 0) {
    fprintf(stderr, "uadecore: No config file passed as a message.\n");
    exit(1);
  } else if (ret < 0) {
    fprintf(stderr, "uadecore: Invalid input. Expected a config file.\n");
    exit(1);
  }

  /* use the config file provided with a message, if '-config' option
     was not given */
  if (!cfg_loaded) {
    if (cfgfile_load (&currprefs, optionsfile) == 0) {
      fprintf(stderr, "uadecore: Could not load uaerc (%s).\n", optionsfile);
      exit(1);
    }
  }

  free(s_argv);

  uadecore_reboot = 1;
}


static void uade_print_help(enum print_help problemcode, char *progname)
{
  switch (problemcode) {
  case OPTION_HELP:
    /* just for printing help */
    break;
  case OPTION_ILLEGAL_PARAMETERS:
    fprintf(stderr, "uadecore: Invalid parameters.\n\n");
    break;
  case OPTION_NO_SONGS:
    fprintf(stderr, "uadecore: No songs given as parameters.\n\n");
    break;
  default:
    fprintf(stderr, "uadecore: Unknown error.\n");
    break;
  }
  fprintf(stderr, "UADE usage:\n");
  fprintf(stderr, " %s [OPTIONS]\n\n", progname);

  fprintf(stderr, " options:\n");
  fprintf(stderr, " -h\t\tPrint help\n");
  fprintf(stderr, " -i file\tSet input source ('filename' or 'fd://number')\n");
  fprintf(stderr, " -o file\tSet output destination ('filename' or 'fd://number'\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "This tool should not be run from the command line. This is for internal use\n");
  fprintf(stderr, "of other programs.\n");
}

/* this is called for each played song from newcpu.c/m68k_reset() */
void uadecore_reset(void)
{
  /* don't load anything under 0x1000 (execbase top at $1000) */
  const int modnameaddr = 0x00400;
  const int scoreaddr   = 0x01000;
  const int userstack   = 0x08500;
  const int superstack  = 0x08f00;
  const int playeraddr  = 0x09000;
  int relocaddr;
  int modaddr;
  int len;
  FILE *file;
  int bytesread;
  struct uade_file *player;
  struct uade_file *module;

  uint8_t command[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) command;

  int ret;

  invalidate_amiga_file_cache();

 nextsong:

  /* IMPORTANT:
     It seems that certain players don't work totally reliably if memory
     contains trash from previous songs. To be certain that each song is
     played from the same initial state of emulator we clear the memory
     from 0x400 to 'highmem' each time a new song is played */
  highmem = 0;
  while (highmem < 0x800000) {
    if (!valid_address(0, highmem + 0x10000))
      break;
    highmem += 0x10000;
  }
  if (highmem < 0x80000) {
    fprintf(stderr, "uadecore: There must be at least 512 KiB of amiga memory (%d bytes found).\n", highmem);
    exit(1);
  }
  if (highmem < 0x200000) {
    fprintf(stderr, "uadecore: Warning: highmem == 0x%x (< 0x200000)!\n", highmem);
  }
  memset(get_real_address(0), 0, highmem);

  song.cur_subsong = song.min_subsong = song.max_subsong = 0;

  ret = uade_receive_string(song.scorename, UADE_COMMAND_SCORE, sizeof(song.scorename), &uadecore_ipc);
  if (ret == 0) {
    exit(0);
  } else if (ret < 0) {
    fprintf(stderr, "uadecore: Invalid input. Expected score name.\n");
    exit(1);
  }

  player = NULL;
  module = NULL;

  /* Get eagleplayer from libuade */
  player = uade_receive_file(&uadecore_ipc);
  if (player == NULL || player->data == NULL) {
	  fprintf(stderr, "uadecore: Invalid input. Expected player.\n");
	  exit(1);
  }
  if (player->name == NULL) {
	  strlcpy(song.playername, "no-player-name", sizeof song.playername);
  } else {
	  strlcpy(song.playername, player->name, sizeof song.playername);
  }

  /* Get module from libuade */
  module = uade_receive_file(&uadecore_ipc);
  if (module == NULL) {
	  fprintf(stderr, "uadecore: Invalid input. Expected module.\n");
	  exit(1);
  }
  if (module->name != NULL)
	  strlcpy(song.modulename, module->name, sizeof song.modulename);
  else
	  strlcpy(song.modulename, "no-module-name", sizeof song.modulename);

  uadecore_set_automatic_song_end(1);

  uade_put_long(SCORE_EXEC_DEBUG, execdebug ? 0x12345678 : 0);
  uade_put_long(SCORE_VOLUME_TEST, voltestboolean);
  uade_put_long(SCORE_DMA_WAIT, dmawait);
  uade_put_long(SCORE_MODULECHANGE, disable_modulechange);

  bytesread = uade_safe_copy(playeraddr, player->data, player->size);
  if (bytesread == 0) {
	  fprintf(stderr, "uadecore: Can not do safe copy for player\n");
	  goto skiptonextsong;
  }

  uade_file_free(player);
  player = NULL;

  /* set player executable address for relocator */
  uade_put_long(SCORE_PLAYER_ADDR, playeraddr);
  len = calc_reloc_size((uae_u32 *) get_real_address(playeraddr),
			(uae_u32 *) get_real_address(playeraddr + bytesread));
  if (!len) {
    fprintf(stderr, "uadecore: Problem with reloc calculation.\n");
    goto skiptonextsong;
  }
  relocaddr  = ((playeraddr + bytesread) & 0x7FFFF000) + 0x4000;
  /* + 0x4000 for hippel coso (wasseremu) */
  modaddr = ((relocaddr + len) & 0x7FFFF000) + 0x2000;

  if (modaddr <= relocaddr) {
    /* this is very bad because sound core memory allocation will fail */
    fprintf(stderr, "uadecore: Warning: modaddr <= relocaddr: 0x%x <= 0x%x\n", modaddr, relocaddr);
  }

  uade_put_long(SCORE_RELOC_ADDR, relocaddr);  /*address for relocated player*/
  uade_put_long(SCORE_MODULE_ADDR, modaddr);   /* set module address */
  uade_put_long(SCORE_MODULE_LEN, 0);          /* set module size to zero */
  uade_put_long(SCORE_MODULE_NAME_ADDR, 0);    /* mod name address pointer */

  /* Load the module, if available */
  if (module->data != NULL) {
	  bytesread = uade_safe_copy(modaddr, module->data, module->size);
	  if (bytesread == 0) {
		  fprintf(stderr, "uadecore: Module safe copy failed\n");
		  goto skiptonextsong;
	  }

	  uade_put_long(SCORE_MODULE_LEN, bytesread);

	  if (!valid_address(modnameaddr, strlen(song.modulename) + 1)) {
		  fprintf(stderr, "uadecore: Invalid address for modulename.\n");
		  goto skiptonextsong;
	  }

	  strlcpy((char *) get_real_address(modnameaddr), song.modulename, 1024);
	  uade_put_long(SCORE_MODULE_NAME_ADDR, modnameaddr);
  } else {
	  if (!valid_address(modnameaddr, strlen(song.playername) + 1)) {
		  fprintf(stderr, "uadecore: Invalid address for playername.\n");
		  goto skiptonextsong;
	  }

	  strlcpy((char *) get_real_address(modnameaddr), song.playername, 1024);
	  uade_put_long(SCORE_MODULE_NAME_ADDR, modnameaddr);

	  bytesread = 0;
  }

  uade_file_free(module);
  module = NULL;

  /* load sound core (score) */
  if ((file = fopen(song.scorename, "rb"))) {
    bytesread = uade_safe_load(scoreaddr, file, highmem - scoreaddr);
    fclose(file);
  } else {
    fprintf (stderr, "uadecore: Can not load score (%s).\n", song.scorename);
    goto skiptonextsong;
  }

  m68k_areg(regs,7) = scoreaddr;
  m68k_setpc(scoreaddr);

  /* obey player format checking */
  uade_put_long(SCORE_FORCE, 0);
  /* set default subsong */
  uade_put_long(SCORE_SET_SUBSONG, 0);
  uade_put_long(SCORE_SUBSONG, 0);
  /* set PAL mode */
  uadecore_set_ntsc(0);

  /* pause bits (don't care!), for debugging purposes only */
  uade_put_long(SCORE_PREPAUSE, 0);
  uade_put_long(SCORE_POSTPAUSE, 0);
  /* set user and supervisor stack pointers */
  uade_put_long(SCORE_USER_STACK, userstack);
  uade_put_long(SCORE_SUPER_STACK, superstack);
  /* no message for score */
  uade_put_long(SCORE_OUTPUT_MSG, 0);
  if ((userstack - (scoreaddr + bytesread)) < 0x1000)
    fprintf(stderr, "uadecore: Amiga stack overrun warning.\n");

  flush_sound();

  /* note that speed_hack can be negative (meaning that uade never uses
     speed hack, even if it's requested by the amiga player)! */
  uadecore_time_critical = 0;
  if (speed_hack > 0) {
    uadecore_time_critical = 1;
  }

  uadecore_reboot = 0;

  uadecore_audio_output = 0;
  uadecore_audio_skip = 0;

  old_ledstate = gui_ledstate;

  if (uade_receive_short_message(UADE_COMMAND_TOKEN, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not receive token in uade_reset().\n");
    exit(1);
  }

  if (uade_send_short_message(UADE_REPLY_CAN_PLAY, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not send 'CAN_PLAY' reply.\n");
    exit(1);
  }
  if (uade_send_short_message(UADE_COMMAND_TOKEN, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not send token from uade_reset().\n");
    exit(1);
  }

  set_sound_freq(UADE_DEFAULT_FREQUENCY);
  epoptionsize = 0;

  return;

 skiptonextsong:
  uade_file_free(player);
  player = NULL;
  uade_file_free(module);
  module = NULL;

  fprintf(stderr, "uadecore: Can not play. Reboot.\n");

  if (uade_receive_short_message(UADE_COMMAND_TOKEN, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not receive token in uade_reset().\n");
    exit(1);
  }

  if (uade_send_short_message(UADE_REPLY_CANT_PLAY, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not send 'CANT_PLAY' reply.\n");
    exit(1);
  }
  if (uade_send_short_message(UADE_COMMAND_TOKEN, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Can not send token from uade_reset().\n");
    exit(1);
  }
  goto nextsong;
}


static void uade_put_long(int addr, int val)
{
  uae_u32 *p;
  if (!valid_address(addr, 4)) {
    fprintf(stderr, "uadecore: Invalid uade_put_long (0x%x).\n", addr);
    return;
  }
  p = (uae_u32 *) get_real_address(addr);
  *p = htonl(val);
}

static int uade_safe_load(int dst, FILE *file, int maxlen)
{
  char buf[UADE_SAFE_BUFSIZE];
  int nbytes, len, off;

  len = UADE_SAFE_BUFSIZE;
  off = 0;

  while (maxlen > 0) {

    if (maxlen < UADE_SAFE_BUFSIZE)
      len = maxlen;

    nbytes = fread(buf, 1, len, file);
    if (!nbytes)
      break;

    if (!valid_address(dst + off, nbytes)) {
      fprintf(stderr, "uadecore: Invalid load range [%x,%x).\n", dst + off, dst + off + nbytes);
      break;
    }

    memcpy(get_real_address(dst + off), buf, nbytes);
    off += nbytes;
    maxlen -= nbytes;
  }

  /* find out how much would have been read even if maxlen was violated */
  while ((nbytes = fread(buf, 1, UADE_SAFE_BUFSIZE, file)))
    off += nbytes;

  return off;
}

static void uade_safe_get_string(char *dst, int src, int maxlen)
{
  int i = 0;
  while (1) {
    if (i >= maxlen)
      break;
    if (!valid_address(src + i, 1)) {
      fprintf(stderr, "uadecore: Invalid memory range in safe_get_string.\n");
      break;
    }
    dst[i] = * (char *) get_real_address(src + i);
    i++;
  }
  if (maxlen > 0) {
    if (i < maxlen) {
      dst[i] = 0;
    } else { 
      fprintf(stderr, "uadecore: Warning: string truncated.\n");
      dst[maxlen - 1] = 0;
    }
  }
}


void uadecore_send_amiga_message(int msgtype)
{
  uade_put_long(SCORE_OUTPUT_MSG, msgtype);
}


void uadecore_set_ntsc(int usentsc)
{
  uade_put_long(SCORE_NTSC, usentsc);
}


void uadecore_set_automatic_song_end(int song_end_possible)
{
  uade_put_long(SCORE_HAVE_SONGEND, song_end_possible);
}


/* if kill_it is zero, uade may switch to next subsong. if kill_it is non-zero
   uade will always switch to next song (if any) */
void uadecore_song_end(char *reason, int kill_it)
{
  uint8_t space[sizeof(struct uade_msg) + 4 + 256];
  struct uade_msg *um = (struct uade_msg *) space;
  int tailbytes = ((intptr_t) sndbufpt) - ((intptr_t) sndbuffer);
  um->msgtype = UADE_REPLY_SONG_END;
  write_be_u32(um->data, tailbytes);
  write_be_u32(um->data + 4, kill_it);
  strlcpy((char *) um->data + 8, reason, 256);
  um->size = 8 + strlen(reason) + 1;
  if (uade_send_message(um, &uadecore_ipc)) {
    fprintf(stderr, "uadecore: Could not send song end message.\n");
    exit(1);
  }
  /* if audio_output is zero (and thus the client is waiting for the first
     sound data block from this song), then start audio output so that the
     clients first sound finishes ASAP and we can go to the next (sub)song.
     uade must finish the pending sound data request (for the client) even if
     the sound core crashed */
  uadecore_audio_output = 1;
}


void uadecore_swap_buffer_bytes(void *data, int bytes)
{
  uae_u8 *buf = (uae_u8 *) data;
  uae_u8 sample;
  int i;
  assert((bytes % 2) == 0);
  for (i = 0; i < bytes; i += 2) {
    sample = buf[i + 0];
    buf[i + 0] = buf[i + 1];
    buf[i + 1] = sample;
  }
}


/* check if string is on a safe zone */
static int uade_valid_string(uae_u32 address)
{
  while (valid_address(address, 1)) {
    if (* ((uae_u8 *) get_real_address(address)) == 0)
      return 1;
    address++;
  }
  fprintf(stderr, "uadecore: Invalid string at 0x%x.\n", address);
  return 0;
}
