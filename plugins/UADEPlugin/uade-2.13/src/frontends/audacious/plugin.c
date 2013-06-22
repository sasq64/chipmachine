/* UADE2 plugin for Audacious
 *
 * Copyright (C) 2005-2006  Heikki Orsila, UADE TEAM
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any way you
 * want in your projects.
 */

#include <libgen.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "uadeipc.h"
#include "eagleplayer.h"
#include "uadeconfig.h"
#include "uadecontrol.h"
#include "uadeconstants.h"
#include "ossupport.h"
#include "uadeconf.h"
#include "effects.h"
#include "sysincludes.h"
#include "songinfo.h"
#include "plugin.h"
#include "subsongseek.h"
#include "fileinfo.h"
#include "songdb.h"
#include "uadestate.h"


#define PLUGIN_DEBUG 0

#if PLUGIN_DEBUG
#define plugindebug(fmt, args...) do { fprintf(stderr, "%s:%d: %s: " fmt, __FILE__, __LINE__, __func__, ## args); } while(0)
#else
#define plugindebug(fmt, args...)
#endif


static int initialize_song(char *filename);
static void uade_cleanup(void);
static void uade_file_info(char *filename);
static void uade_get_song_info(char *filename, char **title, int *length);
static void uade_init(void);
static int uadeaudacious_is_our_file(char *filename);

#if __AUDACIOUS_PLUGIN_API__ >= 6
static void uade_info_string(InputPlayback *playhandle);
#define UADE_INFO_STRING(playhandle) uade_info_string(playhandle)
#else
static void uade_info_string(void);
/* Hack, we don't actually use playhandle */
#define UADE_INFO_STRING(playhandle) uade_info_string()
#endif

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static int uade_get_time(InputPlayback *playhandle);
static void uade_pause(InputPlayback *playhandle, short paused);
static void uade_play_file(InputPlayback *playhandle);
static void uade_seek(InputPlayback *playhandle, int time);
static void uade_stop(InputPlayback *playhandle);
#else
static int uade_get_time(void);
static void uade_pause(short paused);
static void uade_play_file(char *filename);
static void uade_seek(int time);
static void uade_stop(void);
#endif


/* GLOBAL VARIABLE DECLARATIONS */

static InputPlugin uade_ip = {
  .description = "UADE " UADE_VERSION,
  .init = uade_init,
  .is_our_file = uadeaudacious_is_our_file,
  .play_file = uade_play_file,
  .stop = uade_stop,
  .pause = uade_pause,
  .seek = uade_seek,
  .get_time = uade_get_time,
  .cleanup = uade_cleanup,
  .get_song_info = uade_get_song_info,
  .file_info_box = uade_file_info,
};

#ifndef __AUDACIOUS_INPUT_PLUGIN_API__
static InputPlugin *playhandle = &uade_ip;
#endif

#if __AUDACIOUS_PLUGIN_API__ >= 2
static InputPlugin *uade_iplist[] = {&uade_ip, NULL};
DECLARE_PLUGIN(uade, NULL, NULL, uade_iplist, NULL, NULL, NULL, NULL);
#endif

static const AFormat sample_format = FMT_S16_NE;

/* Definition of trigger type:
   abort_playing variable does not need locking, because it is initialized to
   false synchronously in play_file, and it is triggered to be true in stop().
   Other places in the plugin only read the value. We call this type of
   variable a trigger type.

   The locking strategy in the plugin is that all lockable variables are
   initialized in play_file() for each song. Using variables after that
   requires locking.  When locking is needed, call uade_lock() and
   uade_unlock(). */

static int abort_playing;     /* Trigger type */

static struct uade_state state;

static char configname[PATH_MAX];
static pthread_t decode_thread;
static struct uade_config config_backup;
static char gui_filename[PATH_MAX];
static char gui_formatname[256];
static int gui_info_set;
static char gui_modulename[256];
static char gui_module_filename[PATH_MAX];
static char gui_playername[256];
static char gui_player_filename[PATH_MAX];
static int last_beat_played;  /* Lock before use */
static char md5name[PATH_MAX];
static int record_playtime; /* Lock before use */
static int plugin_disabled;
static char songconfname[PATH_MAX];

static time_t config_load_time;
static time_t md5_load_time;

int uade_is_paused;           /* Lock before use */
int uade_thread_running;      /* Trigger type */
int uade_seek_forward;        /* Lock before use */
int uade_select_sub;          /* Lock before use */

int uade_config_optimization;

static pthread_mutex_t vlock = PTHREAD_MUTEX_INITIALIZER;


static void uade_usleep(void)
{
#if __AUDACIOUS_PLUGIN_API__ >= 6
  g_usleep(10000);
#else
  xmms_usleep(10000);
#endif
}


static void test_uade_conf(void)
{
  struct stat st;

  if (stat(configname, &st))
    return;

  /* Read only newer files */
  if (st.st_mtime <= config_load_time)
    return;

  config_load_time = st.st_mtime;

  uade_load_config(&config_backup, configname);
}


static void load_config(void)
{
  test_uade_conf();
}


static void load_content_db(void)
{
  struct stat st;
  time_t curtime = time(NULL);
  char name[PATH_MAX];

  if (curtime)
    md5_load_time = curtime;

  if (md5name[0] == 0) {
    char *home = uade_open_create_home();
    if (home)
      snprintf(md5name, sizeof md5name, "%s/.uade2/contentdb", home);
  }

  /* User database has priority over global database, so we read it first */
  if (md5name[0]) {
    if (stat(md5name, &st) == 0) {
      if (uade_read_content_db(md5name))
	return;
    } else {
      FILE *f = fopen(md5name, "w");
      if (f)
	fclose(f);
      uade_read_content_db(md5name);
    }
  }

  snprintf(name, sizeof name, "%s/contentdb.conf", UADE_CONFIG_BASE_DIR);
  if (stat(name, &st) == 0)
    uade_read_content_db(name);
}


static void uade_cleanup(void)
{
  if (state.pid)
    kill(state.pid, SIGTERM);

  if (md5name[0]) {
    struct stat st;
    if (stat(md5name, &st) == 0 && md5_load_time >= st.st_mtime)
      uade_save_content_db(md5name);
  }
}


static void uade_file_info(char *filename)
{
  int adder = 0;

  if (strncmp(filename, "uade://", 7) == 0)
    adder = 7;
  uade_gui_file_info(filename + adder, gui_player_filename, gui_modulename, gui_playername, gui_formatname);

}


int uade_get_cur_subsong(int def)
{
    int subsong;
    uade_lock();
    subsong = -1;
    if (state.song != NULL)
      subsong = state.song->cur_subsong;
    uade_unlock();
    if (subsong == -1)
	subsong = def;
    return subsong;
}


int uade_get_max_subsong(int def)
{
    int subsong;
    uade_lock();
    subsong = -1;
    if (state.song != NULL)
      subsong = state.song->max_subsong;
    uade_unlock();
    if (subsong == -1)
	subsong = def;
    return subsong;
}


int uade_get_min_subsong(int def)
{
    int subsong;
    uade_lock();
    subsong = -1;
    if (state.song != NULL)
      subsong = state.song->min_subsong;
    uade_unlock();
    if (subsong == -1)
	subsong = def;
    return subsong;
}


void uade_lock(void)
{
  if (pthread_mutex_lock(&vlock)) {
    fprintf(stderr, "UADE2 locking error.\n");
    exit(-1);
  }
}


void uade_unlock(void)
{
  if (pthread_mutex_unlock(&vlock)) {
    fprintf(stderr, "UADE2 unlocking error.\n");
    exit(-1);
  }
}


/* this function is first called by xmms. returns pointer to plugin table */
#if __AUDACIOUS_PLUGIN_API__ < 2
InputPlugin *get_iplugin_info(void)
{
  return &uade_ip;
}
#endif


/* xmms initializes uade by calling this function */
static void uade_init(void)
{
  char *home;
  int config_loaded;

  config_load_time = time(NULL);

  config_loaded = uade_load_initial_config(configname, sizeof configname,
					   &config_backup, NULL);

  load_content_db();

  uade_load_initial_song_conf(songconfname, sizeof songconfname,
			      &config_backup, NULL);

  home = uade_open_create_home();

  if (home != NULL) {
    /* If config exists in home, ignore global uade.conf. */
    snprintf(configname, sizeof configname, "%s/.uade2/uade.conf", home);
  }

  if (config_loaded == 0) {
    fprintf(stderr, "No config file found for UADE XMMS plugin. Will try to load config from\n");
    fprintf(stderr, "$HOME/.uade2/uade.conf in the future.\n");
  }
}


/* Audacious calls this function to check if filename belongs to this
   plugin. */
static int uadeaudacious_is_our_file(char *filename)
{
  char *decoded = NULL;
  int ret = FALSE;

  if (strncmp(filename, "uade://", 7) == 0)
    return 1;

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
  if (strncmp(filename, "file:/", 6) == 0) {
    decoded = g_filename_from_uri((char *) filename, NULL, NULL);
    filename = decoded;
  }
#endif

  if (filename != NULL) {

    uade_lock();

    /* This is a performance optimization to avoid re-reading uade.conf
     * when state.config hasn't yet been read. uade_is_our_file() needs the
     * config */
    if (!state.validconfig) {
      state.config = config_backup;
      state.validconfig = 1;

      /* Verify that this condition is true at most once */
      assert(!uade_config_optimization);
      uade_config_optimization = 1;
    }

    ret = uade_is_our_file(filename, 1, &state) ? TRUE : FALSE;

    uade_unlock();
  }

  if (decoded != NULL)
    free (decoded);

  return ret;
}


/* Analyze file format, and handshake with uadecore. */
static int initialize_song(char *filename)
{
  int ret;
  char modulename[PATH_MAX];
  char playername[PATH_MAX];
  char scorename[PATH_MAX];

  uade_lock();

  state.config = config_backup;
  state.validconfig = 1;

  state.ep = NULL;
  state.song = NULL;

  ret = uade_is_our_file(filename, 0, &state);

  if (!ret)
    goto error;

  strlcpy(modulename, filename, sizeof modulename);
  strlcpy(gui_module_filename, filename, sizeof gui_module_filename);

  snprintf(scorename, sizeof scorename, "%s/score", UADE_CONFIG_BASE_DIR);

  if (strcmp(state.ep->playername, "custom") == 0) {
    strlcpy(playername, modulename, sizeof playername);
    modulename[0] = 0;
    gui_module_filename[0] = 0;
  } else {
    snprintf(playername, sizeof playername, "%s/players/%s", UADE_CONFIG_BASE_DIR, state.ep->playername);
  }

  if (!uade_alloc_song(&state, filename))
    goto error;

  uade_set_ep_attributes(&state);

  uade_set_song_attributes(&state, playername, sizeof playername);

  uade_set_effects(&state);

  strlcpy(gui_player_filename, playername, sizeof gui_player_filename);

  ret = uade_song_initialization(scorename, playername, modulename, &state);
  if (ret) {
    if (ret != UADECORE_CANT_PLAY && ret != UADECORE_INIT_ERROR) {
      fprintf(stderr, "Can not initialize song. Unknown error.\n");
      plugin_disabled = 1;
    }
    uade_lock();
    uade_unalloc_song(&state);

    goto error;
  }

  uade_unlock();
  return TRUE;

 error:
  uade_unlock();
  return FALSE;
}

static void *play_loop(void *arg)
{
#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
  InputPlayback *playhandle = arg;
#endif

  enum uade_control_state controlstate = UADE_S_STATE;
  int ret;
  int left = 0;
  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;
  int subsong_end = 0;
  uint16_t *sm;
  int i;
  unsigned int play_bytes, tailbytes = 0;
  uint64_t subsong_bytes = 0;
  char *reason;
  uint32_t *u32ptr;
  int framesize = UADE_CHANNELS * UADE_BYTES_PER_SAMPLE;
  int song_end_trigger = 0;
  int64_t skip_bytes = 0;

  uade_lock();
  record_playtime = 1;
  uade_unlock();

  while (1) {
    if (controlstate == UADE_S_STATE) {

      assert(left == 0);

      if (abort_playing) {
	uade_lock();
	record_playtime = 0;
	uade_unlock();
	break;
      }

      uade_lock();
      if (uade_seek_forward) {
	skip_bytes += uade_seek_forward * (UADE_BYTES_PER_FRAME * state.config.frequency);
	playhandle->output->flush(playhandle->output->written_time() + uade_seek_forward * 1000);
	uade_seek_forward = 0;
      }
      if (uade_select_sub != -1) {
	state.song->cur_subsong = uade_select_sub;

	uade_change_subsong(&state);

	playhandle->output->flush(0);

	uade_select_sub = -1;
	subsong_end = 0;
	subsong_bytes = 0;

	/* we do this to avoid timeout, and to not record playtime */
	state.song->out_bytes = 0;
	record_playtime = 0;

	UADE_INFO_STRING(playhandle);
      }
      if (subsong_end && song_end_trigger == 0) {

	if (state.song->cur_subsong == -1 || state.song->max_subsong == -1) {
	  song_end_trigger = 1;

	} else {

	  state.song->cur_subsong++;

	  if (state.song->cur_subsong > state.song->max_subsong) {

	    song_end_trigger = 1;

	  } else {

	    uade_change_subsong(&state);

	    while (playhandle->output->buffer_playing())
	      uade_usleep();

	    playhandle->output->flush(0);

	    subsong_end = 0;
	    subsong_bytes = 0;

	    uade_unlock();
	    uade_gui_subsong_changed(state.song->cur_subsong);
	    uade_lock();
	    
	    UADE_INFO_STRING(playhandle);
	  }
	}
      }
      uade_unlock();

      if (song_end_trigger) {
	/* We must drain the audio fast if abort_playing happens (e.g.
	   the user changes song when we are here waiting the sound device) */
	while (playhandle->output->buffer_playing() && abort_playing == 0)
	  uade_usleep();

	break;
      }

      left = uade_read_request(&state.ipc);

      if (uade_send_short_message(UADE_COMMAND_TOKEN, &state.ipc)) {
	fprintf(stderr, "Can not send token.\n");
	return NULL;
      }
      controlstate = UADE_R_STATE;

    } else {

      if (uade_receive_message(um, sizeof(space), &state.ipc) <= 0) {
	fprintf(stderr, "Can not receive events from uade\n");
	exit(-1);
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

	if (subsong_end) {
	  play_bytes = tailbytes;
	  tailbytes = 0;
	} else {
	  play_bytes = um->size;
	}

	if (subsong_end == 0 && song_end_trigger == 0 &&
	    uade_test_silence(um->data, play_bytes, &state)) {
	  subsong_end = 1;
	}

	subsong_bytes += play_bytes;
	uade_lock();
	state.song->out_bytes += play_bytes;
	uade_unlock();

	if (skip_bytes > 0) {
	  if (play_bytes <= skip_bytes) {
	    skip_bytes -= play_bytes;
	    play_bytes = 0;
	  } else {
	    play_bytes -= skip_bytes;
	    skip_bytes = 0;
	  }
	}

	uade_effect_run(&state.effects, (int16_t *) um->data, play_bytes / framesize);
#if __AUDACIOUS_PLUGIN_API__ >= 6
	playhandle->pass_audio(playhandle, sample_format, UADE_CHANNELS, play_bytes, um->data, &uade_thread_running);
#else
	produce_audio(playhandle->output->written_time(), sample_format, UADE_CHANNELS, play_bytes, um->data, &uade_thread_running);
#endif
	if (state.config.timeout != -1 && state.config.use_timeouts) {
	  if (song_end_trigger == 0) {
	    uade_lock();
	    if (state.song->out_bytes / (UADE_BYTES_PER_FRAME * state.config.frequency) >= state.config.timeout) {
	      song_end_trigger = 1;
	      record_playtime = 0;
	    }
	    uade_unlock();
	  }
	}

	if (state.config.subsong_timeout != -1 && state.config.use_timeouts) {
	  if (subsong_end == 0 && song_end_trigger == 0) {
	    if (subsong_bytes / (UADE_BYTES_PER_FRAME * state.config.frequency) >= state.config.subsong_timeout) {
	      subsong_end = 1;
	      record_playtime = 0;
	    }
	  }
	}

	assert (left >= um->size);
	left -= um->size;
	break;

      case UADE_REPLY_FORMATNAME:
	uade_check_fix_string(um, 128);
	strlcpy(gui_formatname, (char *) um->data, sizeof gui_formatname);
	strlcpy(state.song->formatname, (char *) um->data, sizeof state.song->formatname);
	break;

      case UADE_REPLY_MODULENAME:
	uade_check_fix_string(um, 128);
	strlcpy(gui_modulename, (char *) um->data, sizeof gui_modulename);
	strlcpy(state.song->modulename, (char *) um->data, sizeof state.song->modulename);
	break;

      case UADE_REPLY_MSG:
	uade_check_fix_string(um, 128);
	plugindebug("Message: %s\n", (char *) um->data);
	break;

      case UADE_REPLY_PLAYERNAME:
	uade_check_fix_string(um, 128);
	strlcpy(gui_playername, (char *) um->data, sizeof gui_playername);
	strlcpy(state.song->playername, (char *) um->data, sizeof state.song->playername);
	break;

      case UADE_REPLY_SONG_END:
	if (um->size < 9) {
	  fprintf(stderr, "Invalid song end reply\n");
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
	  song_end_trigger = 1;
	}
	i = 0;
	reason = (char *) &um->data[8];
	while (reason[i] && i < (um->size - 8))
	  i++;
	if (reason[i] != 0 || (i != (um->size - 9))) {
	  fprintf(stderr, "Broken reason string with song end notice\n");
	  exit(-1);
	}
	/* fprintf(stderr, "Song end (%s)\n", reason); */
	break;

      case UADE_REPLY_SUBSONG_INFO:
	if (um->size != 12) {
	  fprintf(stderr, "subsong info: too short a message\n");
	  exit(-1);
	}
	u32ptr = (uint32_t *) um->data;
	uade_lock();
	state.song->min_subsong = ntohl(u32ptr[0]);
	state.song->max_subsong = ntohl(u32ptr[1]);
	state.song->cur_subsong = ntohl(u32ptr[2]);

	if (!(-1 <= state.song->min_subsong && state.song->min_subsong <= state.song->cur_subsong && state.song->cur_subsong <= state.song->max_subsong)) {
	  int tempmin = state.song->min_subsong, tempmax = state.song->max_subsong;
	  fprintf(stderr, "uade: The player is broken. Subsong info does not match with %s.\n", gui_filename);
	  state.song->min_subsong = tempmin <= tempmax ? tempmin : tempmax;
	  state.song->max_subsong = tempmax >= tempmin ? tempmax : tempmin;
	  if (state.song->cur_subsong > state.song->max_subsong)
	    state.song->max_subsong = state.song->cur_subsong;
	  else if (state.song->cur_subsong < state.song->min_subsong)
	    state.song->min_subsong = state.song->cur_subsong;
	}
	uade_unlock();
	break;

      default:
	fprintf(stderr, "Expected sound data. got %d.\n", um->msgtype);
	plugin_disabled = 1;
	return NULL;
      }
    }
  }

  last_beat_played = 1;

  if (uade_send_short_message(UADE_COMMAND_REBOOT, &state.ipc)) {
    fprintf(stderr, "Can not send reboot.\n");
    return NULL;
  }

  if (uade_send_short_message(UADE_COMMAND_TOKEN, &state.ipc)) {
    fprintf(stderr, "Can not send token.\n");
    return NULL;
  }

  do {
    ret = uade_receive_message(um, sizeof(space), &state.ipc);
    if (ret < 0) {
      fprintf(stderr, "Can not receive events from uade.\n");
      return NULL;
    }
    if (ret == 0) {
      fprintf(stderr, "End of input after reboot.\n");
      return NULL;
    }
  } while (um->msgtype != UADE_COMMAND_TOKEN);

  return NULL;
}

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static void uade_play_file(InputPlayback *playhandle)
#else
static void uade_play_file(char *filename)
#endif
{

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
  char *filename = playhandle->filename;
#endif

  char *decoded = NULL;

  char tempname[PATH_MAX];
  char *t;

  load_config();

  uade_lock();

  abort_playing = 0;
  last_beat_played = 0;
  record_playtime = 0;

  uade_is_paused = 0;
  uade_select_sub = -1;
  uade_seek_forward = 0;

  assert(state.song == NULL);

  uade_unlock();

  if (strncmp(filename, "uade://", 7) == 0)
    filename += 7;

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
  if (strncmp(filename, "file:/", 6) == 0) {
    decoded = g_filename_from_uri((char *) filename, NULL, NULL);
    if (decoded != NULL)
      filename = decoded;
  }
#endif

  /* We must make a copy of the name for basename, it is not guaranteed
     to preserve the original string intact */
  strlcpy(tempname, filename, sizeof tempname);
  t = basename(tempname);
  if (t == NULL)
    t = filename; /* out of memory */

  strlcpy(gui_filename, t, sizeof gui_filename);
  gui_info_set = 0;

  gui_formatname[0] = 0;
  gui_modulename[0] = 0;
  gui_playername[0] = 0;
  gui_module_filename[0] = 0;
  gui_player_filename[0] = 0;

  if (!state.pid) {
    char configname[PATH_MAX];
    snprintf(configname, sizeof configname, "%s/uaerc", UADE_CONFIG_BASE_DIR);
    uade_spawn(&state, UADE_CONFIG_UADE_CORE, configname);
  }

  if (!playhandle->output->open_audio(sample_format, config_backup.frequency, UADE_CHANNELS)) {
    abort_playing = 1;
    return;
  }

  if (plugin_disabled) {
    fprintf(stderr, "An error has occured. uade plugin is internally disabled.\n");
    goto err;
  }

  /* If content db has changed (newer mtime chan previously read) then force
     a reload */
  if (md5name[0]) {
    struct stat st;
    time_t curtime;

    if (stat(md5name, &st) == 0 && md5_load_time < st.st_mtime)
      load_content_db();

    /* Save current db if an hour has passed */
    curtime = time(NULL);
    if (curtime >= (md5_load_time + 3600)) {
      uade_save_content_db(md5name);
      md5_load_time = curtime;
    }
  } else {
    load_content_db();
  }

  if (initialize_song(filename) == FALSE)
    goto err;

#if __AUDACIOUS_PLUGIN_API__ >= 3
  decode_thread = pthread_self();
  uade_thread_running = 1;
  playhandle->set_pb_ready(playhandle);
  play_loop(playhandle);
#else
  if (pthread_create(&decode_thread, NULL, play_loop, playhandle)) {
    fprintf(stderr, "uade: can't create play_loop() thread. Please report!\n");
    goto err;
  }
  uade_thread_running = 1;
#endif

  free(decoded);
  return;

 err:

  free(decoded);

  uade_lock();

  if (state.song != NULL)
    uade_unalloc_song(&state);

  uade_unlock();

  /* close audio that was opened */
  playhandle->output->close_audio();

  abort_playing = 1;
}




#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static void uade_stop(InputPlayback *playhandle)
#else
static void uade_stop(void)
#endif
{
  /* Signal other subsystems to proceed to finished state as soon as possible
   */
  abort_playing = 1;

  /* Wait for playing thread to finish */
  if (uade_thread_running) {
    pthread_join(decode_thread, NULL);
    uade_thread_running = 0;
  }

  uade_gui_close_subsong_win();

  uade_lock();

  if (state.song != NULL) {

    if (record_playtime) {
      /* Song ended voluntarily -> tell the play time for Audacious, and
	 record it into song length db */
      int play_time = (state.song->out_bytes * 1000) / (UADE_BYTES_PER_FRAME * state.config.frequency);
      if (state.song->md5[0] != 0)
        uade_add_playtime(state.song->md5, play_time);

      state.song->playtime = play_time;
      state.song->cur_subsong = state.song->max_subsong;

      UADE_INFO_STRING(playhandle);
    }

    /* We must free uadesong after playthread has finished and additional
       GUI windows have been closed. */
    uade_unalloc_song(&state);
  }

  uade_unlock();

  playhandle->output->close_audio();
}


/* XMMS calls this function when pausing or unpausing */
#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static void uade_pause(InputPlayback *playhandle, short paused)
#else
static void uade_pause(short paused)
#endif
{
  uade_lock();
  uade_is_paused = paused;
  uade_unlock();

  playhandle->output->pause(paused);
}


/* XMMS calls this function when song is seeked */
#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static void uade_seek(InputPlayback *playhandle, int time)
#else
static void uade_seek(int time)
#endif
{
  uade_gui_seek_subsong(time);
}


/* XMMS calls this function periodically to determine current playing time.
   We use this function to report song name and title after play_file(),
   and to tell XMMS to end playing if song ends for any reason. */
#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
static int uade_get_time(InputPlayback *playhandle)
#else
static int uade_get_time(void)
#endif
{
  if (abort_playing || last_beat_played)
    return -1;

  /* uade_get_time can be called before play_file initializes uadesong
     structure, so we must return 0 as time */
  if (!uade_thread_running)
    return 0;

  if (!gui_info_set && state.song->max_subsong != -1) {
    uade_lock();

    if (state.song->max_subsong != -1)
      UADE_INFO_STRING(playhandle);

    gui_info_set = 1;
    uade_unlock();
    file_info_update(gui_module_filename, gui_player_filename, gui_modulename, gui_playername, gui_formatname);
  }

  return playhandle->output->output_time();
}


static void uade_get_song_info(char *filename, char **title, int *length)
{
  char tempname[PATH_MAX];
  char *t;

  if (strncmp(filename, "uade://", 7) == 0)
    filename += 7;

  strlcpy(tempname, filename, sizeof tempname);
  t = basename(tempname);
  if (t == NULL)
    t = filename;
  if ((*title = strdup(t)) == NULL)
    plugindebug("Not enough memory for song info.\n");
  *length = -1;
}


#if __AUDACIOUS_PLUGIN_API__ >= 6
static void uade_info_string(InputPlayback *playhandle)
#else
static void uade_info_string(void)
#endif
{
  char info[256];
  int playtime = state.song->playtime;

  /* Hack. Set info text and song length late because we didn't know
  subsong amounts before this. Pass zero as a length so that the
  graphical play time counter will run but seek is still enabled.
  Passing -1 as playtime would disable seeking. */
  if (playtime <= 0)
    playtime = 0;

  if (uade_generate_song_title(info, sizeof info, &state))
    strlcpy(info, gui_filename, sizeof info);

#if __AUDACIOUS_PLUGIN_API__ >= 6
  playhandle->set_params(playhandle, info, playtime,
			 UADE_BYTES_PER_FRAME * state.config.frequency,
			 state.config.frequency, UADE_CHANNELS);
#else
  uade_ip.set_info(info, playtime, UADE_BYTES_PER_FRAME * state.config.frequency,
		   state.config.frequency, UADE_CHANNELS);
#endif
}
