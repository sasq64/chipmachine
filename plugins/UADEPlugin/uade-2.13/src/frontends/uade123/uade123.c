/* uade123 - a simple command line frontend for uadecore.

   Copyright (C) 2005-2006 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#define _GNU_SOURCE

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
#include <getopt.h>

#include "uadecontrol.h"
#include "uadeipc.h"
#include "ossupport.h"
#include "uadeconfig.h"
#include "eagleplayer.h"
#include "uadeconf.h"
#include "sysincludes.h"
#include "songdb.h"
#include "support.h"
#include "uadestate.h"
#include "uade123.h"
#include "playlist.h"
#include "playloop.h"
#include "audio.h"
#include "terminal.h"
#include "amigafilter.h"

int uade_debug_trigger;
int uade_info_mode;
double uade_jump_pos = 0.0;

int uade_no_audio_output;
int uade_no_text_output;

char uade_output_file_format[16];
char uade_output_file_name[PATH_MAX];
struct playlist uade_playlist;
FILE *uade_terminal_file;
int uade_terminated;
int uade_song_end_trigger;

static int debug_mode;
static char md5name[PATH_MAX];
static time_t md5_load_time;
static pid_t uadepid = -1;
static char uadename[PATH_MAX];


static void print_help(void);
static void setup_sighandlers(void);
ssize_t stat_file_size(const char *name);
static void trivial_sigchld(int sig);
static void trivial_sigint(int sig);
static void cleanup(void);


static void load_content_db(struct uade_config *uc)
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

  /* First try to read users database */
  if (md5name[0]) {
    /* Try home directory first */
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

  /* Second try to read global database, this does not override any data
     from user database */
  snprintf(name, sizeof name, "%s/contentdb", uc->basedir.name);
  if (stat(name, &st) == 0)
    uade_read_content_db(name);
}


static void save_content_db(void)
{
  struct stat st;
  if (md5name[0] && stat(md5name, &st) == 0) {

    if (md5_load_time < st.st_mtime)
      uade_read_content_db(md5name);

    uade_save_content_db(md5name);
  }
}


static void scan_playlist(struct uade_config *uc)
{
  struct playlist_iterator pli;
  char *songfile;
  struct uade_state state = {.config = *uc};

  playlist_iterator(&pli, &uade_playlist);

  while (1) {
    songfile = playlist_iterator_get(&pli);
    if (songfile == NULL)
      break;

    if (!uade_is_our_file(songfile, 1, &state))
      continue;

    songfile = canonicalize_file_name(songfile);

    if (songfile)
      printf("%s\n", songfile);

    free(songfile);
    songfile = NULL;
  }
}


static void set_song_options(int *songconf_loaded, char *songoptions,
			     char *songconfname, size_t maxname)
{
  char homesongconfname[PATH_MAX];
  struct playlist_iterator pli;
  char *songfile;
  char *home;

  home = uade_open_create_home();
  if (home == NULL)
    die("No $HOME for song.conf :(\n");

  snprintf(homesongconfname, sizeof homesongconfname, "%s/.uade2/song.conf", home);

  if (*songconf_loaded == 0)
    strlcpy(songconfname, homesongconfname, maxname);

  playlist_iterator(&pli, &uade_playlist);

  while (1) {
    songfile = playlist_iterator_get(&pli);
    if (songfile == NULL)
      break;

    if (!uade_update_song_conf(songconfname, homesongconfname, songfile, songoptions))
      fprintf(stderr, "Could not update song.conf entry for %s\n", songfile);
  }
}


int main(int argc, char *argv[])
{
  int i;
  char configname[PATH_MAX] = "";
  char playername[PATH_MAX] = "";
  char scorename[PATH_MAX] = "";
  int playernamegiven = 0;
  char tmpstr[PATH_MAX + 256];
  long subsong = -1;
  FILE *listfile = NULL;
  int have_modules = 0;
  int ret;
  char *endptr;
  int uadeconf_loaded, songconf_loaded;
  char songconfname[PATH_MAX] = "";
  char uadeconfname[PATH_MAX];
  struct uade_config uc_eff, uc_cmdline, uc_main;
  char songoptions[256] = "";
  int have_song_options = 0;
  int plistdir;
  int scanmode = 0;

  struct uade_state state;

  enum {
    OPT_FIRST = 0x1FFF,
    OPT_BASEDIR,
    OPT_REPEAT,
    OPT_SCAN,
    OPT_SCOPE,
    OPT_SET,
    OPT_STDERR,
    OPT_VERSION
  };

  struct option long_options[] = {
    {"ao-option",        1, NULL, UC_AO_OPTION},
    {"basedir",          1, NULL, OPT_BASEDIR},
    {"buffer-time",      1, NULL, UC_BUFFER_TIME},
    {"cygwin",           0, NULL, UC_CYGWIN_DRIVE_WORKAROUND},
    {"debug",            0, NULL, 'd'},
    {"detect-format-by-content", 0, NULL, UC_CONTENT_DETECTION},
    {"disable-timeouts", 0, NULL, UC_DISABLE_TIMEOUTS},
    {"enable-timeouts",  0, NULL, UC_ENABLE_TIMEOUTS},
    {"ep-option",        1, NULL, 'x'},
    {"filter",           2, NULL, UC_FILTER_TYPE},
    {"force-led",        1, NULL, UC_FORCE_LED},
    {"frequency",        1, NULL, UC_FREQUENCY},
    {"gain",             1, NULL, 'G'},
    {"get-info",         0, NULL, 'g'},
    {"headphones",       0, NULL, UC_HEADPHONES},
    {"headphones2",      0, NULL, UC_HEADPHONES2},
    {"help",             0, NULL, 'h'},
    {"ignore",           0, NULL, 'i'},
    {"interpolator",     1, NULL, UC_RESAMPLER},
    {"jump",             1, NULL, 'j'},
    {"keys",             1, NULL, 'k'},
    {"list",             1, NULL, '@'},
    {"magic",            0, NULL, UC_CONTENT_DETECTION},
    {"no-ep-end-detect", 0, NULL, 'n'},
    {"no-song-end",      0, NULL, 'n'},
    {"normalise",        2, NULL, UC_NORMALISE},
    {"ntsc",             0, NULL, UC_NTSC},
    {"one",              0, NULL, '1'},
    {"pal",              0, NULL, UC_PAL},
    {"panning",          1, NULL, 'p'},
    {"recursive",        0, NULL, 'r'},
    {"repeat",           0, NULL, OPT_REPEAT},
    {"resampler",        1, NULL, UC_RESAMPLER},
    {"scan",             0, NULL, OPT_SCAN},
    {"scope",            0, NULL, OPT_SCOPE},
    {"shuffle",          0, NULL, 'z'},
    {"set",              1, NULL, OPT_SET},
    {"silence-timeout",  1, NULL, 'y'},
    {"speed-hack",       0, NULL, UC_SPEED_HACK},
    {"stderr",           0, NULL, OPT_STDERR},
    {"stdout",           0, NULL, 'c'},
    {"subsong",          1, NULL, 's'},
    {"subsong-timeout",  1, NULL, 'w'},
    {"timeout",          1, NULL, 't'},
    {"verbose",          0, NULL, 'v'},
    {"version",          0, NULL, OPT_VERSION},
    {NULL,               0, NULL, 0}
  };

  uade_config_set_defaults(&uc_cmdline);

  if (!playlist_init(&uade_playlist))
    die("Can not initialize playlist.\n");

#define GET_OPT_STRING(x) if (strlcpy((x), optarg, sizeof(x)) >= sizeof(x)) { die("Too long a string for option %c.\n", ret); }

  while ((ret = getopt_long(argc, argv, "@:1cde:f:gG:hij:k:np:P:rs:S:t:u:vw:x:y:z", long_options, 0)) != -1) {
    switch (ret) {

    case '@':
      listfile = fopen(optarg, "r");
      if (listfile == NULL)
	die("Can not open list file: %s\n", optarg);
      break;

    case '1':
      uade_set_config_option(&uc_cmdline, UC_ONE_SUBSONG, NULL);
      break;

    case 'c':
      strlcpy(uade_output_file_name, "/dev/stdout", sizeof uade_output_file_name);
      /* Output sample data to stdout so do not print anything on stdout */
      uade_terminal_file = stderr;
      break;

    case 'd':
      debug_mode = 1;
      uade_debug_trigger = 1;
      break;
    case 'e':
      GET_OPT_STRING(uade_output_file_format);
      break;

    case 'f':
      GET_OPT_STRING(uade_output_file_name);
      break;

    case 'g':
      uade_info_mode = 1;
      uade_no_audio_output = 1;
      uade_no_text_output = 1;
      uade_set_config_option(&uc_cmdline, UC_ACTION_KEYS, "off");
      break;

    case 'G':
      uade_set_config_option(&uc_cmdline, UC_GAIN, optarg);
      break;

    case 'h':
      print_help();
      exit(0);

    case 'i':
      uade_set_config_option(&uc_cmdline, UC_IGNORE_PLAYER_CHECK, NULL);
      break;

    case 'j':
      uade_jump_pos = strtod(optarg, &endptr);
      if (*endptr != 0 || uade_jump_pos < 0.0)
	die("Invalid jump position: %s\n", optarg);
      break;

    case 'k':
      uade_set_config_option(&uc_cmdline, UC_ACTION_KEYS, optarg);
      break;

    case 'n':
      uade_set_config_option(&uc_cmdline, UC_NO_EP_END, NULL);
      break;

    case 'p':
      uade_set_config_option(&uc_cmdline, UC_PANNING_VALUE, optarg);
      break;

    case 'P':
      GET_OPT_STRING(playername);
      playernamegiven = 1;
      have_modules = 1;
      break;

    case 'r':
      uade_set_config_option(&uc_cmdline, UC_RECURSIVE_MODE, NULL);
      break;

    case 's':
      subsong = strtol(optarg, &endptr, 10);
      if (*endptr != 0 || subsong < 0 || subsong > 255)
	die("Invalid subsong string: %s\n", optarg);
      break;

    case 'S':
      GET_OPT_STRING(scorename);
      break;

    case 't':
      uade_set_config_option(&uc_cmdline, UC_TIMEOUT_VALUE, optarg);
      break;

    case 'u':
      GET_OPT_STRING(uadename);
      break;

    case 'v':
      uade_set_config_option(&uc_cmdline, UC_VERBOSE, NULL);
      break;

    case 'w':
      uade_set_config_option(&uc_cmdline, UC_SUBSONG_TIMEOUT_VALUE, optarg);
      break;

    case 'x':
      uade_set_config_option(&uc_cmdline, UC_EAGLEPLAYER_OPTION, optarg);
      break;

    case 'y':
      uade_set_config_option(&uc_cmdline, UC_SILENCE_TIMEOUT_VALUE, optarg);
      break;

    case 'z':
      uade_set_config_option(&uc_cmdline, UC_RANDOM_PLAY, NULL);
      break;

    case '?':
    case ':':
      exit(1);

    case OPT_BASEDIR:
      uade_set_config_option(&uc_cmdline, UC_BASE_DIR, optarg);
      break;

    case OPT_REPEAT:
      playlist_repeat(&uade_playlist);
      break;

    case OPT_SCAN:
      scanmode = 1;
      /* Set recursive mode in scan mode */
      uade_set_config_option(&uc_cmdline, UC_RECURSIVE_MODE, NULL);
      break;

    case OPT_SCOPE:
      uade_no_text_output = 1;
      uade_set_config_option(&uc_cmdline, UC_USE_TEXT_SCOPE, NULL);
      break;

    case OPT_SET:
      have_song_options = 1;
      strlcpy(songoptions, optarg, sizeof songoptions);
      break;

    case OPT_STDERR:
      uade_terminal_file = stderr;
      break;

    case OPT_VERSION:
      printf("uade123 %s\n", UADE_VERSION);
      exit(0);
      break;

    case UC_AO_OPTION:
    case UC_BUFFER_TIME:
    case UC_FILTER_TYPE:
    case UC_FORCE_LED:
    case UC_FREQUENCY:
    case UC_NORMALISE:
    case UC_RESAMPLER:
      uade_set_config_option(&uc_cmdline, ret, optarg);
      break;

    case UC_CONTENT_DETECTION:
    case UC_CYGWIN_DRIVE_WORKAROUND:
    case UC_DISABLE_TIMEOUTS:
    case UC_ENABLE_TIMEOUTS:
    case UC_HEADPHONES:
    case UC_HEADPHONES2:
    case UC_NTSC:
    case UC_PAL:
    case UC_SPEED_HACK:
      uade_set_config_option(&uc_cmdline, ret, NULL);
      break;

    default:
      die("Impossible option.\n");
    }
  }

  uadeconf_loaded = uade_load_initial_config(uadeconfname, sizeof uadeconfname,
					     &uc_main, &uc_cmdline);

  /* Merge loaded configurations and command line options */
  uc_eff = uc_main;
  uade_merge_configs(&uc_eff, &uc_cmdline);

  if (uadeconf_loaded == 0) {
    debug(uc_eff.verbose, "Not able to load uade.conf from ~/.uade2/ or %s/.\n", uc_eff.basedir.name);
  } else {
    debug(uc_eff.verbose, "Loaded configuration: %s\n", uadeconfname);
  }

  songconf_loaded = uade_load_initial_song_conf(songconfname,
						sizeof songconfname,
						&uc_main, &uc_cmdline);

  if (songconf_loaded == 0) {
    debug(uc_eff.verbose, "Not able to load song.conf from ~/.uade2/ or %s/.\n", uc_eff.basedir.name);
  } else {
    debug(uc_eff.verbose, "Loaded song.conf: %s\n", songconfname);
  }

  /* Read play list from file */
  if (listfile != NULL) {
    while (xfgets(tmpstr, sizeof(tmpstr), listfile) != NULL) {
      if (tmpstr[0] == '#')
	continue;
      if (tmpstr[strlen(tmpstr) - 1] == '\n')
	tmpstr[strlen(tmpstr) - 1] = 0;
      playlist_add(&uade_playlist, tmpstr, uc_eff.recursive_mode, uc_eff.cygwin_drive_workaround);
    }
    fclose(listfile);
    listfile = NULL;
    have_modules = 1;
  }

  /* Read play list from command line parameters */
  for (i = optind; i < argc; i++) {
    /* Play files */
    playlist_add(&uade_playlist, argv[i], uc_eff.recursive_mode, uc_eff.cygwin_drive_workaround);
    have_modules = 1;
  }

  if (scanmode) {
    scan_playlist(&uc_eff);
    exit(0);
  }

  if (have_song_options) {
    set_song_options(&songconf_loaded, songoptions, songconfname, sizeof songconfname);
    exit(0);
  }

  load_content_db(&uc_eff);

  if (uc_eff.random_play)
    playlist_randomize(&uade_playlist);

  if (have_modules == 0) {
    print_help();
    exit(0);
  }

  /* we want to control terminal differently in debug mode */
  if (debug_mode)
    uc_eff.action_keys = 0;

  if (uc_eff.action_keys)
    setup_terminal();

  do {
    DIR *bd = opendir(uc_eff.basedir.name);
    if (bd == NULL)
      dieerror("Could not access dir %s", uc_eff.basedir.name);

    closedir(bd);

    snprintf(configname, sizeof configname, "%s/uaerc", uc_eff.basedir.name);

    if (scorename[0] == 0)
      snprintf(scorename, sizeof scorename, "%s/score", uc_eff.basedir.name);

    if (uadename[0] == 0)
      strlcpy(uadename, UADE_CONFIG_UADE_CORE, sizeof uadename);

    if (access(configname, R_OK))
      dieerror("Could not read %s", configname);

    if (access(scorename, R_OK))
      dieerror("Could not read %s", scorename);

    if (access(uadename, X_OK))
      dieerror("Could not execute %s", uadename);

  } while (0);

  setup_sighandlers();

  memset(&state, 0, sizeof state);

  uade_spawn(&state, uadename, configname);

  if (!audio_init(&uc_eff))
    goto cleanup;

  plistdir = UADE_PLAY_CURRENT;

  while (1) {

    ssize_t filesize;

    /* modulename and songname are a bit different. modulename is the name
       of the song from uadecore's point of view and songname is the
       name of the song from user point of view. Sound core considers all
       custom songs to be players (instead of modules) and therefore modulename
       will become a zero-string with custom songs. */
    char modulename[PATH_MAX];
    char songname[PATH_MAX];

    if (!playlist_get(modulename, sizeof modulename, &uade_playlist, plistdir))
      break;

    plistdir = UADE_PLAY_NEXT;

    state.config = uc_main;
    state.song = NULL;
    state.ep = NULL;

    if (uc_cmdline.verbose)
      state.config.verbose = 1;

    if (playernamegiven == 0) {
      debug(state.config.verbose, "\n");

      if (!uade_is_our_file(modulename, 0, &state)) {
	fprintf(stderr, "Unknown format: %s\n", modulename);
	continue;
      }

      debug(state.config.verbose, "Player candidate: %s\n", state.ep->playername);

      if (strcmp(state.ep->playername, "custom") == 0) {
	strlcpy(playername, modulename, sizeof playername);
	modulename[0] = 0;
      } else {
	snprintf(playername, sizeof playername, "%s/players/%s", uc_cmdline.basedir.name, state.ep->playername);
      }
    }

    if (strlen(playername) == 0) {
      fprintf(stderr, "Error: an empty player name given\n");
      goto cleanup;
    }

    /* If no modulename given, try the playername as it can be a custom song */
    strlcpy(songname, modulename[0] ? modulename : playername, sizeof songname);

    if (!uade_alloc_song(&state, songname)) {
      fprintf(stderr, "Can not read %s: %s\n", songname, strerror(errno));
      continue;
    }

    /* The order of parameter processing is important:
     * 0. set uade.conf options (done before this)
     * 1. set eagleplayer attributes
     * 2. set song attributes
     * 3. set command line options
     */

    if (state.ep != NULL)
      uade_set_ep_attributes(&state);

    if (uade_set_song_attributes(&state, playername, sizeof playername)) {
      debug(state.config.verbose, "Song rejected based on attributes: %s\n",
	    state.song->module_filename);
      uade_unalloc_song(&state);
      continue;
    }

    uade_merge_configs(&state.config, &uc_cmdline);

    /* Now we have the final configuration in "uc". */

    uade_set_effects(&state);

    if ((filesize = stat_file_size(playername)) < 0) {
      fprintf(stderr, "Can not find player: %s (%s)\n", playername, strerror(errno));
      uade_unalloc_song(&state);
      continue;
    }

    debug(state.config.verbose, "Player: %s (%zd bytes)\n", playername, filesize);

    fprintf(stderr, "Song: %s (%zd bytes)\n", state.song->module_filename, state.song->bufsize);

    ret = uade_song_initialization(scorename, playername, modulename, &state);
    switch (ret) {
    case UADECORE_INIT_OK:
      break;

    case UADECORE_INIT_ERROR:
      uade_unalloc_song(&state);
      goto cleanup;

    case UADECORE_CANT_PLAY:
	debug(state.config.verbose, "Uadecore refuses to play the song.\n");
	uade_unalloc_song(&state);
	continue; /* jump to the beginning of playlist loop */

    default:
      die("Unknown error from uade_song_initialization()\n");
    }

    if (subsong >= 0)
      uade_set_subsong(subsong, &state.ipc);

    plistdir = play_loop(&state);

    uade_unalloc_song(&state);

    if (plistdir == UADE_PLAY_FAILURE)
      goto cleanup;
    else if (plistdir == UADE_PLAY_EXIT)
      break;
  }

  debug(uc_cmdline.verbose || uc_main.verbose, "Killing child (%d).\n", uadepid);
  cleanup();
  return 0;

 cleanup:
  cleanup();
  return 1;
}


static void print_help(void)
{
  printf("uade123 %s\n", UADE_VERSION);
  printf(
"\n"
"Usage: uade123 [<options>] <input file> ...\n"
"\n"
"OPTIONS:\n"
"\n"
" -1, --one,          Play at most one subsong per file\n"
" -@ filename, --list=filename,  Read playlist of files from 'filename'\n"
" --ao-option=x:y,    Set option for libao, where 'x' is an audio driver specific\n"
"                     option and 'y' is a value. Note 'x' may not contain\n"
"                     ':' character, but 'y' may. This option can be used\n"
"                     multiple times to specify multiple options.\n"
"                     Example for alsa09 plugin: --ao-option=dev:default\n"
" --buffer-time=x,    Set audio buffer length to x milliseconds. The default\n"
"                     value is determined by the libao.\n"
" -c, --stdout        Write sample data to stdout\n"
" --cygwin,           Cygwin path name workaround: X:\\ -> /cygdrive/X\n"
" --detect-format-by-content, Detect modules strictly by file content.\n"
"                     Detection will ignore file name prefixes.\n"
" --disable-timeouts, Disable timeouts. This can be used for songs that are\n"
"                     known to end. Useful for recording fixed time pieces.\n"
"                     Some formats, such as protracker, disable timeouts\n"
"                     automatically, because it is known they will always end.\n"
" -e format,          Set output file format. Use with -f. wav is the default\n"
"                     format.\n"
" --enable-timeouts,  Enable timeouts. See --disable-timeouts.\n"
" -f filename,        Write audio output into 'filename' (see -e also)\n"
" --filter=model      Set filter model to A500, A1200 or NONE. The default is\n"
"                     A500. NONE means disabling the filter.\n"
" --filter,           Enable filter emulation. It is enabled by default.\n"
" --force-led=0/1,    Force LED state to 0 or 1. That is, filter is OFF or ON.\n"
" --frequency=x,      Set output frequency to x Hz. The default is 44,1 kHz.\n"
" -G x, --gain=x,     Set volume gain to x in range [0, 128]. Default is 1,0.\n"
" -g, --get-info,     Just print playername and subsong info on stdout.\n"
"                     Do not play.\n"
" -h, --help,         Print help\n"
" --headphones,       Enable headphones postprocessing effect.\n"
" --headphones2       Enable headphones 2 postprocessing effect.\n"
" -i, --ignore,       Ignore eagleplayer fileformat check result. Play always.\n"
" -j x, --jump=x,     Jump to time position 'x' seconds from the beginning.\n"
"                     fractions of a second are allowed too.\n"
" -k 0/1, --keys=0/1, Turn action keys on (1) or off (0) for playback control\n"
"                     on terminal. \n"
" -n, --no-ep-end-detect, Ignore song end reported by the eagleplayer. Just\n"
"                     keep playing. This does not affect timeouts. Check -w.\n"
" --ntsc,             Set NTSC mode for playing (can be buggy).\n"
" --pal,              Set PAL mode (default)\n"
" --normalise,        Enable normalise postprocessing effect.\n"
" -p x, --panning=x,  Set panning value in range [0, 2]. 0 is full stereo,\n"
"                     1 is mono, and 2 is inverse stereo. The default is 0,7.\n"
" -P filename,        Set player name\n"
" -r, --recursive,    Recursive directory scan\n"
" --repeat,           Play playlist over and over again\n"
" --resampler=x       Set resampling method to x, where x = default, sinc\n"
"                     or none.\n"
" -s x, --subsong=x,  Set subsong 'x'\n"
" --scan              Scan given files and directories for playable songs and\n"
"                     print their names, one per line on stdout.\n"
"                     Example: uade123 --scan /song/directory\n"
" --set=\"options\"     Set song.conf options for each given song.\n"
" --speed-hack,       Set speed hack on. This gives more virtual CPU power.\n"
" --stderr,           Print messages on stderr rather than stdout\n"
" -t x, --timeout=x,  Set song timeout in seconds. -1 is infinite.\n"
"                     The default is infinite.\n"
" -v,  --verbose,     Turn on verbose mode\n"
" -w x, --subsong-timeout=x,  Set subsong timeout in seconds. -1 is infinite.\n"
"                             Default is 512s\n"
" -x y, --ep-option=y, Use eagleplayer option y. Option can be used many times.\n"
"                      Example: uade123 -x type:nt10 mod.foobar, will play\n"
"                      mod.foobar as a Noisetracker 1.0 module. See eagleplayer\n"
"                      options from the man page.\n"
" -y x, --silence-timeout=x,  Set silence timeout in seconds. -1 is infinite.\n"
"                         Default is 20s\n"
" -z, --shuffle,      Randomize playlist order before playing.\n"
"\n"
"EXPERT OPTIONS:\n"
"\n"
" --basedir=dirname,  Set uade base directory (contains data files)\n"
" -d, --debug,        Enable debug mode (expert only)\n"
" -S filename,        Set sound core name\n"
" --scope,            Turn on Paula hardware register debug mode\n"
" -u uadename,        Set uadecore executable name\n"
"\n");

  print_action_keys();

  printf(
"\n"
"Example: Play all songs under /chip/fc directory in shuffling mode:\n"
"  uade -z /chip/fc/*\n");
}


void print_action_keys(void)
{
  tprintf("ACTION KEYS FOR INTERACTIVE MODE:\n");
  tprintf(" [0-9]         Change subsong.\n");
  tprintf(" '<'           Previous song.\n");
  tprintf(" '.'           Skip 10 seconds forward.\n");
  tprintf(" SPACE, 'b'    Next subsong.\n");
  tprintf(" 'c'           Pause.\n");
  tprintf(" 'f'           Toggle filter (takes filter control away from eagleplayer).\n");
  tprintf(" 'g'           Toggle gain effect.\n");
  tprintf(" 'h'           Print this list.\n");
  tprintf(" 'H'           Toggle headphones effect.\n");
  tprintf(" 'i'           Print module info.\n");
  tprintf(" 'I'           Print hex dump of head of module.\n");
  tprintf(" 'N'           Toggle normalise effect.\n");
  tprintf(" RETURN, 'n'   Next song.\n");
  tprintf(" 'p'           Toggle postprocessing effects.\n");
  tprintf(" 'P'           Toggle panning effect. Default value is 0.7.\n");
  tprintf(" 'q'           Quit.\n");
  tprintf(" 's'           Toggle between random and normal play.\n");
  tprintf(" 'v'           Toggle verbose mode.\n");
  tprintf(" 'x'           Restart current subsong.\n");
  tprintf(" 'z'           Previous subsong.\n");
}


static void setup_sighandlers(void)
{
  struct sigaction act;

  memset(&act, 0, sizeof act);
  act.sa_handler = trivial_sigint;

  if ((sigaction(SIGINT, &act, NULL)) < 0)
    dieerror("can not install signal handler SIGINT");

  memset(&act, 0, sizeof act);
  act.sa_handler = trivial_sigchld;
  act.sa_flags = SA_NOCLDSTOP;

  if ((sigaction(SIGCHLD, &act, NULL)) < 0)
    dieerror("can not install signal handler SIGCHLD");
}


ssize_t stat_file_size(const char *name)
{
  struct stat st;

  if (stat(name, &st))
    return -1;

  return st.st_size;
}


/* test song_end_trigger by taking care of mutual exclusion with signal
   handlers */
int test_song_end_trigger(void)
{
  int ret;
  sigset_t set;

  /* Block SIGINT while handling uade_song_end_trigger */
  if (sigemptyset(&set))
    goto sigerr;
  if (sigaddset(&set, SIGINT))
    goto sigerr;
  if (sigprocmask(SIG_BLOCK, &set, NULL))
    goto sigerr;

  ret = uade_song_end_trigger;
  uade_song_end_trigger = 0;

  /* Unblock SIGINT */
  if (sigprocmask(SIG_UNBLOCK, &set, NULL))
    goto sigerr;

  return ret;

 sigerr:
  die("signal hell\n");
}


static void cleanup(void)
{
  save_content_db();

  if (uadepid != -1) {
    kill(uadepid, SIGTERM);
    uadepid = -1;
  }

  audio_close();
}


static void trivial_sigchld(int sig)
{
  int status;

  if (waitpid(-1, &status, WNOHANG) == uadepid) {
    uadepid = -1;
    uade_terminated = 1;
  }
}


static void trivial_sigint(int sig)
{
  static struct timeval otv = {.tv_sec = 0, .tv_usec = 0};
  struct timeval tv;
  int msecs;

  if (debug_mode == 1) {
    uade_debug_trigger = 1;
    return;
  }

  uade_song_end_trigger = 1;

  /* counts number of milliseconds between ctrl-c pushes, and terminates the
     prog if they are less than 100 msecs apart. */ 
  if (gettimeofday(&tv, NULL)) {
    fprintf(stderr, "Gettimeofday() does not work.\n");
    return;
  }

  msecs = 0;
  if (otv.tv_sec) {
    msecs = (tv.tv_sec - otv.tv_sec) * 1000 + (tv.tv_usec - otv.tv_usec) / 1000;
    if (msecs < 100)
      exit(1);
  }

  otv = tv;
}
