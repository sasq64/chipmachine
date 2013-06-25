/* uade123 - a simple command line frontend for uadecore.

   Copyright (C) 2005-2010 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#define _GNU_SOURCE

#include "uade123.h"

#include "playloop.h"
#include "audio.h"
#include "terminal.h"

#include <assert.h>
#include <errno.h>
#include <dirent.h>
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

#define MAX_AO_OPTIONS 16

int actionkeys = 1;
int buffertime = -1;

int uade_info_mode;
double uade_jump_pos = 0.0;

int uade_no_audio_output;
int uade_no_text_output;

char uade_output_file_format[16];
char uade_output_file_name[PATH_MAX];
struct playlist uade_playlist;
FILE *uade_terminal_file;

static int debug_mode;
static int debug_trigger;

static void print_help(void);
static void setup_sighandlers(void);
static void trivial_sigint(int sig);
static void cleanup(struct uade_state *state);

static char *xfgets(char *s, int size, FILE *stream)
{
	char *ret;
	while (1) {
		ret = fgets(s, size, stream);
		/*
		 * Believe it or not, but it is possible that ret == NULL but
		 * EOF did not happened.
		 */
		if (ret != NULL || feof(stream))
			break;
	}
	return ret;
}

static void scan_playlist(struct uade_state *state)
{
	struct playlist_iterator pli;
	char *songfile;

	playlist_iterator(&pli, &uade_playlist);

	while (1) {
		songfile = playlist_iterator_get(&pli);
		if (songfile == NULL)
			break;

		if (!uade_is_our_file(songfile, state))
			continue;

		songfile = canonicalize_file_name(songfile);

		if (songfile)
			printf("%s\n", songfile);

		free(songfile);
		songfile = NULL;
	}
}


static void set_song_options(const char *songoptions, struct uade_state *state)
{
	struct playlist_iterator pli;
	char *songfile;

	playlist_iterator(&pli, &uade_playlist);

	while (1) {
		songfile = playlist_iterator_get(&pli);
		if (songfile == NULL)
			break;

		if (!uade_set_song_options(songfile, songoptions, state))
			fprintf(stderr, "Could not update song.conf entry for "
				"%s\n", songfile);
	}
}

void test_and_trigger_debug(struct uade_state *state)
{
	if (!debug_trigger)
		return;
	uade_set_debug(state);
	debug_trigger = 0;
}

static void write_sample_data_to_file(const char *filename)
{
	int is_stdout = 0;
	if (strcmp(filename, "-") == 0) {
		filename = "/dev/stdout";
		is_stdout = 1;
	} else if (strcmp(filename, "/dev/stdout") == 0) {
		is_stdout = 1;
	}
	strlcpy(uade_output_file_name, filename, sizeof uade_output_file_name);

	/*
	 * If sample data is written to stdout, write uade123
	 * messages to stderr.
	 */
	if (is_stdout)
		uade_terminal_file = stderr;

	/*
	 * When writing to a file it does not make any sense to have interactive
	 * keys.
	 */
	actionkeys = 0;
}

void set_terminal_file(void)
{
	/*
	 * Note, if write_sample_data_to_file() is called, it must be done
	 * before calling this function.
	 */
	if (uade_no_text_output) {
		uade_terminal_file = fopen("/dev/null", "w");
		if (uade_terminal_file == NULL) {
			fprintf(stderr, "Unable to open /dev/null\n");
			exit(1);
		}
	} else if (uade_terminal_file == NULL) {
		uade_terminal_file = stdout;
	}
}

int main(int argc, char *argv[])
{
	int i;
	char tmpstr[PATH_MAX + 256];
	long subsong = -1;
	FILE *listfile = NULL;
	int have_modules = 0;
	int ret;
	char *endptr;
	struct uade_config *uc_cmdline = NULL;
	char songoptions[256] = "";
	int have_song_options = 0;
	int plistdir;
	int scanmode = 0;

	char **aooptions = NULL;
	int naooptions = 0;

	int recursivemode = 0;
	int randomplay = 0;

	struct uade_state *state;

	enum {
		OPT_FIRST = 0x1FFF,
		OPT_AO_OPTION,
		OPT_BASEDIR,
		OPT_BUFFER_TIME,
		OPT_REPEAT,
		OPT_SCAN,
		OPT_SCOPE,
		OPT_SET,
		OPT_STDERR,
		OPT_VERSION
	};

	struct option long_options[] = {
		{"ao-option",        1, NULL, OPT_AO_OPTION},
		{"basedir",          1, NULL, OPT_BASEDIR},
		{"buffer-time",      1, NULL, OPT_BUFFER_TIME},
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

	uc_cmdline = uade_new_config();
	if (uc_cmdline == NULL)
		uade_die("No memory for uade_config.\n");

	if (!playlist_init(&uade_playlist))
		uade_die("Can not initialize playlist.\n");

#define GET_OPT_STRING(x) if (strlcpy((x), optarg, sizeof(x)) >= sizeof(x)) { uade_die("Too long a string for option %c.\n", ret); }

	while ((ret = getopt_long(argc, argv, "@:1cde:f:gG:hij:k:np:P:rs:S:t:u:vw:x:y:z", long_options, 0)) != -1) {
		switch (ret) {
			
		case '@':
			listfile = fopen(optarg, "r");
			if (listfile == NULL)
				uade_die("Can not open list file: %s\n",
					 optarg);
			break;

		case '1':
			uade_config_set_option(uc_cmdline, UC_ONE_SUBSONG,
					       NULL);
			break;

		case 'c':
			write_sample_data_to_file("/dev/stdout");
			break;

		case 'd':
			debug_mode = 1;
			debug_trigger = 1;
			break;
		case 'e':
			GET_OPT_STRING(uade_output_file_format);
			break;

		case 'f':
			write_sample_data_to_file(optarg);
			break;

		case 'g':
			uade_info_mode = 1;
			uade_no_audio_output = 1;
			uade_no_text_output = 1;
			actionkeys = 0;
			break;

		case 'G':
			uade_config_set_option(uc_cmdline, UC_GAIN, optarg);
			break;

		case 'h':
			print_help();
			exit(0);

		case 'i':
			uade_config_set_option(uc_cmdline,
					       UC_IGNORE_PLAYER_CHECK, NULL);
			break;
      
		case 'j':
			uade_jump_pos = strtod(optarg, &endptr);
			if (*endptr != 0 || uade_jump_pos < 0.0)
				uade_die("Invalid jump position: %s\n", optarg);
			break;

		case 'k':
			actionkeys = strtol(optarg, &endptr, 0);
			if (*endptr != 0 || actionkeys < 0 || actionkeys > 1)
				uade_die("Invalid value: expect 0 or 1. "
					 "Got %s\n", optarg);
			break;

		case 'n':
			uade_config_set_option(uc_cmdline, UC_NO_EP_END, NULL);
			break;

		case 'p':
			uade_config_set_option(uc_cmdline, UC_PANNING_VALUE,
					       optarg);
			break;

		case 'P':
			uade_config_set_option(uc_cmdline, UC_PLAYER_FILE,
					       optarg);
			have_modules = 1;
			break;
			
		case 'r':
			recursivemode = 1;
			break;

		case 's':
			subsong = strtol(optarg, &endptr, 10);
			if (*endptr != 0 || subsong < 0 || subsong > 255)
				uade_die("Invalid subsong string: %s\n", optarg);
			break;

		case 'S':
			uade_config_set_option(uc_cmdline, UC_SCORE_FILE,
					       optarg);
			break;

		case 't':
			uade_config_set_option(uc_cmdline, UC_TIMEOUT_VALUE,
					       optarg);
			break;

		case 'u':
			uade_config_set_option(uc_cmdline, UC_UADECORE_FILE,
					       optarg);
			break;

		case 'v':
			uade_config_set_option(uc_cmdline, UC_VERBOSE, NULL);
			break;

		case 'w':
			uade_config_set_option(uc_cmdline,
					       UC_SUBSONG_TIMEOUT_VALUE,
					       optarg);
			break;

		case 'x':
			uade_config_set_option(uc_cmdline,
					       UC_EAGLEPLAYER_OPTION, optarg);
			break;

		case 'y':
			uade_config_set_option(uc_cmdline,
					       UC_SILENCE_TIMEOUT_VALUE,
					       optarg);
			break;
      
		case 'z':
			randomplay = 1;
			break;

		case '?':
		case ':':
			exit(1);

		case OPT_BASEDIR:
			uade_config_set_option(uc_cmdline, UC_BASE_DIR, optarg);
			break;

		case OPT_REPEAT:
			playlist_repeat(&uade_playlist);
			break;

		case OPT_SCAN:
			scanmode = 1;
			recursivemode = 1;
			break;

		case OPT_SCOPE:
			uade_no_text_output = 1;
			uade_config_set_option(uc_cmdline, UC_USE_TEXT_SCOPE,
					       NULL);
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

		case OPT_AO_OPTION:
			if (aooptions == NULL)
				aooptions = calloc(MAX_AO_OPTIONS + 1,
						   sizeof(aooptions[0]));
			if (aooptions == NULL)
				uade_die("Can not allocate memory for "
					 "ao options\n");
			if (naooptions == MAX_AO_OPTIONS)
				uade_die("Too many ao options\n");
			aooptions[naooptions] = strdup(optarg);
			if (aooptions[naooptions] == NULL)
				uade_die("Can not allocate memory for "
					 "ao option\n");
			naooptions++;
			break;

		case OPT_BUFFER_TIME:
			buffertime = strtol(optarg, &endptr, 0);
			if (*endptr != 0 || buffertime <= 0)
				uade_die("Invalid value: expect > 0. Got %s\n",
					 optarg);
			break;

		case UC_FILTER_TYPE:
		case UC_FORCE_LED:
		case UC_FREQUENCY:
		case UC_RESAMPLER:
			uade_config_set_option(uc_cmdline, ret, optarg);
			break;

		case UC_CONTENT_DETECTION:
		case UC_DISABLE_TIMEOUTS:
		case UC_ENABLE_TIMEOUTS:
		case UC_HEADPHONES:
		case UC_HEADPHONES2:
		case UC_NTSC:
		case UC_PAL:
		case UC_SPEED_HACK:
			uade_config_set_option(uc_cmdline, ret, NULL);
			break;

		default:
			uade_die("Impossible option.\n");
		}
	}

	set_terminal_file();

	state = uade_new_state(uc_cmdline);
	if (state == NULL)
		uade_die("Can not initialize uade state\n");

	test_and_trigger_debug(state);

	/* Read playlist from file */
	if (listfile != NULL) {
		while (xfgets(tmpstr, sizeof(tmpstr), listfile) != NULL) {
			if (tmpstr[0] == '#')
				continue;
			if (tmpstr[strlen(tmpstr) - 1] == '\n')
				tmpstr[strlen(tmpstr) - 1] = 0;
			playlist_add(&uade_playlist, tmpstr, recursivemode);
		}
		fclose(listfile);
		listfile = NULL;
		have_modules = 1;
	}

	/* Read play list from command line parameters */
	for (i = optind; i < argc; i++) {
		/* Play files */
		playlist_add(&uade_playlist, argv[i], recursivemode);
		have_modules = 1;
	}

	if (scanmode) {
		scan_playlist(state);
		exit(0);
	}

	if (have_song_options) {
		set_song_options(songoptions, state);
		exit(0);
	}

	if (randomplay)
		playlist_randomize(&uade_playlist);

	if (have_modules == 0) {
		print_help();
		exit(0);
	}

	/* we want to control terminal differently in debug mode */
	if (debug_mode)
		actionkeys = 0;

	if (actionkeys)
		setup_terminal();

	setup_sighandlers();

	if (!audio_init(state, aooptions))
		goto cleanup;

	plistdir = UADE_PLAY_CURRENT;

	while (1) {
		char modulename[PATH_MAX];
		const struct uade_song_info *info;
		size_t playerfsize = 0;

		if (!playlist_get(modulename, sizeof modulename, &uade_playlist,
				  plistdir))
			break;

		plistdir = UADE_PLAY_NEXT;

		uade_debug(state, "\n");

		if (!uade_is_our_file(modulename, state)) {
			fprintf(stderr, "Unknown format: %s\n", modulename);
			continue;
		}

		ret = uade_play(modulename, subsong, state);
		if (ret < 0) {
			goto cleanup;
		} else if (ret == 0) {
			fprintf(stderr, "Can not play %s\n", modulename);
			continue;
		}

		info = uade_get_song_info(state);
		uade_filesize(&playerfsize, info->playerfname);
		uade_debug(state, "Player: %s (%zd bytes)\n",
			   info->playerfname, playerfsize);
		fprintf(stderr, "Song: %s (%zd bytes)\n",
			info->modulefname, info->modulebytes);

		plistdir = play_loop(state);

		if (uade_stop(state)) {
			uade_cleanup_state(state);
			state = uade_new_state(uc_cmdline);
			if (state == NULL)
				uade_die("uade_stop() failed, new state can "
					 "not created\n");
			uade_debug(state, "uade_stop() failed. New uade state "
				   "created.\n");
			continue;
		}

		if (plistdir == UADE_PLAY_FAILURE)
			goto cleanup;
		else if (plistdir == UADE_PLAY_EXIT)
			break;
	}

	cleanup(state);
	free(uc_cmdline);
	return 0;
  
cleanup:
	cleanup(state);
	free(uc_cmdline);
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
" -c, --stdout        Write sample data to stdout. Implies non-interactive mode.\n"
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
" -f filename,        Write audio data into given file. If filename is \"-\"\n"
"                     or \"/dev/stdout\", sample data is written to stdout.\n"
"                     This implies non-interactive mode. See -e option also.\n"
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
"                     Decimal values are valid.\n"
" -k 0/1, --keys=0/1, Turn action keys on (1) or off (0) for playback control\n"
"                     on terminal. \n"
" -n, --no-ep-end-detect, Ignore song end reported by the eagleplayer. Just\n"
"                     keep playing. This does not affect timeouts. Check -w.\n"
" --ntsc,             Set NTSC mode for playing (can be buggy).\n"
" --pal,              Set PAL mode (default)\n"
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
	tprintf("ACTION KEYS FOR INTERACTIVE MODE:\n"
		" [0-9]         Change subsong\n"
		" CURSORS       Cursors left and right seek 10 seconds.\n"
		"               Cursors down and up seek 1 minute.\n"
		" '<'           Previous song\n"
		" '.'           Seek 10 seconds forward (same as cursor right)\n"
		" 'b'           Next subsong\n"
		" 'c', SPACE    Pause\n"
		" 'f'           Toggle filter (takes filter control away from eagleplayer)\n"
		" 'g'           Toggle gain effect\n"
		" 'h'           Print this list\n"
		" 'H'           Toggle headphones effect\n"
		" 'i'           Print module info\n"
		" 'I'           Print hex dump of head of module\n"
		" RETURN, '>'   Next song\n"
		" 'p'           Toggle postprocessing effects\n"
		" 'P'           Toggle panning effect. Default value is 0.7.\n"
		" 'q'           Quit\n"
		" 's'           Toggle between random and linear play\n"
		" 'v'           Toggle verbose mode\n"
		" 'x'           Restart current subsong\n"
		" 'z'           Previous subsong\n");
}


static void setup_sighandlers(void)
{
	struct sigaction act = {.sa_handler = trivial_sigint};
	if (sigaction(SIGINT, &act, NULL))
		uade_die_error("can not install signal handler SIGINT");
}

static void cleanup(struct uade_state *state)
{
	uade_cleanup_state(state);
	audio_close();
}

static void trivial_sigint(int sig)
{
	static struct timeval otv = {.tv_sec = 0, .tv_usec = 0};
	struct timeval tv;

	if (debug_mode) {
		debug_trigger = 1;
		return;
	}

	/*
	 * Counts number of milliseconds between ctrl-c pushes, and terminates
	 * the prog if they are less than 100 msecs apart.
	 */
	if (gettimeofday(&tv, NULL)) {
		fprintf(stderr, "Gettimeofday() does not work.\n");
		return;
	}
	if (otv.tv_sec) {
		int msecs = (tv.tv_sec - otv.tv_sec) * 1000 + (tv.tv_usec - otv.tv_usec) / 1000;
		if (msecs < 100)
			exit(1);
	}
	otv = tv;
}
