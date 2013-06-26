/**
 * @ingroup  file68_lib
 * @file     sc68/file68.h
 * @author   Benjamin Gerard
 * @date     1998-09-03
 * @brief    Music file header.
 *
 */

/* Copyright (C) 1998-2011 Benjamin Gerard */

#ifndef _FILE68_FILE68_H_
#define _FILE68_FILE68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif
#include "istream68.h"
#include "tag68.h"

/**
 * @defgroup  file68_file  Music file manipulation
 * @ingroup   file68_lib
 *
 *  Provides various functions for sc68 file manipulation.
 *
 * @{
 */

/**
 * SC68 (not IANA) official mime-type.
 * @see file68_idstr
 */
#define SC68_MIMETYPE "audio/x-sc68"

/**
 * SC68 file identification string definition.
 * @see file68_identifier()
 */
#define SC68_IDSTR "SC68 Music-file / (c) (BeN)jamin Gerard / SasHipA-Dev  "

/**
 * SC68 file identification string definition V2.
 * @see file68_identifier()
 */
#define SC68_IDSTR_V2 "SC68 M2"

#define SC68_NOFILENAME "n/a"   /**< SC68 unknown filename or author.      */
#define SC68_LOADADDR   0x10000 /**< Default load address in 68K memory.    */
#define SC68_MAX_TRACK  99      /**< Maximum track per disk (2 digits max). */


/**
 * @name  Features flag definitions for music68_t.
 * @{
 */
enum  {
  SC68_YM        = 1 << 0,   /**< YM-2149 actif.                  */
  SC68_STE       = 1 << 1,   /**< STE sound actif.                */
  SC68_AMIGA     = 1 << 2,   /**< AMIGA sound actif.              */
  SC68_STECHOICE = 1 << 3,   /**< Optionnal STF/STE (not tested). */
  SC68_TIMERS    = 1 << 4,   /**< Has timer info.                 */
  SC68_TIMERA    = 1 << 5,   /**< Timer-A used.                   */
  SC68_TIMERB    = 1 << 6,   /**< Timer-B used.                   */
  SC68_TIMERC    = 1 << 7,   /**< Timer-C used.                   */
  SC68_TIMERD    = 1 << 8    /**< Timer-D used.                   */
};
/**
 * @}
 */


/**
 * @}
 */

/**
 * Hardware and features flags.
 */
typedef union {

  struct {
    unsigned ym:1;        /**< Music uses YM-2149 (ST).          */
    unsigned ste:1;       /**< Music uses STE specific hardware. */
    unsigned amiga:1;     /**< Music uses Paula Amiga hardware.  */
    unsigned stechoice:1; /**< Music allow STF/STE choices.      */

    unsigned timers:1;    /**< Set if the timer status is known. */
    unsigned timera:1;    /**< Music uses timer A                */
    unsigned timerb:1;    /**< Music uses timer B                */
    unsigned timerc:1;    /**< Music uses timer C                */
    unsigned timerd:1;    /**< Music uses timer D                */

  } bit;                  /**< Flags bit field.                  */
  unsigned all;           /**< All flags in one.                 */
} hwflags68_t;


/**
 * SC68 music (track) structure.
 */
typedef struct {
  unsigned int d0;       /**< D0 value to init this music.            */
  unsigned int a0;       /**< A0 Loading address. @see SC68_LOADADDR. */
  unsigned int frq;      /**< Frequency in Hz (default:50).           */
  unsigned int start_ms; /**< Start time in ms from disk 1st track.   */
  unsigned int total_ms; /**< Total time in ms (first+loops).         */
  unsigned int total_fr; /**< Total time in frames.                   */
  unsigned int first_ms; /**< First loop duration in ms.              */
  unsigned int first_fr; /**< First loop duration in frames.          */
  unsigned int loops_ms; /**< Loop duration in ms (0:no loop).        */
  unsigned int loops_fr; /**< Loop duration in frames (0:no loop).    */
  int          loops;    /**< Default number of loop (0:infinite).    */
  int          track;    /**< Track remapping number (0:default).     */

  struct {
    unsigned   sfx  : 1; /**< Track is a sound-fx not a music.        */
    unsigned   pic  : 1; /**< Track is position independant code.     */
    unsigned   time : 1; /**< Track has time info.                    */
    unsigned   loop : 1; /**< Track has loop info.                    */

    unsigned asid_trk : 8; /**< 0:not asid, >0: original track.       */
    unsigned asid_tA  : 2; /**< timer used for YM channel-A.          */
    unsigned asid_tB  : 2; /**< timer used for YM channel-B.          */
    unsigned asid_tC  : 2; /**< timer used for YM channel-C.          */
    unsigned asid_tX  : 2; /**< timer not used by aSID.               */

  } has;                 /**< Track flags.                            */

  char        *replay;   /**< External replay name.                   */
  hwflags68_t  hwflags;  /**< Hardware and features.                  */
  tagset68_t   tags;     /**< Meta data.                              */
  unsigned int datasz;   /**< data size in bytes. */
  char        *data;     /**< Music data.         */
} music68_t;


/**
 * SC68 music disk structure.
 *
 *  The disk68_t structure is the memory representation for an SC68
 *  disk.  Each SC68 file could several music or tracks, in the limit
 *  of a maximum of 99 tracks per file. Each music is independant, but
 *  some information, including music data, could be inherit from
 *  previous track. In a general case, tracks are grouped by theme,
 *  that could be a demo or a game.
 *
 */
typedef struct {
  int          def_mus;     /**< Preferred default music (default is 0). */
  int          nb_mus;      /**< Number of music track in file.          */
  int          nb_asid;     /**< Number of aSID track append.            */
  unsigned int time_ms;     /**< Total time for all tracks in ms.        */
  hwflags68_t  hwflags;     /**< All tracks flags ORed.                  */
  tagset68_t   tags;        /**< Meta tags for the disk (album)          */
  music68_t    mus[SC68_MAX_TRACK]; /**< Information for each music.     */
  unsigned int datasz;      /**< data size in byte.                      */
  char        *data;        /**< points to data.                         */
  char         buffer[4];   /**< raw data. MUST be last member.          */
} disk68_t;

/**
 * @name  Meta tag functions.
 * @{
 */

FILE68_API
/**
 * Count and re-order metatags.
 *
 *  The file68_tag_count() function counts metatags and ensures no
 *  empty tag within that count.
 *
 * @param  mb     pointer to SC68 disk
 * @param  track  track number (0:disk)
 *
 * @retval -1     on error
 * @retval >=0    number of tag (in fact it should always be >= 3)
 */
int file68_tag_count(disk68_t * mb, int track);

FILE68_API
/**
 * Enumerate metatags.
 *
 *  The file68_tag_enum() function enumerates defined metatags.
 *
 * @param  mb     pointer to SC68 disk
 * @param  track  track number (0:disk)
 * @param  idx    index of the metatag
 * @param  key    pointer to tag name
 * @param  val    pointer to tag value
 *
 * @retval -1     on error
 * @retval  0     on success
 */
int file68_tag_enum(const disk68_t * mb, int track, int idx,
                    const char ** key, const char ** val);

FILE68_API
/**
 * Get metatag from disk or track.
 *
 *  The file68_tag_get() function gets the value of a metatag.
 *
 * @param  mb     pointer to SC68 disk
 * @param  track  track number (0:disk)
 * @param  key    tag name
 *
 * @return tag value
 * @retval  0  tag is not set
 *
 * @see file68_tag_set()
 */
const char * file68_tag_get(const disk68_t * mb, int track,
                            const char * key);

/**
 * @}
 */


FILE68_API
/**
 * Get metatag from disk or track.
 *
 *  The file68_tag_set() function sets the value of a metatag.
 *
 * @param  mb     pointer to SC68 disk
 * @param  track  track number (0:disk)
 * @param  key    tag name
 * @param  val    tag value to set (0 to unset)
 *
 * @return tag value
 * @retval  0  tag is not set
 *
 * @see file68_tag_set()
 */
const char * file68_tag_set(disk68_t * mb, int track,
                            const char * key, const char * val);

/**
 * @name  File verify functions.
 * @{
 */

FILE68_API
/**
 * Verify SC68 file from stream.
 *
 *  The file68_verify() function opens, reads and closes given file to
 *  determine if it is a valid SC68 file. This function only checks
 *  for a valid file header, and does not perform any consistent error
 *  checking.
 *
 * @param  is       input stream to verify
 *
 * @return error-code
 * @retval  0  success, seems to be a valid SC68 file
 * @retval <0  failure, file error or invalid SC68 file
 *
 * @see file68_load()
 * @see file68_save()
 * @see file68_diskname()
 */
int file68_verify(istream68_t * is);

FILE68_API
/**
 * Verify SC68 file.
 *
 * @param  url      URL to verify.
 */
int file68_verify_url(const char * url);

FILE68_API
/**
 * Verify SC68 file mapped into memory buffer.
 *
 * @param  buffer   buffer address
 * @param  len      buffer length
 */
int file68_verify_mem(const void * buffer, int len);

FILE68_API
/**
 * Get SC68 disk name.
 *
 *  The file68_diskname() function opens, reads and closes given file
 *  to determine if it is a valid SC68 file. In the same time it tries
 *  to retrieve the stored disk name into the dest buffer with a
 *  maximum length of max bytes.  If the name overflows, the last byte
 *  of the dest buffer will be non zero.
 *
 * @param  is       input stream
 * @param  dest     disk name destination buffer
 * @param  max      number of bytes of dest buffer
 *
 * @return error-code
 * @retval  0  success, found a disk-name
 * @retval <0  failure, file error, invalid SC68 file or disk-name not found
 *
 * @see file68_load()
 * @see file68_save()
 * @see file68_diskname()
 *
 * @deprecated This function needs to be rewritten.
 */
int file68_diskname(istream68_t * is, char * dest, int max);



FILE68_API
/**
 * Check if an URL is a standard sc68 one.
 *
 * @param  url        URL to check.
 * @param  exts       extension list. (0 is default: ".sc68\0.sndh\0.snd\0").
 * @param  is_remote  fill with 0/1 if respectevely URL is a local/remote
 *                     file. May be 0.
 * @return  bool
 * @retval  0  not compatible sc68 file
 * @retval  1  sc68 compatible file
 */
int file68_is_our_url(const char * url, const char * exts, int * is_remote);

/**
 * @}
 */


/**
 * @name  File load functions.
 * @{
 */

FILE68_API
/**
 * Load SC68 file from stream.
 *
 *  The file68_load() function allocates memory and loads an SC68
 *  file.  The function performs all necessary initializations in the
 *  returned disk68_t structure. A single buffer has been allocated
 *  including disk68_t structure followed by music data. It is user
 *  charge to free memory by calling file68_free() function.
 *
 * @param   is   input stream
 *
 * @return  pointer to allocated disk68_t disk structure
 * @retval  0  failure
 *
 * @see file68_verify()
 * @see file68_save()
 */
disk68_t * file68_load(istream68_t * is);

FILE68_API
/**
 * Load SC68 file.
 *
 * @param  url      URL to load.
 *
 * @return  pointer to allocated disk68_t disk structure
 * @retval  0  failure
 */
disk68_t * file68_load_url(const char * url);

FILE68_API
/**
 * Load SC68 file mapped into memory buffer.
 *
 * @param  buffer   buffer address
 * @param  len      buffer length
 *
 * @return  pointer to allocated disk68_t disk structure
 * @retval  0  failure
 */
disk68_t * file68_load_mem(const void * buffer, int len);

/**
 * @}
 */


/**
 * @name  File save functions.
 * @{
 */

FILE68_API
/**
 * Save SC68 disk into stream.
 *
 * @param  os       output stream (must be seekable)
 * @param  mb       pointer to SC68 disk to save
 * @param  version  file version [0:default]
 * @param  gzip     gzip compression level [0:no-gzip, 1..9 or -1]
 *
 * @return error-code
 * @retval  0  success
 * @retval <0  failure
 *
 * @see file68_load()
 * @see file68_verify()
 * @see file68_diskname()
 */
int file68_save(istream68_t * os, const disk68_t * mb,
                int version, int gzip);

FILE68_API
/**
 * Save SC68 disk into file.
 *
 * @param  url      URL to save.
 * @param  mb       pointer to SC68 disk to save
 * @param  version  file version [0:default]
 * @param  gzip     gzip compression level [0:no-gzip, 1..9 or -1]
 */
int file68_save_url(const char * url, const disk68_t * mb,
                    int version, int gzip);

FILE68_API
/**
 * Save SC68 disk into memory buffer.
 *
 * @param  buffer   destination buffer.
 * @param  len      size of destination buffer.
 * @param  mb       pointer to SC68 disk to save
 * @param  version  file version [0:default]
 * @param  gzip     gzip compression level [0:no-gzip, 1..9 or -1]
 */
int file68_save_mem(const char * buffer, int len, const disk68_t * mb,
                    int version, int gzip);

/**
 * @}
 */


/**
 * @name  Memory allocation functions.
 * @{
 */

FILE68_API
/**
 * Create fresh new disk.
 *
 * @param  extra  extra bytes to allocate
 *
 * @return  a pointer to allocated disk
 * @retval  0  failure
 *
 */
disk68_t * file68_new(int extra);

FILE68_API
/**
 * Destroy loaded or allocated disk.
 *
 * @param  extra  extra bytes to allocate
 *
 * @return  a pointer to allocated disk
 * @retval  0  failure
 *
 *
 * @see file68_new()
 * @see file68_load()
 */
void file68_free(disk68_t * disk);

/**
 * @}
 */


/**
 * @name  Library functions.
 * @{
 */

FILE68_API
/**
 * Get package version string.
 *
 * @return "file68 X.Y.Z"
 */
const char * file68_versionstr(void);

FILE68_API
/**
 * Get package version number.
 *
 * @return X*100+Y*10+Z
 */
int file68_version(void);

FILE68_API
/**
 * Get identifier string.
 * @param  version  0:default 1:v1 2:v2
 * @return a static string of the identifier
 * @see SC68_IDSTR
 * @see SC68_IDSTR_V2
 */
const char * file68_identifier(int version);

FILE68_API
/**
 * Get sc68 official (no IANA) mime type.
 * @return a static string of the mimetype (should be audio/x-sc68).
 */
const char * file68_mimetype(void);

FILE68_API
/**
 * Initialize file68 library.
 *
 *  @param  argc  argument count
 *  @param  argv  argument array (as for main())
 *  @return new argument count
 */
int file68_init(int argc, char **argv);

FILE68_API
/**
 * Shutdown file68 library.
 */
void file68_shutdown(void);

/**
 * @}
 */


/**
 * @}
 */

#endif /* #ifndef _FILE68_FILE68_H_ */