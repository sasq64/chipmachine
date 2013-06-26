/**
 * @ingroup   sc68_lib
 * @file      sc68/sc68.h
 * @author    Benjamin Gerard
 * @date      2003/08/07
 * @brief     sc68 API header.
 *
 */

/* Copyright (C) 1998-2011 Benjamin Gerard */

#ifndef _SC68_SC68_H_
#define _SC68_SC68_H_

#ifndef SC68_EXTERN
# ifdef __cplusplus
#  define SC68_EXTERN extern "C"
# else
#  define SC68_EXTERN extern
# endif
#endif

#ifndef SC68_API
/* Building */
# ifdef SC68_EXPORT
#  if defined(DLL_EXPORT) && defined(HAVE_DECLSPEC)
#   define SC68_API __declspec(dllexport)
#  elif defined(HAVE_VISIBILITY)
#   define SC68_API SC68_EXTERN __attribute__ ((visibility("default")))
#  else
#   define SC68_API SC68_EXTERN
#  endif
/* Using */
# else
#  if defined(SC68_DLL)
#   define SC68_API __declspec(dllimport)
#  else
#   define SC68_API SC68_EXTERN
#  endif
# endif
#endif

/**
 *  @defgroup  sc68_lib  sc68 library
 *  @ingroup   api68
 *
 *  This API provides functions to use sc68 libraries efficiently.
 *
 *  @par Multi-threading concern
 *
 *  sc68 should now be thread safe. At least concerning the functions
 *  with a sc68_t parameter. As no torture test has been run against
 *  it this should be considered as quiet experimental.
 *
 *  @par Quick start
 *
 *  Here is a quiet minimalist piece of code to use basics of sc68
 *  library:
 *
 *  @code
 *  #include <sc68/sc68.h>
 *  #include <FILE.h> // for fwrite()
 *
 *  sc68_t * sc68 = 0;
 *  char buffer[512*4];
 *  int  n;
 *
 *  // Initialise the library.
 *  // You should consider using sc68_init_t struct.
 *  if (sc68_init(0)) goto error;
 *
 *  // Create an emulator instance.
 *  // You should consider using sc68_create_t struct.
 *  if (sc68 = sc68_create(0), !sc68) goto error;
 *
 *  // Load an sc68 file.
 *  if (sc68_load_url(sc68, fname)) {
 *    goto error;
 *  }
 *
 *  // Set track and loop (optionnal).
 *  sc68_play(sc68, 1, 1);
 *
 *  // Loop until the end of disk. You can use SC68_LOOP to wait the end
 *  // of the track. Notice that SC68_ERROR set all bits and make the loop
 *  // break too.
 *  while ( (n=sizeof(buffer)>>2), ! (sc68_process(sc68, buffer, &n) & SC68_END) ) {
 *    fwrite(buffer, 1, n, stdout); // copy PCM to stdout
 *  }
 *
 *  // Close eject the current disk.
 *  sc68_close(sc68);
 *
 * error:
 *
 *  // Destroy sc68 instance.
 *  sc68_destroy(sc68);
 *
 *  // Shutdown sc68 library.
 *  sc68_shutdown();
 *
 * @endcode
 *
 *  @{
 */

#ifndef SC68_API
/**
 *  sc68 exported symbol.
 *
 *  Define special atribut for exported symbol.
 *
 *  - empty: static or classic .so library
 *  - __attribute__ (visibility("default"))): gcc support visibility.
 *  - __declspec(dllexport): creating a win32 DLL.
 *  - __declspec(dllimport): using a win32 DLL.
 */
#define SC68_API extern
#endif

/** API information. */
typedef struct _sc68_s sc68_t;

/** API disk. */
typedef void * sc68_disk_t;

/**
 *  Message function.
 *
 *  @param  cat   @ref msg68_cat_e "message category"
 *  @param  sc68  sc68 instance
 *  @param  fmt   printf like format string
 *  @param  list  variable argument list
 * */
typedef void (*sc68_msg_t)();

/**
 *  API initialization parameters.
 *
 *    The sc68_init_t must be properly filled before calling the
 *    sc68_init() function.
 *
 */
typedef struct {

  /** message handler. */
  sc68_msg_t msg_handler;

  /** debug mask (set bit to clear in debugmsg68). */
  int debug_clr_mask;

  /** debug mask (set bit to set in debugmsg68). */
  int debug_set_mask;

  /** number of arguments in command line (modified) */
  int argc;

  /** command line arguments */
  char ** argv;

} sc68_init_t;

/**
 *  Instance creation parameters.
 */
typedef struct {
  /** sampling rate in hz (non 0 value overrides config default).
   *  The real used value is set by sc68_create() function.
   */
  unsigned int sampling_rate;

  /** short name. */
  const char * name;

  /** 68k memory size (2^log2mem). */
  int log2mem;

  /** Run  68k emulator in debug mode (enable memory access trace). */
  int emu68_debug;

  /** User private data. */
  void * cookie;

} sc68_create_t;

/**
 * metatag struct.
 * @warning  MUST match the tag68_t struct defined in tag68.h.
 */
typedef struct {
  char * key;                     /**< Tag name.  */
  char * val;                     /**< Tag value. */
} sc68_tag_t;

/**
 * Music common information.
 */
typedef struct {
  unsigned track;       /**< Current/Default track [1..99].     */
  unsigned time_ms;     /**< Track/Disk duration.               */
  char     time[12];    /**< Time in format TT MM:SS.           */
  unsigned ym:1;        /**< Music uses YM-2149 (ST).           */
  unsigned ste:1;       /**< Music uses STE specific hardware.  */
  unsigned amiga:1;     /**< Music uses Paula Amiga hardware.   */
  const char * hw;      /**< Hardware name.                     */
  int          tags;    /**< Number of tags.                    */
  sc68_tag_t * tag;
} sc68_cinfo_t;

/**
 * Music information.
 */
typedef struct {
  int tracks;            /**< number of tracks [1..99].          */
  unsigned start_ms;     /**< Absolute start time in disk in ms. */
  unsigned loop_ms;      /**< Length of track loop in ms.        */

  unsigned int addr;     /**< Laod address.                      */
  unsigned int rate;     /**< Replay rate.                       */
  char * replay;         /**< replay name.                       */

  sc68_cinfo_t dsk;      /**< disk info.                         */
  sc68_cinfo_t trk;      /**< track info (MUST BE just after dsk.*/

  char * album;          /**< Points to album's title tag.       */
  char * title;          /**< Points to track's title tag.       */
  char * artist;         /**< Points to track's artist tag.      */
} sc68_music_info_t;


/**
 * Return code (as returned by sc68_process() function)
 */
enum sc68_code_e {
  SC68_IDLE   = (1<<0),   /**< Set if no emulation has been runned. */
  SC68_CHANGE = (1<<1),   /**< Set if track has changed.            */
  SC68_LOOP   = (1<<2),   /**< Set if track has looped.             */
  SC68_END    = (1<<3),   /**< Set if track(s) has ended.           */
  SC68_SEEK   = (1<<4),   /**< Set if currently seeking.            */
  /* */
  SC68_OK     =  0,                     /**< Success return code.  */
  SC68_ERROR  = ~0                      /**< Failure return code.  */
};

/**
 * sc68 sampling rate values in hertz (hz).
 */
enum sc68_spr_e {
  SC68_SPR_QUERY   = -1, /**< Query default or current sampling rate. */
  SC68_SPR_DEFAULT =  0  /**< Default sampling rate.                  */
};

/**
 * @name API control functions.
 *  @{
 */

SC68_API
/**
 * Get version number
 */
int sc68_version(void);

SC68_API
/**
 * Get version string
 *
 */
const char * sc68_versionstr(void);

SC68_API
/**
 * Initialise sc68 API.
 *
 * @param   init  Initialization parameters.
 *
 * @return  error-code
 * @retval   0  Success
 * @retval  -1  Error
 *
 * @see sc68_shutdown()
 */
int sc68_init(sc68_init_t * init);

SC68_API
/**
 * Destroy sc68 API.
 *
 */
void sc68_shutdown(void);

SC68_API
/**
 * Create sc68 instance.
 *
 * @param   creation  Creation parameters.
 *
 * @return  Pointer to created sc68 instance.
 * @retval  0  Error.
 *
 * @see sc68_destroy()
 */
sc68_t * sc68_create(sc68_create_t * create);

SC68_API
/**
 * Destroy sc68 instance.
 *
 * @param   sc68  sc68 instance to destroy.
 *
 * @see sc68_create()
 * @note  It is safe to call with null api.
 */
void sc68_destroy(sc68_t * sc68);

SC68_API
/**
 * Get instance name.
 *
 * @param   sc68  sc68 instance to destroy.
 * @return  sc68 instance name.
 */
const char * sc68_name(sc68_t * sc68);

SC68_API
/**
 * Get user private data pointer.
 *
 *   The sc68_cookie_ptr() function get a pointer to the user private
 *   data. This pointer can be use to get a modify this sc68 instance
 *   private data.
 *
 * @param   sc68  sc68 instance.
 * @return        a pointer to the user data inside this sc68 instance.
 * @retval  0     on error
 */
void ** sc68_cookie_ptr(sc68_t * sc68);

SC68_API
/**
 * Set/Get sampling rate.
 *
 * @param   sc68  sc68 api or 0 for library default.
 * @param   hz    @ref sc68_spr_e "sampling rate" (in hz).
 *
 * @return  Actual @ref sc68_spr_e "sampling rate"
 * @retval  -1  on error
 * @retval  >0  on success
 */
int sc68_sampling_rate(sc68_t * sc68, int hz);

SC68_API
/**
 * Set share data path.
 *
 * @param   sc68  sc68 instance.
 * @param   path  New shared data path.
 */
void sc68_set_share(sc68_t * sc68, const char * path);

SC68_API
/**
 * Set user data path.
 *
 * @param   sc68  sc68 instance.
 * @param   path  New user data path.
 */
void sc68_set_user(sc68_t * sc68, const char * path);


SC68_API
/**
 * Empty error message stack.
 *
 * @param   sc68   sc68 instance or 0 for library messages.
 */
void sc68_error_flush(sc68_t * sc68);


SC68_API
/**
 * Pop and return last stacked error message.
 *
 * @param   sc68   sc68 instance or 0 for library messages.
 * @return  Error string.
 * @retval  0      No stacked error message.
 */
const char * sc68_error_get(sc68_t * sc68);


SC68_API
/**
 * Stack an error Add Pop and return last stacked error message.
 *
 * @param   sc68   sc68 instance or 0 for library messages.
 * @return  Error string.
 * @retval  0      No stacked error message.
 */
int sc68_error_add(sc68_t * sc68, const char * fmt, ...);


SC68_API
/**
 * Display debug message.
 *
 *  @param  sc68  sc68 instance.
 *  @param  fmt   printf() like format string.
 *
 * @see debugmsg68()
 * @see sc68_t::debug_bit
 */
void sc68_debug(sc68_t * sc68, const char * fmt, ...);

/**
 * @}
 */


/**
 * @name Music control functions.
 *  @{
 */

SC68_API
/**
 * Fill PCM buffer.
 *
 *    The sc68_process() function fills the PCM buffer with the current
 *    music data. If the current track is finished and it is not the last
 *    the next one is automatically loaded. The function returns status
 *    value that report events that have occured during this pass.
 *
 * @param  sc68  sc68 instance.
 * @param  buf   PCM buffer (must be at least 4*n bytes).
 * @param  n     Pointer to number of PCM sample to fill.
 *
 * @return Process status
 *
 */
int sc68_process(sc68_t * sc68, void * buf, int * n);

SC68_API
/**
 * Set/Get current track.
 *
 *   The sc68_play() function get or set current track.
 *
 *   If track == -1 and loop == 0 the function returns the current
 *   track or 0 if none.
 *
 *   If track == -1 and loop != 0 the function returns the current
 *   loop counter.
 *
 *   If track >= 0 the function will test the requested track
 *   number. If it is 0, the disk default track will be use. If the
 *   track is out of range, the function fails and returns -1 else it
 *   returns 0.  To avoid multi-threading issus the track is not
 *   changed directly but a change-track event is posted. This event
 *   will be processed at the next call to the sc68_process()
 *   function. If loop is -1 the default music loop is used. If loop
 *   is 0 does an infinite loop.
 *
 * @param  sc68   sc68 instance.
 * @param  track  track number [-1:read current, 0:set disk default]
 * @param  loop   number of loop [-1:default 0:infinite]
 *
 * @return error code or track number.
 * @retval 0  Success or no current track
 * @retval >0 Current track
 * @retval -1 Failure.
 *
 */
int sc68_play(sc68_t * sc68, int track, int loop);

SC68_API
/**
 * Stop playing.
 *
 *     The sc68_stop() function stop current playing track. Like the
 *     sc68_play() function the sc68_stop() function does not really
 *     stop the music but send a stop-event that will be processed by
 *     the next call to sc68_process() function.
 *
 * @param  sc68    sc68 instance.
 * @return error code
 * @retval 0  Success
 * @retval -1 Failure
 */
int sc68_stop(sc68_t * sc68);

SC68_API
/**
 * Set/Get current play position.
 *
 *    The sc68_seek() functions get or set the current play position.
 *
 *    If time_ms == -1 the function returns the current play position
 *    or -1 if not currently playing.
 *
 *    If time_ms >= 0 the function tries to seek to the given position.
 *    If time_ms is out of range the function returns -1.
 *    If time_ms is inside the current playing track the function does not
 *    seek backward.
 *    Else the function changes to the track which time_ms belong to and
 *    returns the time position at the beginning of this track.
 *
 *    The returned time is always the current position in millisecond
 *    (not the goal position).
 *
 * @param  sc68        sc68 instance.
 * @param  time_ms     new time position in ms (-1:read current time).
 * @param  is_seeking  Fill with current seek status (0:not seeking 1:seeking)
 *
 * @return  Current play position (in ms)
 * @retval  >0  Success
 * @retval  -1  Failure
 *
 * @bug  Time position calculation may be broken if tracks have different
 *       loop values. But it should not happen very often.
 */
int sc68_seek(sc68_t * sc68, int time_ms, int * is_seeking);

SC68_API
/**
 * Get disk/track information.
 *
 * @param  info  pointer to a sc68_music_info_t struct to be fill.
 * @param  sc68  sc68 instance
 * @param  track track number (-1 or 0:current/default).
 * @param  disk  disk to get information from (0 for current disk).
 *
 * @return error code
 * @retval  0  Success.
 * @retval -1  Failure.
 *
 */
int sc68_music_info(sc68_t * sc68, sc68_music_info_t * info,
                    int track, sc68_disk_t disk);

/**
 * @}
 */


/**
 * @name File functions.
 *  @{
 */

/**
 * Get official sc68 mime-type.
 * @retval  "audio/x-sc68"
 */
const char * sc68_mimetype(void);

#ifdef _FILE68_ISTREAM68_H_
SC68_API
/**
 * Create a stream from url. */
istream68_t *  sc68_stream_create(const char * url, int mode);
#endif

/**
 * Verify an sc68 disk. */
#ifdef _FILE68_ISTREAM68_H_
SC68_API
int sc68_verify(istream68_t * is);
#endif
SC68_API
int sc68_verify_url(const char * url);
SC68_API
int sc68_verify_mem(const void * buffer, int len);
SC68_API
int sc68_is_our_url(const char * url,
                    const char *exts, int * is_remote);

/**
 * Load an sc68 disk for playing. */
#ifdef _FILE68_ISTREAM68_H_
SC68_API
int sc68_load(sc68_t * sc68, istream68_t * is);
#endif
SC68_API
int sc68_load_url(sc68_t * sc68, const char * url);
SC68_API
int sc68_load_mem(sc68_t * sc68, const void * buffer, int len);

/**
 * Load an sc68 disk outside the API.
 *
 *  @notice Free it with sc68_disk_free() function.
 */
#ifdef _FILE68_ISTREAM68_H_
SC68_API
sc68_disk_t sc68_load_disk(istream68_t * is);
#endif
SC68_API
sc68_disk_t sc68_load_disk_url(const char * url);
SC68_API
sc68_disk_t sc68_disk_load_mem(const void * buffer, int len);
SC68_API
void sc68_disk_free(sc68_disk_t disk);

SC68_API
/**
 * Change current disk.
 *
 * @param  sc68   sc68 instance
 * @param  disk  New disk (0 does a sc68_close())
 *
 * @return error code
 * @retval 0  Success, disk has been loaded.
 * @retval -1 Failure, no disk has been loaded (occurs if disk was 0).
 *
 * @note    Can be safely call with null sc68.
 * @warning After sc68_open() failure, the disk has been freed.
 * @warning Beware not to use disk information after sc68_close() call
 *          because the disk should have been destroyed.
 */
int sc68_open(sc68_t * sc68, sc68_disk_t disk);

SC68_API
/**
 * Close current disk.
 *
 * @param  sc68  sc68 instance
 *
 * @note   Can be safely call with null sc68 or if no disk has been loaded.
 */
void sc68_close(sc68_t * sc68);

SC68_API
/**
 * Get number of tracks.
 *
 * @param  sc68  sc68 instance
 *
 * @return Number of track
 * @retval -1 error
 *
 * @note Could be use to check if a disk is loaded.
 */
int sc68_tracks(sc68_t * sc68);

/**
 * @}
 */


/**
 * @name Configuration functions
 *  @{
 */

SC68_API
/**
 * Load config.
 *
 * @param  sc68  sc68 instance
 */
int sc68_config_load(sc68_t * sc68);

SC68_API
/**
 * Save config.
 *
 * @param  sc68  sc68 instance
 */
int sc68_config_save(sc68_t * sc68);

SC68_API
/**
 * Apply current configuration to sc68.
 *
 * @param  sc68  sc68 instance
 */
void sc68_config_apply(sc68_t * sc68);

/**
 * @}
 */


/**
 * @name Dynamic memory access.
 *  @{
 */

SC68_API
/**
 * Allocate dynamic memory.
 *
 *   The sc68_alloc() function calls the alloc68() function.
 *
 * @param  n  Size of buffer to allocate.
 *
 * @return pointer to allocated memory buffer.
 * @retval 0 error
 *
 * @see sc68_calloc()
 * @see sc68_free()
 */
void * sc68_alloc(unsigned int n);

SC68_API
/**
 * Allocate zeroed dynamic memory.
 *
 *   The sc68_calloc() function calls the calloc68() function.
 *
 * @param  n  Size of buffer to allocate.
 *
 * @return pointer to allocated memory buffer.
 * @retval 0 error
 *
 * @see sc68_alloc()
 * @see sc68_free()
 */
void * sc68_calloc(unsigned int n);


SC68_API
/**
 * Free dynamic memory.
 *
 *   The sc68_free() function calls the free68() function.
 *
 * @param  data  Previously allocated memory buffer.
 *
 * @see sc68_alloc()
 * @see sc68_calloc()
 */
void sc68_free(void * data);

/**
 * @}
 */

/**
 * @}
 */

#endif /* #ifndef _SC68_SC68_H_ */