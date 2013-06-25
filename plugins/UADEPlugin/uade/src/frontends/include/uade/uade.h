#ifndef _LIB_UADE_H_
#define _LIB_UADE_H_

#ifdef __cplusplus
extern "C"
{
#endif 

/*
 * uade.h defines the interface for libuade. Developers should use
 * definitions from this file. It is possible to use functions defined
 * in other files, but it is discouraged for compatibility reasons.
 * Most relevant functions are defined explicitly in this file.
 *
 * NOTE: You should catch SIGPIPE because a write for the uadecore process
 * might cause a SIGPIPE if the uadecore process dies.
 *
 * You may only call uade_* functions for one uade_state only from one thread
 * at a time. If you have multiple uade_states, each can be called
 * simultaneously. Note, especially be careful not to call uade_is_our_file()
 * while playing in another thread with same uadestate.
 *
 * For the simplest players, the workflow to decode Amiga song is:
 * 1. state = uade_new_state(NULL);
 * 2. uade_play(fname, -1, state);  (Or, use uade_play_from_buffer.)
 * 3. Optionally, uade_get_song_info(state);
 * 4. uade_read(buffer, buffersize, state);
 * 5. uade_stop(state);  (Or, uade_cleanup_state(state), if no more songs are
 *                        played with the given state).
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <uade/options.h>

#define UADE_CHANNELS 2
#define UADE_BYTES_PER_SAMPLE 2
#define UADE_BYTES_PER_FRAME (UADE_CHANNELS * UADE_BYTES_PER_SAMPLE)

struct uade_file {
	char *name;  /* filename */
	char *data;  /* file data, can be NULL */
	size_t size;
};

struct uade_subsong_info {
	int cur; /* current subsong: min <= cur <= max */
	int min; /* minimum subsong number */
	int def; /* start from this subsong: min <= def <= max */
	int max; /* maximum subsong number */
};

/* Maximum number of bytes, including '\0', in file name extension */
#define UADE_MAX_EXT_LEN 16

struct eagleplayer;

struct uade_detection_info {
	int custom;                 /* 1 if the file is a custom, 0 otherwise */

	/* 1 if detected by data content, not name. 0 otherwise. */
	int content;

	char ext[UADE_MAX_EXT_LEN]; /* File extension from eagleplayer.conf */
	struct eagleplayer *ep;     /* Don't touch */
};

struct uade_song_info {
	struct uade_subsong_info subsongs;
	struct uade_detection_info detectioninfo;
	size_t modulebytes;       /* Size of song file in bytes */
	char modulemd5[33];       /* Hexadecimal string of the md5 sum of the
				     song file */
	double duration;          /* Duration in seconds. Zero if unknown. */
	int64_t subsongbytes;     /* Bytes synthesized in the current subsong */
	int64_t songbytes;        /* Bytes synthesized in the current song,
				     counting all the subsongs */
	char modulefname[PATH_MAX];
	char playerfname[PATH_MAX];
	char formatname[256]; /* insert example */
	char modulename[256]; /* insert example */

	/*
	 * By convention, this is set to "custom" if the song is a custom.
	 * libuade doesn't like if you set "custom" 
	 */
	char playername[256];
};

enum uade_song_info_type {
	UADE_MODULE_INFO = 0,
	UADE_HEX_DUMP_INFO,
	UADE_NUMBER_OF_INFOS,
};

#define RMC_MAGIC "rmc\x00\xfb\x13\xf6\x1f\xa2"
#define RMC_MAGIC_LEN 9

enum uade_seek_mode {
	UADE_SEEK_NOT_SEEKING = 0, /* Must be 0 */
	UADE_SEEK_SONG_RELATIVE,
	UADE_SEEK_SUBSONG_RELATIVE,
	UADE_SEEK_POSITION_RELATIVE,
};

enum uade_option {
	UC_NO_OPTION = 0x1000,
	UC_BASE_DIR,
	UC_CONTENT_DETECTION,
	UC_DISABLE_TIMEOUTS,
	UC_ENABLE_TIMEOUTS,
	UC_EAGLEPLAYER_OPTION,
	UC_FILTER_TYPE,
	UC_FORCE_LED_OFF,
	UC_FORCE_LED_ON,
	UC_FORCE_LED,
	UC_FREQUENCY,
	UC_GAIN,
	UC_HEADPHONES,
	UC_HEADPHONES2,
	UC_IGNORE_PLAYER_CHECK,
	UC_NO_FILTER,
	UC_NO_HEADPHONES,
	UC_NO_PANNING,
	UC_NO_POSTPROCESSING,
	UC_NO_EP_END,
	UC_NTSC,
	UC_ONE_SUBSONG,
	UC_PAL,
	UC_PANNING_VALUE,
	UC_PLAYER_FILE,
	UC_RESAMPLER,
	UC_SCORE_FILE,
	UC_SILENCE_TIMEOUT_VALUE,
	UC_SPEED_HACK,
	UC_SUBSONG_TIMEOUT_VALUE,
	UC_TIMEOUT_VALUE,
	UC_UADECORE_FILE,
	UC_UAE_CONFIG_FILE,
	UC_USE_TEXT_SCOPE,
	UC_VERBOSE,
};

/* Audio effects */
typedef enum {
	UADE_EFFECT_ALLOW,
	UADE_EFFECT_GAIN,
	UADE_EFFECT_HEADPHONES,
	UADE_EFFECT_HEADPHONES2,
	UADE_EFFECT_PAN,
} uade_effect_t;

struct bencode;
struct uade_config;
struct uade_event;
struct uade_state;

/*
 * Free resources of a state. This also implies uade_stop().
 * A call does nothing if state == NULL.
 */
void uade_cleanup_state(struct uade_state *state);

/*
 * Return a config structure for setting playback options. The structure is
 * initialized to default values, so calling uade_config_set_defaults() is
 * not necessary for the returned structure. If no memory is available,
 * NULL is returned. The returned structure must be freed with free().
 */
struct uade_config *uade_new_config(void);

void uade_config_set_defaults(struct uade_config *uc);
void uade_config_set_option(struct uade_config *uc, enum uade_option opt,
			    const char *value);

/*
 * Toggles config boolean, e.g. opt == UC_VERBOSE, and returns the new boolean
 * value. Returns 1 when boolean is true, returns 0 when boolean is false,
 * and returns -1 when opt is not a boolean or toggling is not supported.
 * Currently, it is only valid to call this when opt is one of UC_VERBOSE
 * or UC_FORCE_LED.
 */
int uade_config_toggle_boolean(struct uade_config *uc, enum uade_option opt);

void uade_effect_disable(struct uade_state *state, uade_effect_t effect);
void uade_effect_disable_all(struct uade_state *state);
void uade_effect_enable(struct uade_state *state, uade_effect_t effect);
int uade_effect_is_enabled(const struct uade_state *state,
			   uade_effect_t effect);
void uade_effect_toggle(struct uade_state *state, uade_effect_t effect);
/* Effect-specific knobs */
void uade_effect_gain_set_amount(struct uade_state *state, float amount);
void uade_effect_pan_set_amount(struct uade_state *state, float amount);

const char *uade_event_name(const struct uade_event *event);

/*
 * Get's pointer struct uade_config that is used to play the current song.
 * Note, this always returns a non-NULL pointer even we are not in playing mode.
 */
struct uade_config *uade_get_effective_config(struct uade_state *state);

/*
 * This is called to drive the playback. This should be called when there is
 * data to be read from fd returned by uade_get_fd(). If there is no data,
 * the function will block until an event takes place.
 *
 * Returns 0 on success.
 *
 * Returns -1 on failure. In this case, the uade state must be
 * freed with uade_cleanup_state() and created again with uade_new_state().
 */
int uade_get_event(struct uade_event *event, struct uade_state *state);

/* Returns fd to poll for new events */
int uade_get_fd(const struct uade_state *state);

/*
 * Returns filter state: 0 when filter is off, 1 otherwise.
 * Also see uade_send_filter_command().
 */
int uade_get_filter_state(const struct uade_state *state);

/*
 * uade_read() synthesizes samples. This is the main API call to get audio data.
 *
 * Returns -1 on error. Error indicates playback must be terminated.
 *
 * Returns 0 on song end or when bytes == 0. Note, calling uade_read()
 * again after song end returns 0 again, indefinitely.
 *
 * Returns a positive value to indicate a number of sample bytes at 'data'.
 *
 * Sample data is a sequence of frames. Each frame consists of two int16_t
 * samples. The first sample in the frame is for the left channel, and the
 * second sample is for the right channel.
 */
ssize_t uade_read(void *data, size_t bytes, struct uade_state *state);

/*
 * Various notifications that libuade and uadecore send. Currently they are
 * purely informational notifications, so you don't have to handle them.
 * They are very useful for debugging, though. Take a look at uade123/playloop.c
 * to see how UADE_NOTIFICATION_* notifications are handled.
 */
enum uade_notification_type {
	/*
         * UADE_NOTIFICATION_MESSAGE is a purely informational notification.
	 * For example, the message can indicate to you that amiga run-time
	 * did something very stupid like inserting bad values into HW register.
	 * One common message is "LED is OFF/ON" which means that software
	 * controlled hardware audio filter was turned off or on.
	 */
	UADE_NOTIFICATION_MESSAGE,

	/*
         * UADE_NOTIFICATION_SONG_END is a purely informational notification.
	 * This message indicates whether the song ended in a good way or not.
         *
	 * The usual good ways: "player" which means that the amiga player
	 * reported song end. "No more subsongs left" which means that
	 * libuade reports there no more subsongs to be played.
         *
	 * Some bad ways: "score crashed" and "score died" mean the Amiga
	 * run-time software crashed/died. "module check failed" implies that
	 * libuade heuristically implied that a given file should be played
	 * with a specific Amiga player, but the player rejected the song.
	 */
	UADE_NOTIFICATION_SONG_END, /* Purely informational */
};

struct uade_notification_song_end {
	int happy;            /* Non-zero if the song end was happy */
	/* Non-zero iff playing stops now (the last subsong ended, or error) */
	int stopnow;
	int subsong;          /* Subsong number */
	int64_t subsongbytes; /* Number of bytes in the subsong */
	char *reason;         /* Textual description of the cause */
};

struct uade_notification {
	enum uade_notification_type type;
	union {
		char *msg;
		struct uade_notification_song_end song_end;
	};
};

/*
 * uade_read_notification() returns 0, if there are no notifications.
 * Returns 1, if there is a notification, and copies that notification
 * over 'notification'. You must call uade_cleanup_notification() for the
 * returned notification after use, otherwise you leak memory.
 *
 * You can call uade_read_notification after uade_read(), but you don't
 * have to.
 */
int uade_read_notification(struct uade_notification *notification,
			   struct uade_state *state);

/*
 * Call uade_cleanup_notification() after use for every notification you got
 * from uade_read_notification().
 */
void uade_cleanup_notification(struct uade_notification *notification);

/* Returns sampling rate of current state */
int uade_get_sampling_rate(const struct uade_state *state);

/*
 * uade_get_song_info() can be called after successful call to uade_play()
 * to get information about module, player and format name, and the
 * subsongs (min, max, current subsong), etc.
 */
const struct uade_song_info *uade_get_song_info(const struct uade_state *state);

/*
 * Returns non-zero if fname can be played with uade. Returns zero if it can
 * not be played. Note, playability depends sometimes on the file name because
 * not all formats are detected by contents.
 *
 * Warning, this function is slow and uses unreliable heuristics.
 * See uade_is_rmc().
 */
int uade_is_our_file(const char *fname, struct uade_state *state);

/*
 * uade_is_our_file_from_buffer()
 *
 * Same as uade_is_our_file, but gets data from buf with size bytes.
 * Does not work with multifile songs.
 * Optional file name can be provided in "fname" parameter for format detection.
 * fname can be NULL. Only the file extension of the name matters.
 * fname parameter is necessary for formats where content detection is not
 * supported.
 *
 * Warning, this function is slow and uses unreliable heuristics.
 * See uade_is_rmc().
 */
int uade_is_our_file_from_buffer(const char *fname, const void *buf,
				 size_t size, struct uade_state *state);

/*
 * uade_is_rmc() returns 1, if the buffer has RMC prefix code, 0 otherwise.
 *
 * Note, you should use this function for file type detection if you only
 * need support for Amiga songs wrapped in RMC format.
 */
int uade_is_rmc(const char *buf, size_t size);

/*
 * uade_is_rmc_file() returns 1, if the file pointed to by fname is an rmc file.
 * Otherwise 0 is returned. See the note about uade_is_rmc() function.
 */
int uade_is_rmc_file(const char *fname);

/* Return 1 if uade is in verbose mode (for debugging), 0 otherwise. */
int uade_is_verbose(const struct uade_state *state);

/*
 * Returns pointer to RMC data structure of the current song if it is played
 * from an RMC container.
 */
struct bencode *uade_get_rmc_from_state(const struct uade_state *state);

/*
 * Creates a new playback context. This is necessary to play a song.
 * Context is initialized with default configs if uc == NULL, otherwise
 * the given configs are used. The fuction returns NULL on error.
 * Each context is completely independent. A context is not thread-safe,
 * but different contexts can be used simultaneously in different threads.
 */
struct uade_state *uade_new_state(const struct uade_config *uc);

/*
 * uade_load_amiga_file() loads a file by using AmigaOS path search.
 * 'name' is the file name. 'playerdir' is the directory containing
 * eagleplayers. Note, this function is mainly used in callbacks related to
 * uade_set_amiga_loader().
 */
struct uade_file *uade_load_amiga_file(const char *name,
				       const char *playerdir,
				       struct uade_state *state);

int uade_next_subsong(struct uade_state *state);

/*
 * To play a file given an uade_state, call uade_play() with following
 * parameters:
 * - 'fname' is the path name of the song
 * - 'subsong' is the subsong to play, pass -1 for the the default subsong
 *
 * Returns -1 on fatal error. In this case uade state must be cleanup
 * with uade_cleanup_state() and created again with uade_new_state().
 *
 * Returns 0 if the song can not be played.
 *
 * Returns 1 if the song can be played. In this case uade_stop() must be
 * called before calling uade_play() again.
 */
int uade_play(const char *fname, int subsong, struct uade_state *state);

/*
 * uade_play_from_buffer()
 *
 * Same as uade_play, but gets data from 'buf' with 'size' bytes.
 * Does not work with multifile songs.
 * Optional file name can be provided in 'fname' parameter for format detection.
 * 'fname' can be NULL. Only the file extension of the name matters.
 * 'fname' parameter is necessary for formats where content detection is not
 * supported. 'buf' can be freed after call.
 */
int uade_play_from_buffer(const char *fname, const void *buf, size_t size,
			  int subsong, struct uade_state *state);

/*
 * This forces emulated Amiga to change filter to the given newstate.
 * The filter is turned off if newstate == 0, otherwise the filter is turned on.
 * Returns 0 on success, -1 otherwise. Note, successful return means that
 * the command was queued for sending to the emulator. The next batch of
 * sample data may still be filtered with the old state.
 */
int uade_set_filter_state(struct uade_state *state, int newstate);

/*
 * This can be used to monitor and control Amiga file loading from the music
 * player. 'amigaloader' can be set to NULL to disable the callback.
 * 'context' is an arbitrary pointer passed from the application.
 * Note: this is used in rmc command to collect files related to a
 * given Amiga song.
 */
void uade_set_amiga_loader(struct uade_file * (*amigaloader)(const char *name, const char *playerdir, void *context, struct uade_state *state),
			   void *context,
			   struct uade_state *state);

void uade_set_debug(struct uade_state *state);
int uade_set_song_options(const char *songfile, const char *songoptions,
			  struct uade_state *state);

/*
 * Seek to a given time location and/or subsong.
 *
 * If 'mode' is UADE_SEEK_SONG_RELATIVE, 'seconds' is a non-negative value
 * measured from the beginning of the first subsong.
 * 'subsong' is ignored.
 * If 'mode' is UADE_SEEK_SUBSONG_RELATIVE, 'seconds' is a non-negative value
 * measured from the beginning of the given subsong.
 * 'subsong' == -1 means the current subsong.
 * If 'mode' is UADE_SEEK_POSITION_RELATIVE, 'seconds' is measured from the
 * current position (can be a negative value). 'subsong' is ignored.
 * Returns 0 on success, -1 on error.
 *
 * UADE_SEEK_SUBSONG_RELATIVE and UADE_SEEK_POSITION_RELATIVE seeking stops
 * the seek at a subsong boundary. UADE_SEEK_SONG_RELATIVE does not stop at
 * boundaries.
 *
 * Note, seek is not complete when the call returns. It can take a long
 * time until the seek completes. One must run the normal event handling
 * loop during the seek.
 *
 * Returns on 0 on success, -1 on failure.
 *
 * Examples:
 * - Seek to time pos 66.6s: uade_seek(UADE_SEEK_SONG_RELATIVE, 66.6, 0,
 *                                     state)
 * - Seek to subsong 2: uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, 2, state)
 * - Skip 10 secs forward: uade_seek(UADE_SEEK_POSITION_RELATIVE, 10, 0, state)
 */
int uade_seek(enum uade_seek_mode whence, double seconds, int subsong,
	      struct uade_state *state);

/* Same as uade_seek(), but seeks to a given sample (aka frame) location */
int uade_seek_samples(enum uade_seek_mode whence, ssize_t samples, int subsong,
		      struct uade_state *state);

/* Returns 1 if uade is in seek mode, 0 otherwise. */
int uade_is_seeking(const struct uade_state *state);

/*
 * Return current song position in seconds. Function returns a negative value
 * for non-rmc songs. Time is always returned with RMC files.
 * 'whence' determines whether time is relative to the beginning of the first
 * subsong (UADE_SEEK_SONG_RELATIVE) or the
 * current subsong (UADE_SEEK_SUBSONG_RELATIVE).
 */
double uade_get_time_position(enum uade_seek_mode whence,
			      const struct uade_state *state);

int uade_song_info(char *info, size_t maxlen, const char *filename,
		   enum uade_song_info_type type);

/*
 * This should be called after a successful uade_play().
 *
 * Returns 0 on success.
 *
 * Returns -1 on error. In this case the state must be freed with
 * uade_cleanup_state() and created again with uade_new_state().
 */
int uade_stop(struct uade_state *state);


/*
 * Helper functions for RMC. One could do the following by just using
 * bencode-tools.
 */

/*
 * Get arbitrary file from the container. The returned file must be freed with
 * uade_file_free().
 */
struct uade_file *uade_rmc_get_file(const struct bencode *rmc,
				    const char *name);

/* Get main module file from the container */
int uade_rmc_get_module(struct uade_file **module, const struct bencode *rmc);

/*
 * Return the meta data dictionary of the RMC
 */
struct bencode *uade_rmc_get_meta(const struct bencode *rmc);

/*
 * Return a dictionary that contains subsong numbers as keys and subsong
 * lengths (measured in milliseconds) as values. This function always
 * returns a valid pointer (not NULL).
 *
 * Note: This is equivalent to:
 * ben_dict_get_by_str(uade_rmc_get_meta(rmc), "subsongs")
 */
const struct bencode *uade_rmc_get_subsongs(const struct bencode *rmc);

/*
 * Return song length in seconds.
 */
double uade_rmc_get_song_length(const struct bencode *rmc);

/*
 * Parses a given data with size. Returns an RMC data structure if the
 * data is valid, otherwise NULL.
 */
struct bencode *uade_rmc_decode(const void *data, size_t size);

/* Same as uade_rmc_decode, but reads data from file pointed to by fname. */
struct bencode *uade_rmc_decode_file(const char *fname);

/* Put a new file to the RMC data structure */
int uade_rmc_record_file(struct bencode *rmc, const char *name,
			 const void *data, size_t len);

size_t uade_atomic_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * Create a 'struct uade_file' that is used to present and contain name
 * and data of a file. The structure has following members:
 *
 * char *name, the file name
 * char *data, the contents of the file. This can be NULL.
 * size_t size, the size of data
 *
 * data is copied into the 'struct uade_file' so the given data can be changed
 * or freed immediately after call to uade_file().
 *
 * The returned data must be freed with uade_file_free().
 *
 * Returns NULL on error (out of memory).
 */
struct uade_file *uade_file(const char *name, const void *data, size_t size);

void uade_file_free(struct uade_file *f);

/* Create uade_file from a file name. Returns NULL on error. */
struct uade_file *uade_file_load(const char *name);

/*
 * Read file and return the data. The data must be freed with free().
 * If fsize != NULL, fsize contains the number of read bytes. Consequently,
 * a non-NULL returned pointer will contain at least fsize bytes of valid
 * data.
 */
void *uade_read_file(size_t *fsize, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
