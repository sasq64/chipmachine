/*
 * @file    file68.c
 * @brief   load and save sc68 stream
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-11-21 19:59:29 ben>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "file68_api.h"
#include "file68.h"
#include "chunk68.h"
#include "error68.h"
#include "alloc68.h"
#include "msg68.h"
#include "string68.h"
#include "rsc68.h"
#include "option68.h"

#include "istream68_def.h"
#include "istream68_file.h"
#include "istream68_fd.h"
#include "istream68_curl.h"
#include "istream68_mem.h"
#include "istream68_z.h"
#include "istream68_null.h"
#include "gzip68.h"
#include "ice68.h"
#include "url68.h"

#ifndef u64
# ifdef HAVE_STDINT_H
#  include <stdint.h>
#  define u64 uint_least64_t
# elif defined(_MSC_VER)
#  define u64 unsigned __int64
# elif defined(__GNUC__)
#  define u64 unsigned long long
# endif
#endif

#ifndef u64
# error "u64 must be defined as an integer of at least 64 bit"
#endif

#include <string.h>
#include <ctype.h>
#include <stdio.h>

int aSIDify = 0;                        /* 0:off, 1:safe, 2:force */

#define FOURCC(A,B,C,D) ((int)( ((A)<<24) | ((B)<<16) | ((C)<<8) | (D) ))
#define gzip_cc FOURCC('g','z','i','p')
#define ice_cc  FOURCC('i','c','e','!')
#define sndh_cc FOURCC('S','N','D','H')
#define sc68_cc FOURCC('S','C','6','8')

/* SC68 file identifier strings */
const char file68_idstr_v1[56] = SC68_IDSTR;
const char file68_idstr_v2[8]  = SC68_IDSTR_V2;
const char file68_mimestr[]    = SC68_MIMETYPE;
#define MAX_TRACK_LP 4          /* maximum number of guessed loop allowed                */
#define MIN_TRACK_MS (45*1000u) /* below this value, force a few loops                   */
#define DEF_TRACK_MS (90*1000u) /* time we use as default if track contains no such info */

#ifndef DEBUG_FILE68_O
# define DEBUG_FILE68_O 0
#endif
int file68_cat = msg68_DEFAULT;

#define ISCHK(A,B) ( (A)[0] == (B)[0] && (A)[1] == (B)[1] )

/* Array of static strings, mainly used by metatags. */
static /* const */ struct strings_table {
  char n_a[4];
  char aka[4];
  char sc68[5];
  char sndh[5];
  char rate[5];
  char year[5];
  char album[6];
  char genre[6];
  char image[6];
  char title[6];
  char replay[7];
  char artist[7];
  char author[7];
  char ripper[7];
  char format[7];
  char comment[8];
  char composer[9];
  char converter[10];
  char copyright[10];
  char amiga_chiptune[15];
  char atari_st_chiptune[18];

  char _reserved;
} tagstr = {
  SC68_NOFILENAME,
  TAG68_AKA,
  "sc68",
  "sndh",
  TAG68_RATE,
  TAG68_YEAR,
  TAG68_ALBUM,
  TAG68_GENRE,
  TAG68_IMAGE,
  TAG68_TITLE,
  TAG68_REPLAY,
  TAG68_ARTIST,
  TAG68_AUTHOR,
  TAG68_RIPPER,
  TAG68_FORMAT,
  TAG68_COMMENT,
  TAG68_COMPOSER,
  TAG68_CONVERTER,
  TAG68_COPYRIGHT,
  "Amiga chiptune",
  "Atari-ST chiptune",
  -1,
};

/* Does this memory belongs to the disk data ? */
static inline int is_disk_data(const disk68_t * const mb, const void * const _s)
{
  const char * const s = (const char *) _s;
  return (mb && s >= mb->data && s < mb->data+mb->datasz);
}

/* Does this memory belongs to the static string array ? */
static inline int is_static_str(const char * const s)
{
  const char * const tagstr0 = (const char *) &tagstr;
  const char * const tagstr1 = tagstr0+sizeof(tagstr);
  return (s >= tagstr0 && s < tagstr1);
}

/* Free buffer unless it is allocated inside disk data or inside
 * static tag string array */
static void free_string(const disk68_t * const mb, void * const s)
{
  if (s && !is_static_str(s) && !is_disk_data(mb,s))
    free68(s);
}

static inline void safe_free_string(const disk68_t * const mb, void * const _s)
{
  void ** s = _s;
  free_string(mb, *s); *s = 0;
}

static char *strdup_not_static(const disk68_t * const mb, const char * s)
{
  return (char *) ( (s && !is_static_str(s) && !is_disk_data(mb,s))
                    ? strdup68(s)
                    : s );
}

/* Peek Little Endian Unaligned 32 bit value */
static inline int LPeek(const void * const a)
{
  int r;
  const unsigned char * const c = (const unsigned char *) a;
  r = c[0] + (c[1] << 8) + (c[2] << 16) + ((int)(signed char)c[3] << 24);
  return r;
}

static inline int LPeekBE(const void * const a)
{
  int r;
  const unsigned char * const c = (const unsigned char *) a;
  r = ((int)(signed char)c[0] << 24) + (c[1] << 16) + (c[2] << 8) + c[3];
  return r;
}

/* Poke Little Endian Unaligned 32 bit value */
static inline void LPoke(void * const a, int r)
{
  unsigned char * const c = (unsigned char *) a;
  c[0] = r;
  c[1] = r >> 8;
  c[2] = r >> 16;
  c[3] = r >> 24;
}

static int myatoi(const char *s, int i, int max, int * pv)
{
  int v = 0;
  for (; i<max; ++i) {
    int c = s[i] & 255;
    if (c>='0' && c<='9') {
      v = v * 10 + c - '0';
    } else {
      break;
    }
  }
  if (pv) *pv = v;
  return i;
}

/* Match regexpr : [[:alpha:]][-_[:alnum:]]* */
static int is_valid_key(const char * key)
{
  int c;
  if (!key || (c = *key++, !isalpha(c)))
    return 0;
  do {
    if (!isalnum(c) || c == '-' || c == '_')
      return 0;
  } while (c = *key++, c);
  return 1;
}

/* Searching a key (key can be 0 to search an empty slot) */
static int get_customtag(const tagset68_t * tags, const char * key)
{
  int i;
  if (!strcmp68(key, tagstr.title))
    return TAG68_ID_TITLE;
  if (!strcmp68(key, tagstr.album))
    return TAG68_ID_ALBUM;
  if (!strcmp68(key, tagstr.artist))
    return TAG68_ID_ARTIST;
  if (!strcmp68(key, tagstr.author))
    return TAG68_ID_AUTHOR;
  if (!strcmp68(key, tagstr.genre))
    return TAG68_ID_GENRE;
  if (!strcmp68(key, tagstr.format))
    return TAG68_ID_FORMAT;
  for (i=TAG68_ID_CUSTOM; i<TAG68_ID_MAX; ++i) {
    if (!strcmp68(key, tags->array[i].key))
      return i;
  }
  return -1;
}

/* Set/Replace a custom key (val can be 0 to remove the tag). */
static int set_customtag(disk68_t * mb, tagset68_t * tags,
                         const char * key, const char * val)
{
  int i
    = get_customtag(tags, key);
  if (!val) {
    if (i >= 0) {
      safe_free_string(mb,&tags->array[i].val);
      if (i >= TAG68_ID_CUSTOM)
        safe_free_string(mb,&tags->array[i].key);
    }
  } else {
    if (i < 0)
      i = get_customtag(tags, 0);
    if (i >= 0) {
      safe_free_string(mb,&tags->array[i].val);
      if (!tags->array[i].key && !(tags->array[i].key = strdup_not_static(mb, key)))
        i = -1;
      else if (!(tags->array[i].val = strdup_not_static(mb, val)))
        i = -1;
    }
  }
  return i;
}

#define ok_int(V)  strok68(V)
#define strnull(S) strnevernull68(S)

static int sndh_is_magic(const char *buffer, int max)
{
  const int start = 6;
  int i=0, v = 0;
  if (max >= start) {
    for (i=start, v = LPeekBE(buffer); i < max && v != sndh_cc;
         v = ((v<<8)| (buffer[i++]&255)) & 0xFFFFFFFF)
      ;
  }
  i = (v == sndh_cc) ? i-4: 0;
  return i;
}

/* Ensure enough data in id buffer;
 *  retval  0      on error
 *  retval  count  on success
 */
static inline
int ensure_header(istream68_t * const is, char *id, int have, int need)
{
  if (have < need) {
    int miss = need - have;
    int read = istream68_read(is, id+have, miss);
    if (read != miss) {
      /* msg68_error("not a sc68 file (%s)\n", read==-1?"read error":"eof"); */
      have = 0;
    } else {
      have = need;
    }
  }
  return have;
}

/* Verify header; returns # bytes to alloc & read
 * or -1 if error
 * or -gzip_cc if may be gzipped
 * or -ice_cc if may be iced
 * or -sndh_cc if may be sndh
 */
static int read_header(istream68_t * const is)
{
  char id[256];
  const int idv1_req = sizeof(file68_idstr_v1);
  const int idv2_req = sizeof(file68_idstr_v2);
  const int sndh_req = 32;
  int have = 0;
  const char * missing_id = "not a sc68 file (no magic)";

  /* Read ID v2 string */
  if(have = ensure_header(is, id, have, idv2_req), !have) {
    return -1;
  }

  if (!memcmp(id, file68_idstr_v1, idv2_req)) {
    /* looks like idv1; need more bytes to confirm */
    if (have = ensure_header(is, id, have, idv1_req), !have) {
      return -1;
    }
    if (memcmp(id, file68_idstr_v1, idv1_req)) {
      return error68(missing_id);
    }
    TRACE68(file68_cat,"file68: found %s signature\n", "SC68_v1");
  } else if (!memcmp(id, file68_idstr_v2, idv2_req)) {
    TRACE68(file68_cat,"file68: found %s signature\n","SC68_v2");
  } else {
    if (have = ensure_header(is, id, have, sndh_req), !have) {
      return -1;
    }
    if (gzip68_is_magic(id)) {
      TRACE68(file68_cat,"file68: found %s signature\n","GZIP");
      return -gzip_cc;
    } else if (ice68_is_magic(id)) {
      TRACE68(file68_cat,"file68: found %s signature\n","ICE!");
      return -ice_cc;
    } else {
      /* Must be done after gzip or ice becausez id-string may appear
       * in compressed buffer too.
       */
      if (sndh_is_magic(id,sndh_req)) {
        TRACE68(file68_cat,"file68: found %s signature\n","SNDH");
        return -sndh_cc;
      }
    }
    return error68(missing_id);
  }

  /* Check 1st chunk */
  if (istream68_read(is, id, 4) != 4
      || memcmp(id, CH68_CHUNK CH68_BASE, 4)) {
    return error68("file68: not sc68 file -- missing base chunk");
  }

  /* Get base chunk : file total size */
  if (istream68_read(is, id, 4) != 4
      || (have = LPeek(id), have <= 8)) {
    return error68("file68: not sc68 file -- weird base chunk size");
  }
  /* TRACE68(file68_cat,"file68: header have %d bytes\n",have-8); */
  return have-8;
}

static const char * not_noname(const char * s)
{
  return (s && strcmp68(s,tagstr.n_a)) ? s : 0;
}

/* FR  = SEC * HZ
 * FR  = MS*HZ/1000
 *
 * SEC = FR / HZ
 * MS  = FR*1000/HZ
 */

static unsigned int fr_to_ms(unsigned int fr, unsigned int hz)
{
  u64 ms;
  ms  = fr;
  ms *= 1000u;
  ms /= hz;
  return (unsigned int) ms;
}

static unsigned int ms_to_fr(unsigned int ms, unsigned int hz)
{
  u64 fr;
  fr  = ms;
  fr *= hz;
  fr /= 1000u;
  return (unsigned int) fr;
}

/* Extract AKA from legacy sc68 artist nomenclature `ARTIST (AKA)'
 * excepted for unknown (replay) ones.
 */
static int guess_artist_alias(disk68_t * mb, tagset68_t * tags)
{
  int id;
  char * artist = tags->array[TAG68_ID_ARTIST].val;
  if ((id = get_customtag(tags, tagstr.aka))<0 && artist) {
    /* Don't have aka tag, trying to guess it from artist */
    char * e = strrchr(artist,')');
    if (e && !e[1]) {
      char * s;
      for (s = e-1; s > artist && *s != '('; --s)
        ;
      if (s > artist && s[-1] == ' ' && strncmp68(artist,"unknown",7)) {
        s[-1] = 0;
        *e = 0;
        id = set_customtag(mb, tags, tagstr.aka, s+1);
        TRACE68(file68_cat,"file68 guessed AKA '%s' from '%s' tag-id:%d\n", s+1, artist, id);
      }
    }
  }
  return id;
}

/* This function inits all pointers for this music files. It setup non
 * initialized data to defaut value. It verifies most values are in
 * good range.
 */
static int valid(disk68_t * mb)
{
  music68_t *m;
  int i, pdatasz = 0;
  void * pdata   = 0;
  char * title, * artist, * aalias;

  if (mb->nb_mus <= 0)
    return error68("file68: disk has no track");

  /* Ensure default music in valid range */
  if (mb->def_mus < 0 || mb->def_mus >= mb->nb_mus)
    mb->def_mus = 0;

  /* default album is 'noname' */
  if (!mb->tags.tag.title.val)
    mb->tags.tag.title.val = tagstr.n_a;
  /* default title is album */
  title  = mb->tags.tag.title.val;
  /* default album artist is default track artist. Postpone ... */
  /* default artist is album artist or 'n/a' */
  artist = mb->tags.tag.artist.val ? mb->tags.tag.artist.val : tagstr.n_a;
  /* default alias */
  aalias = (i = guess_artist_alias(mb, &mb->tags), i<0) ? 0 : mb->tags.array[i].val;

  /* Disk total time : 00:00 */
  mb->time_ms = 0;

  /* Clear flags */
  mb->hwflags.all = 0;

  /* Init all music in this file */
  for (m = mb->mus, i = 0; m < mb->mus + mb->nb_mus; m++, i++) {

    /* $$$ TEMP $$$ hack tao_tsd, to remove ym from hardware flags */
    if (!strcmp68(m->replay,"tao_tsd")) {
      m->hwflags.bit.ym     = 0;
      m->hwflags.bit.ste    = 1;
    }

    /* Default load address */
    if ( (m->has.pic = !m->a0) )
      m->a0 = SC68_LOADADDR;

    /* Default replay frequency is 50Hz */
    if (!m->frq)
      m->frq = 50u;

    /* Compute ms from frames prior to frames from ms. */
    if ( (m->has.time = m->first_fr > 0u) ) {
      m->first_ms = fr_to_ms(m->first_fr, m->frq);
    } else {
      if ( !(m->has.time = m->first_ms > 0) )
        m->first_ms = DEF_TRACK_MS;
      m->first_fr = ms_to_fr(m->first_ms, m->frq);
    }

    if (m->has.loop) {
      /* Compute ms from frames. */
      m->loops_ms = fr_to_ms(m->loops_fr, m->frq);
    } else {
      /* loop time not set, default to track time. */
      m->loops_fr = m->first_fr;
      m->loops_ms = m->first_ms;
    }

    if (!m->loops_fr) {
      /* Track does not loop, force number of loops to 1. */
      m->loops = 1;
    } else if (!m->loops) {
      m->loops = 1;
      if ( m->first_ms < MIN_TRACK_MS)
        m->loops += (MIN_TRACK_MS - m->first_ms + (m->loops_ms>>1)) / m->loops_ms;
      if (m->loops > MAX_TRACK_LP)
        m->loops = MAX_TRACK_LP;
    }

    m->total_fr = m->first_fr + (m->loops - 1) * m->loops_fr;
    m->total_ms = fr_to_ms(m->total_fr, m->frq);

    /* set start time in the disk. */
    m->start_ms = mb->time_ms;

    /* Advance disk total time. */
    mb->time_ms += m->total_ms;

    /* default mode is YM2149 (Atari ST) */
    if (!m->hwflags.all) {
      m->hwflags.bit.ym = 1;
    }
    mb->hwflags.all |= m->hwflags.all;

    /* default genre */
    if (!m->tags.tag.genre.val)
      m->tags.tag.genre.val = (m->hwflags.bit.amiga)
        ? tagstr.amiga_chiptune
        : tagstr.atari_st_chiptune
        ;

    /* default music name is album name */
    if (!m->tags.tag.title.val)
      m->tags.tag.title.val = title;    /* inherits title  */
    else
      title = m->tags.tag.title.val;    /* new inheririted title */

    /* default artist */
    if (!m->tags.tag.artist.val) {
      m->tags.tag.artist.val = artist;  /* inherit artist */
      if (aalias)
        set_customtag(mb, &m->tags, tagstr.aka, aalias);
    } else {
      int id;
      artist = m->tags.tag.artist.val;  /* new inherited artist */
      aalias = (id = guess_artist_alias(mb, &m->tags), id < 0) ? 0 : m->tags.array[id].val;
    }

    /* use data from previous music */
    if (!m->data) {
      m->data   = (char *) pdata;       /* inherit music data */
      m->datasz = pdatasz;
    }
    if (!m->data)
      return error68("file68: track #%d has no data", i+1);
    pdata   = m->data;                  /* new inherited music data */
    pdatasz = m->datasz;
  }

  /* album artist inherits default track artist */
  if (!mb->tags.tag.artist.val) {
    mb->tags.tag.artist.val = mb->mus[mb->def_mus].tags.tag.artist.val;
    if (i = get_customtag(&mb->mus[mb->def_mus].tags, tagstr.aka), i>=0)
      set_customtag(mb, &mb->tags, tagstr.aka, mb->mus[mb->def_mus].tags.array[i].val);
  }

  /* aSIDidy */
  if (aSIDify) {
    int j;

    /* Init all music in this file */
    for (i=j=0; i<mb->nb_mus && mb->nb_mus+j<SC68_MAX_TRACK ; i++) {
      music68_t * s = mb->mus+i;

      if (!s->hwflags.bit.ym)
        continue;                  /* not a YM track, can't aSIDify */

      switch (aSIDify) {

      case 1:
        /* "safe" mode */
        if (!s->hwflags.bit.timers   /* Don't know about timers: not safe */
            || s->hwflags.bit.timera /* One or more timers in used: not safe */
            || s->hwflags.bit.timerb
            || s->hwflags.bit.timerc
            || s->hwflags.bit.timerd)
          break;

      case 2:
        /* "force" mode */

        if (1)
          m = s;
        else {
          m  = mb->mus + mb->nb_mus + j++;
          *m = *s;
          /* $$$ WE SHOULD CHECK FOR MALLOCATED DATA IN THE TAG SET */
        }

        m->has.asid_trk = i+1;
        m->has.asid_tA = 'A'-'A';
        m->has.asid_tB = 'C'-'A';
        m->has.asid_tC = 'D'-'A';
        m->has.asid_tX = 'B'-'A';
        if (m != s) {
          m->start_ms = mb->time_ms;
          mb->time_ms += m->total_ms;
        }
        mb->nb_asid++;

      default:
        break;
      }
    }
    mb->nb_mus += j;
    TRACE68(file68_cat,"file68: aSID tracks -- %d/%d\n", mb->nb_asid,mb->nb_mus );
  }

  return 0;
}

int file68_is_our_url(const char * url, const char * exts, int * is_remote)
{
  const char * url_end, *u;
  char protocol[16], *p;
  int has_protocol, remote, is_our;

  TRACE68(file68_cat,"file68: check url --\n",url);

  is_our = remote = 0;
  if (!url || !*url) {
    goto exit;
  }

  /* Default supported extensions */
  if (!exts) {
    exts = ".sc68\0.sndh\0.snd\0";
  }

  url_end = url + strlen(url);
  has_protocol = !url68_get_protocol(protocol, sizeof(protocol), url);

  if (has_protocol) {
    is_our = !strcmp68(protocol,"SC68");
    if (!is_our && !strcmp68(protocol,"RSC68") && url+14<url_end) {
      is_our = strncmp(url+8, "music/", 6);
    }

    if (is_our)  {
      /* $$$ Not really sure; may be remote or not. The only way to
         know is to check for the corresponding local file.
      */
      remote = 0;
      goto exit;
    }
  }

  /* Check remote for other protocol */
  remote = !url68_local_protocol(protocol);

  /* Check extension ... */
  p = protocol+sizeof(protocol);
  *--p = 0;
  for (u=url_end; u > url && p > protocol; ) {
    int c = *--u & 255;
    if (c == '/') {
      break;
    }
    *--p = c;
    if (c == '.') {
      if (!strcmp68(p,".GZ")) {
        p = protocol+sizeof(protocol)-1;
      } else {
        break;
      }
    }
  }

  while (*exts) {
    is_our = !strcmp68(p,exts);
    if (is_our) {
      break;
    }
    exts += strlen(exts)+1;
  }

exit:
  if (is_remote) *is_remote = remote;
  TRACE68(file68_cat, "file68: check url -- [%s]\n", ok_int(!is_our));
  return is_our;
}

/* Check if file is probable SC68 file
 * return 0:SC68-file
 */
int file68_verify(istream68_t * is)
{
  int res;
/*   const char * fname = strnull(istream68_filename(is)); */

  if (!is) {
    res = error68("file68_verify(): null pointer");
    goto error;
  }

  res = read_header(is);

  /* Verify tells it is a gzip file, so we may give it a try. */
  if (res < 0) {
    void * buffer = 0;
    int len;

    switch (res) {
    case -gzip_cc:
      if (istream68_seek_to(is,0) == 0) {
        istream68_t * zis;
        zis = istream68_z_create(is,ISTREAM68_OPEN_READ,
                                 istream68_z_default_option);
        res = -1;
        if (!istream68_open(zis)) {
          res = file68_verify(zis);
        }
        istream68_destroy(zis);
      }
      break;
    case -ice_cc:
      if (istream68_seek_to(is,0) == 0) {
        buffer = ice68_load(is, &len);
      }
      break;
    case -sndh_cc:
      res = 0;
      break;
    }
    if (buffer) {
      res = -res;
      res = file68_verify_mem(buffer, len);
      free68(buffer);
    }
  }

error:
  return -(res < 0);
}

static istream68_t * url_or_file_create(const char * url, int mode,
                                        rsc68_info_t * info)
{
  char protocol[16], tmp[512];
  const int max = sizeof(tmp)-1;
  istream68_t *isf = 0;
  int has_protocol;
  char * newname = 0;

  TRACE68(file68_cat,"file68: create -- %s -- mode:%d -- with%s info\n",
          strnull(url),mode,info?"":"out");

  if (info) {
    info->type = rsc68_last;
  }

  has_protocol = !url68_get_protocol(protocol, sizeof(protocol), url);

  if (has_protocol) {

    /* convert sc68:// -> rsc68:// */
    if (!strcmp68(protocol, "SC68")) {
      url += 4+3;
      strcpy(tmp, "rsc68://music/");
      strncpy(tmp+14, url, max-14);
      tmp[max] = 0;
      url = tmp;
      strcpy(protocol,"rsc68");
      TRACE68(file68_cat,"file68: transform url into -- %s\n",url);
    }

    if (!strcmp68(protocol, "RSC68")) {
      return rsc68_open_url(url, mode, info);
    }
  }

  isf = url68_stream_create(url, mode);

  if (istream68_open(isf)) {
    istream68_destroy(isf);
    isf = 0;
  }

  free68(newname);
  TRACE68(file68_cat,"file68: create -- [%s,%s]\n",
          ok_int(!isf),
          strnull(istream68_filename(isf)));
  return isf;
}

int file68_verify_url(const char * url)
{
  int res;
  istream68_t * is;

/*   CONTEXT68_CHECK(context); */

  is = url_or_file_create(url,1,0);
  res = file68_verify(is);
  istream68_destroy(is);

  return -(res < 0);
}

int file68_verify_mem(const void * buffer, int len)
{
  int res;
  istream68_t * is;

  is = istream68_mem_create((void *)buffer,len,1);
  res = istream68_open(is) ? -1 : file68_verify(is);
  istream68_destroy(is);

  return res;
}

/* Check if file is probable SC68 file
 * return 0:SC68-file
 */
int file68_diskname(istream68_t * is, char *dest, int max)
{
  msg68_error(0,"file68: file68_diskname function is deprecated\n");
  return -1;
}

disk68_t * file68_load_url(const char * fname)
{
  disk68_t    * d;
  istream68_t * is;
  rsc68_info_t  info;

  TRACE68(file68_cat,"file68: load -- %s\n", strnull(fname));

  is = url_or_file_create(fname, 1, &info);
  d = file68_load(is);
  istream68_destroy(is);

  if (d && info.type == rsc68_music) {
    int i;

    TRACE68(file68_cat,
            "file68: load -- on the fly patch -- #%d/%d/%d\n",
            info.data.music.track,
            info.data.music.loop,
            info.data.music.time);

    if (info.data.music.track > 0 && info.data.music.track <= d->nb_mus) {
      d->mus[0] = d->mus[info.data.music.track-1];
      d->mus[0].start_ms = 0;
      d->mus[0].track = info.data.music.track;
      d->def_mus = 0;
      d->nb_mus  = 1;
      d->time_ms = d->mus[0].total_ms;
      d->hwflags.all = d->mus[0].hwflags.all;
    }
    if (info.data.music.loop != -1) {
      for (i=0; i<d->nb_mus; ++i) {
        music68_t * m = d->mus + i;
        m->loops    = info.data.music.loop;
        m->total_fr = m->first_fr + ( m->loops - 1 ) * m->loops_fr;
        m->total_ms = fr_to_ms(m->total_fr, m->frq);
      }
    }
    if (info.data.music.time != -1) {
      unsigned int ms = info.data.music.time * 1000u;
      for (i=0; i<d->nb_mus; ++i) {
        music68_t * m = d->mus + i;
        m->total_fr = ms_to_fr(ms, m->frq);
        m->total_ms = ms;
      }
    }
    d->time_ms = 0;
    for (i=0; i<d->nb_mus; ++i) {
      music68_t * m = d->mus + i;
      m->start_ms = d->time_ms;
      d->time_ms += m->total_ms;
    }
  }

  TRACE68(file68_cat,"file68: load -- [%s]\n", ok_int(!d));
  return d;
}

disk68_t * file68_load_mem(const void * buffer, int len)
{
  disk68_t * d;
  istream68_t * is;

  is = istream68_mem_create((void *)buffer,len,1);
  d = istream68_open(is) ? 0 : file68_load(is);
  istream68_destroy(is);

  return d;
}

static int st_isgraph( int c )
{
  return c >= 32 && c < 128;
}

static int st_isdigit( int c )
{
  return c >= '0' && c <= '9';
}

#define SNDH_RIPP_ID 0
#define SNDH_CONV_ID 1

static int sndh_info(disk68_t * mb, int len)
{
  const int unknowns_max = 8;
  int i, vbl = 0, frq = 0, time = 0 , musicmon = 0, steonly = 0,
    unknowns = 0, fail = 0;

  char * b = mb->data;
  char empty_tag[4] = { 0, 0, 0, 0 };

  /* Default */
  mb->mus[0].data   = b;
  mb->mus[0].datasz = len;
  mb->nb_mus = -1; /* Make validate failed */
  mb->mus[0].replay = ice68_version() ? 0 : "sndh_ice"; /* native ice depacker ? */
  mb->mus[0].tags.tag.custom[SNDH_RIPP_ID].key = tagstr.ripper;
  mb->mus[0].tags.tag.custom[SNDH_CONV_ID].key = tagstr.converter;

  i = sndh_is_magic(b, len);
  if (!i) {
    /* should not happen since we already have tested it. */
    msg68_critical("file68: sndh -- info mising magic!\n");
    return -1;
  }

/*   i   += 4; /\* Skip sndh_cc *\/ */
/*   len -= 4; */

  /* $$$ Hacky:
     Some music have 0 after values. I don't know what are
     sndh rules. May be 0 must be skipped or may be tag must be word
     aligned.
     Anyway the current parser allows a given number of successive
     unknown tags. May be this number should be increase in order to prevent
     some "large" unknown tag to break the parser.
  */

  while (i+4 < len) {
    char ** p;
    int j, t, s, ctypes;

    /* check char types for the next 4 chars */
    for (ctypes = 0, j=0; j<4; ++j) {
      ctypes |= (st_isgraph( b[i+j] ) << j );
      ctypes |= (st_isdigit( b[i+j] ) << (j + 8) );
    }

    TRACE68(file68_cat,
            "file68: sndh -- pos:%d/%d ctypes:%04X -- '%c%c%c%c'\n",
            i, len, ctypes, b[i+0], b[i+1], b[i+2], b[i+3]);

    t       = -1;                       /* offset on tag */
    s       = -1;                       /* offset on arg */
    p       = 0;                        /* store arg     */

    if (  (ctypes & 0x000F) != 0x000F ) {
      /* Not graphical ... should not be a valid tag */
    } else if (!memcmp(b+i,"SNDH",4)) {
      /* Header */
      t = i; i += 4;
    } else if (!memcmp(b+i,"COMM",4)) {
      /* Artist */
      t = i; s = i += 4;
      p = &mb->mus[0].tags.tag.artist.val;
    } else if (!memcmp(b+i,"TITL",4)) { /* title    */
      /* Title */
      t = i; s = i += 4;
      p = &mb->tags.tag.title.val;
    } else if (!memcmp(b+i,"RIPP",4)) {
      /* Ripper */
      t = i; s = i += 4;
      p = &mb->mus[0].tags.tag.custom[SNDH_RIPP_ID].val;
    } else if (!memcmp(b+i,"CONV",4)) {
      /* Converter */
      t = i; s = i += 4;
      p = &mb->mus[0].tags.tag.custom[SNDH_CONV_ID].val;
    } else if (!memcmp(b+i, "YEAR", 4)) {
      /* year */
      t = i; s = i += 4;
      /* p = ... */
    } else if (!memcmp(b+i,"MuMo",4)) {
      /* MusicMon ???  */
      msg68_warning("file68: sndh -- what to do with 'MuMo' tag ?\n");
      musicmon = 1;
      i += 4;
    } else if (!memcmp(b+i,"TIME",4)) {
      int j, tracks;
      /* Time in second */
      if (! (tracks = mb->nb_mus) ) {
        msg68_warning("file68: sndh -- got 'TIME' before track count\n");
        tracks = 1;
      }
      t = i; i += 4;
      for ( j = 0; j < tracks; ++j ) {
        if (i < len-2 && j < SC68_MAX_TRACK)
          mb->mus[j].first_ms = 1000u *
            ( ( ( (unsigned char) b[i]) << 8 ) | (unsigned char) b[i+1] );
	TRACE68(file68_cat,
		"file68: sndh -- TIME #%02 -- 0x%02X%02X (%c%c) -- %u ms\n",
		j+1, (unsigned char)b[i], (unsigned char)b[i+1],
		isgraph(b[i])?b[i]:'.',	isgraph(b[i+1])?b[i+1]:'.',
		mb->mus[j].first_ms);
        i += 2;
      }

    } else if ( !memcmp(b+i,"##",2) && ( (ctypes & 0xC00) == 0xC00 ) ) {
      mb->nb_mus = ( b[i+2] - '0' ) * 10 + ( b[i+3] - '0' );
      t = i; i += 4;
    } else if ( !memcmp(b+i,"!V",2) && ( (ctypes & 0xC00) == 0xC00 ) ) {
      vbl = ( b[i+2] - '0' ) * 10 + ( b[i+3] - '0' );
      i += 4;
    } else if (!memcmp(b+i,"**",2)) {
      /* FX + string 2 char ??? */
      msg68_warning("file68: sndh -- what to do with tag ? -- '**%c%c'\n",
                    b[i+2], b[i+3]);
      i += 4;
    } else if ( b[i] == 'T' && b[i+1] >= 'A' && b[i+1] <= 'D') {
      t = i; s = i += 2;
      myatoi(b, i, len, &frq);
    } else if( memcmp( b + i, empty_tag, 4 ) == 0 ||
               memcmp( b + i, "HDNS", 4 ) == 0 ) {
      t = i;
      i = len;
    } else {
      /* skip until next 0 byte, as long as it's inside the tag area */
      i += 4;
      while( *(b + i) != 0 && i < len ) {
        i++;
      }
    }

    if ( t < 0 ) {
      /* Unkwown tag, finish here. */
      ++unknowns;
      TRACE68(file68_cat,
              "file68: sndh -- not a tag at offset %d -- %02X%02X%02X%02X\n",
              i, b[i]&255, b[i+1]&255, b[i+2]&255, b[i+3]&255);
      ++i;

      if (fail || unknowns >= unknowns_max) {
        i = len;
      }

    } else {
      /* Well known tag */

      TRACE68(file68_cat,
              "file68: sndh -- got TAG -- '%c%c%c%c'\n",
              b[t], b[t+1], b[t+2], b[t+3]);
      unknowns = 0; /* Reset successive unkwown. */

      if (s >= 0) {
        int j, k;
        for ( j = s, k = s - 1; j < len && b[j]; ++j) {
          if ( b[j] < 32 ) b[j] = 32;
          else k = j;                   /* k is last non space char */
        }

        if (k+1 < len) {
          b[k+1] = 0;                   /* Strip triling space */
          i = k+2;
          if (p)
            *p = b+s;                     /* store tag */
          else
            TRACE68(file68_cat,"file68: sndh -- not storing -- '%s'\n",
                    b+s);

          /* HAXXX: using name can help determine STE needs */
          if (p == &mb->tags.tag.title.val)
            steonly = 0
              || !!strstr(mb->tags.tag.title.val,"STE only") 
              || !!strstr(mb->tags.tag.title.val,"(STe)")
              || !!strstr(mb->tags.tag.title.val,"(STE)")
              ;

          TRACE68(file68_cat,
                  "file68: sndh -- got ARG -- '%s'\n",
                  b+s);

        }

        /* skip the trailing null chars */
        for ( ; i < len && !b[i] ; i++ )
          ;
      }

    }
  }

  if (mb->nb_mus <= 0) {
    TRACE68(file68_cat, "file68: sndh -- %d track; assuming 1 track\n", mb->nb_mus);
    mb->nb_mus = 1;
  }

  if (mb->nb_mus > SC68_MAX_TRACK) {
    mb->nb_mus = SC68_MAX_TRACK;
  }
  time *= 1000u;

  for (i=0; i<mb->nb_mus; ++i) {
    mb->mus[i].d0    = i+1;
    mb->mus[i].loops = 0;
    mb->mus[i].frq   = frq ? frq : vbl;
    mb->mus[i].hwflags.bit.ym  = 1;
    mb->mus[i].hwflags.bit.ste = steonly;
  }
  return 0;
}

int file68_tag_count(disk68_t * mb, int track)
{
  int cnt = -1;

  if (mb && track >= 0 && track <= mb->nb_mus) {
    int idx;
    tagset68_t * tags = !track ? &mb->tags : &mb->mus[track-1].tags;
    for (idx = cnt = TAG68_ID_CUSTOM; idx<TAG68_ID_MAX; ++idx)
      if (tags->array[idx].key && tags->array[idx].val) {
        if (cnt != idx) {
          tags->array[cnt].key = tags->array[idx].key;
          tags->array[cnt].val = tags->array[idx].val;
        }
        ++cnt;
      }
  }
  return cnt;
}

int file68_tag_enum(const disk68_t * mb, int track, int idx,
                    const char ** key, const char ** val)
{
  const char * k = 0, * v = 0;

  if (mb && idx >= 0 && idx < TAG68_ID_MAX) {
    const tagset68_t * tags = 0;
    if (!track)
      tags = &mb->tags;
    else if (track > 0 && track <= mb->nb_mus)
      tags = &mb->mus[track-1].tags;
    if (tags) {
      k = tags->array[idx].key;
      v = tags->array[idx].val;
    }
  }
  if (key) *key = k;
  if (val) *val = v;

  return -!(k && v);
}

const char * file68_tag_get(const disk68_t * mb, int track, const char * key)
{
  const char * val = 0;
  if (mb) {
    const tagset68_t * tags = 0;
    if (!track)
      tags = &mb->tags;
    else if (track <= mb->nb_mus)
      tags = &mb->mus[track-1].tags;
    if (tags) {
      int i = get_customtag(tags, key);
      if (i >= 0)
        val = tags->array[i].val;
    }
  }
  return val;
}

const char * file68_tag_set(disk68_t * mb, int track, const char * key, const char * val)
{
  const char * ret = 0;

  if (mb && is_valid_key(key)) {
    tagset68_t * tags = 0;

    if (!track)
      tags = &mb->tags;
    else if (track <= mb->nb_mus)
      tags = &mb->mus[track-1].tags;
    if (tags) {
      int i = set_customtag(mb, tags, key, val);
      if (i >= 0)
        ret = tags->array[i].val;
      else
        ret = 0;
    }
  }

  return ret;
}

static void free_tags(disk68_t * mb, tagset68_t *tags)
{
  int i;
  for (i=0; i<TAG68_ID_MAX; ++i) {
    free_string(mb, tags->array[i].key); tags->array[i].key = 0;
    free_string(mb, tags->array[i].val); tags->array[i].val = 0;
  }
}

void file68_free(disk68_t * disk)
{
  if (disk) {
    const int max = disk->nb_mus;
    int i;

    free_tags(disk, &disk->tags);

    for (i=0; i<max; ++i) {
      free_tags(disk, &disk->mus[i].tags);
      if (disk->mus[i].data) {
        int j;
        free_string(disk, disk->mus[i].data);
        for (j=max-1; j>=i; --j) {
          if ( disk->mus[j].data == disk->mus[i].data )
            disk->mus[j].data   = 0;
          disk->mus[j].datasz = 0;
        }
        disk->mus[i].data = 0;
        disk->mus[i].datasz = 0;
      }
    }
    if (disk->data != disk->buffer) {
      free68(disk->data);
      disk->data = 0;
    }
    free68(disk);
  }
}

/* Allocate disk buffer */
static disk68_t * alloc_disk(int datasz)
{
  disk68_t * mb;
  int        room = datasz + sizeof(disk68_t) /* - sizeof(mb->buffer) always alloc extra bytes */;

  if (mb = calloc68(room), mb) {
    music68_t *cursix;

    /* data points into buffer */
    mb->data = mb->buffer;
    mb->datasz = datasz;

    /* Setup static tags */
    mb->tags.tag.title.key  = tagstr.title;
    mb->tags.tag.artist.key = tagstr.artist;
    mb->tags.tag.genre.key  = tagstr.format;
    for (cursix = mb->mus; cursix < mb->mus+SC68_MAX_TRACK; ++cursix) {
      cursix->tags.tag.title.key  = tagstr.title;
      cursix->tags.tag.artist.key = tagstr.artist;
      cursix->tags.tag.genre.key  = tagstr.genre;
    }
  }
  return mb;
}

disk68_t * file68_new(int extra)
{
  disk68_t * d = 0;
  if (extra < 0 || extra >= 1<<21)
    msg68_error("file68: invalid amount of extra data -- %d\n", extra);
  else
    d = alloc_disk(extra);
  return d;
}

/* Load , allocate memory and valid struct for SC68 music
 */
disk68_t * file68_load(istream68_t * is)
{
  disk68_t *mb = 0;
  int len;
  int chk_size;
  int opened = 0;
  music68_t *cursix;
  tagset68_t * tags;
  char *b;
  const char *fname = istream68_filename(is);
  const char *errorstr = 0;

  fname = strnevernull68(fname);

  /* Read header and get data length. */
  if (len = read_header(is), len < 0) {
    /* Verify tells it is a gzip or unice file, so we may give it a try.
     */
    if (1) {
      void * buffer = 0;
      int l;
      switch (len) {
      case -gzip_cc:
        /* gzipped */
        if (istream68_seek_to(is,0) == 0) {
          istream68_t * zis;
          zis=istream68_z_create(is,ISTREAM68_OPEN_READ,
                                 istream68_z_default_option);
          if (!istream68_open(zis)) {
            mb = file68_load(zis);
          }
          istream68_destroy(zis);
          if (mb) {
            goto already_valid;
          }
        }
        break;

      case -ice_cc:
        if (istream68_seek_to(is,0) == 0) {
          buffer = ice68_load(is, &l);
        }
        break;

      case -sndh_cc:
        if (istream68_seek_to(is,0) != 0) {
          break;
        }
        len = istream68_length(is);
        if (len <= 32 || len > 1<<21) {
          break;
        }
        mb = alloc_disk(len);
        if (!mb) {
          errorstr = "memory allocation";
          break;
        }
        mb->tags.tag.genre.val = tagstr.sndh;
        if (istream68_read(is, mb->data, len) != len) {
          break;
        }
        if (sndh_info(mb, len)) {
          break;
        }
        goto validate;
      }

      if (buffer) {
        mb = file68_load_mem(buffer, l);
        free68(buffer);
        if (mb) {
          return mb;
        }
      }
    }
    if (!errorstr)
      errorstr = "read header";
    goto error;
  }

  mb = alloc_disk(len);
  if (!mb) {
    errorstr = "memory allocation";
    goto error;
  }
  mb->tags.tag.genre.val = tagstr.sc68;

  if (istream68_read(is, mb->data, len) != len) {
    errorstr = "read data";
    goto error;
  }

  for (b = mb->data, cursix = 0, tags = &mb->tags;
       len >= 8;
       b += chk_size, len -= chk_size) {
    char chk[8];
    if (b[0] != 'S' || b[1] != 'C') {
      break;
    }

    chk[0] = b[2];
    chk[1] = b[3];
    chk[2] = 0;
    chk_size = LPeek(b + 4);
    b += 8;
    len -= 8;

    if (ISCHK(chk, CH68_BASE)) {
      /* nothing to do. */
    }
    /* Default track */
    else if (ISCHK(chk, CH68_DEFAULT)) {
      mb->def_mus = LPeek(b);
    }
    /* Album or track title */
    else if (ISCHK(chk, CH68_FNAME) || ISCHK(chk, CH68_MNAME)) {
      tags->tag.title.val = b;
    }
    /* Start music session. */
    else if (ISCHK(chk, CH68_MUSIC)) {
      if (mb->nb_mus == SC68_MAX_TRACK) {
        /* Can't have more than SC68_MAX_TRACK tracks */
        len = 0;
        break;
      }
      cursix = mb->mus + mb->nb_mus;
      /* cursix->loops    = 0;             /\* default loop        *\/ */
      /* cursix->loops_fr = ~0;            /\* loop length not set *\/ */
      tags = &cursix->tags;
      mb->nb_mus++;
    }
    /* Author name */
    else if (ISCHK(chk, CH68_ANAME)) {
      tags->tag.artist.val = b;
    }
    /* Composer name */
    else if (ISCHK(chk, CH68_CNAME)) {
      if (strcmp68(b,tags->tag.artist.val))
        set_customtag(mb, tags, tagstr.composer, b);
    }
    /* External replay */
    else if (ISCHK(chk, CH68_REPLAY)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->replay = b;
    }
    /* 68000 D0 init value */
    else if (ISCHK(chk, CH68_D0)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->d0 = LPeek(b);
    }
    /* 68000 memory load address */
    else if (ISCHK(chk, CH68_AT)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->a0 = LPeek(b);
    }
    /* Playing time (ms) */
    else if (ISCHK(chk, CH68_TIME)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->first_ms = LPeek(b) * 1000u;
    }
    /* Playing time (frames) */
    else if (ISCHK(chk, CH68_FRAME)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->first_fr = LPeek(b);
    }
    /* Replay frequency */
    else if (ISCHK(chk, CH68_FRQ)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->frq = LPeek(b);
    }
    /* Loop */
    else if (ISCHK(chk, CH68_LOOP)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->loops = LPeek(b);
      TRACE68(file68_cat, "file68: read track #%02d loop of -- *%d*\n",
              mb->nb_mus,cursix->loops);
    }
    /* Loop length */
    else if (ISCHK(chk, CH68_LOOPFR)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->loops_fr = LPeek(b);
      cursix->has.loop = 1;
    }
    /* SFX flag */
    else if (ISCHK(chk, CH68_SFX)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->has.sfx = 1;
    }
    /* Replay flags */
    else if (ISCHK(chk, CH68_TYP)) {
      int f;
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      f = LPeek(b);
      cursix->hwflags.all = 0;
      cursix->hwflags.bit.ym        = !! (f & SC68_YM);
      cursix->hwflags.bit.ste       = !! (f & SC68_STE);
      cursix->hwflags.bit.amiga     = !! (f & SC68_AMIGA);
      cursix->hwflags.bit.stechoice = !! (f & SC68_STECHOICE);
      cursix->hwflags.bit.timers    = !! (f & SC68_TIMERS);
      cursix->hwflags.bit.timera    = !! (f & SC68_TIMERA);
      cursix->hwflags.bit.timerb    = !! (f & SC68_TIMERB);
      cursix->hwflags.bit.timerc    = !! (f & SC68_TIMERC);
      cursix->hwflags.bit.timerd    = !! (f & SC68_TIMERD);
    }
    /* meta data */
    else if (ISCHK(chk, CH68_TAG)) {
      const char * key, * val;
      key = b;
      val = b + strlen(b) + 1;
      TRACE68(file68_cat,"file68: got a tag '%s' '%s'\n", key, val);
      if (set_customtag(mb, tags, key, val) < 0) {
        msg68_warning("file68: unable to set %s tag '%s' '%s'\n", cursix ? "track" : "disk", key, val);
      }
    }
    /* music data */
    else if (ISCHK(chk, CH68_MDATA)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->data = b;
      cursix->datasz = chk_size;
    }
    /* EOF */
    else if (ISCHK(chk, CH68_EOF)) {
      len = 0;
      break;
    }
  }

  /* Check it */
  if (len) {
    errorstr = "prematured end of file";
    goto error;
  }

validate:
  if (valid(mb)) {
    errorstr = "validation test";
    goto error;
  }

already_valid:
  if (opened) {
    istream68_close(is);
  }
  return mb;

error:
  if (opened) {
    istream68_close(is);
  }
  free68(mb);
  msg68_error("file68: load '%s' failed [%s]\n", fname, errorstr ? errorstr : "no reason");
  return 0;
}

static int get_version(const int version) {
  return version == 2 ? 2 : 1;
}

static void get_header(const int version, const char ** const header, int * const headsz)
{
  switch (get_version(version)) {
  case 1:
    *header = file68_idstr_v1;
    *headsz = sizeof(file68_idstr_v1);
    break;
  case 2:
    *header = file68_idstr_v2;
    *headsz = sizeof(file68_idstr_v2);
    break;
  }
}

#ifndef _FILE68_NO_SAVE_FUNCTION_

/* save CHUNK and data */
/* $$$ NEW: Add auto 16-bit alignement. */
static int save_chunk(istream68_t * os,
                      const char * chunk, const void * data, int size)
{
  static char zero[4] = {0,0,0,0};
  chunk68_t chk;
  int align;

  memcpy(chk.id, CH68_CHUNK, 2);
  memcpy(chk.id + 2, chunk, 2);
  align = size & 1;
  LPoke(chk.size, size + align);
  if (istream68_write(os, &chk, (int)sizeof(chunk68_t)) != sizeof(chunk68_t)) {
    goto error;
  }
  /* Special case data is 0 should happen only for SC68 total size
   * chunk.
   */
  if (size && data) {
    if (istream68_write(os, data, size) != size) {
      goto error;
    }
    if (align && istream68_write(os, zero, align) != align) {
      goto error;
    }
  }
  return 0;

error:
  return -1;
}

/* save CHUNK and string (only if non-0 & lenght>0) */
static int save_string(istream68_t * os,
                       const char * chunk, const char * str)
{
  int len;

  if (!str || !(len = strlen(str))) {
    return 0;
  }
  return save_chunk(os, chunk, str, len + 1);
}

/* save CHUNK and string (only if non-0 & lenght>0) */
static int save_noname(istream68_t * os,
                       const char * chunk, const char * str)
{
  return save_string(os, chunk, not_noname(str));
}

/* save CHUNK & string str ( only if oldstr!=str & lenght>0 ) */
static int save_differstr(istream68_t * os,
                          const char *chunk, char *str, char *oldstr)
{
  int len;

  if (oldstr == str
      || !str
      || (oldstr && !strcmp(oldstr, str))) {
    return 0;
  }
  len = strlen(str);
  return !len ? 0 :save_chunk(os, chunk, str, len + 1);
}

/* save CHUNK and 4 bytes Big Endian integer */
static int save_number(istream68_t * os, const char * chunk, int n)
{
  char number[4];

  LPoke(number, n);
  return save_chunk(os, chunk, number, 4);
}

/* save CHUNK and number (only if n!=0) */
static int save_nonzero(istream68_t * os, const char * chunk, int n)
{
  return !n ? 0 : save_number(os, chunk, n);
}

int file68_save_url(const char * fname, const disk68_t * mb,
                    int version, int gzip)
{
  istream68_t * os;
  int err;

  os = url_or_file_create(fname, 2, 0);
  err = file68_save(os, mb, version, gzip);
  istream68_destroy(os);

  return err;
}

static int save_tags(istream68_t *os, const tagset68_t * tags, int start, const char ** skip)
{
  int i, max = 0, err = 0;
  char * tmp = 0;

  for (i=start; i<TAG68_ID_MAX; ++i) {
    int keylen, vallen;

    /* Skip those tags. */
    if (skip) {
      const char ** skip_tag;
      for (skip_tag = skip; *skip_tag && strcmp68(*skip_tag, tags->array[i].key); skip_tag++)
        ;
      if (*skip_tag)
        continue;
    }

    if (tags->array[i].key && (keylen = strlen(tags->array[i].key)) &&
        tags->array[i].val && (vallen = strlen(tags->array[i].val))) {
      int len = keylen + vallen + 2;
      if (len > max) {
        char * new = realloc(tmp, len);
        if (!new) continue;
        tmp = new;
        max = len;
      }
      memcpy(tmp         ,tags->array[i].key, keylen+1);
      memcpy(tmp+keylen+1,tags->array[i].val, vallen+1);
      if (err = save_chunk(os, CH68_TAG, tmp, len), err)
        break;
    }
  }
  free(tmp);
  return err;
}

int file68_save_mem(const char * buffer, int len, const disk68_t * mb,
                    int version, int gzip)
{
  istream68_t * os;
  int err;

  os = istream68_mem_create((char *)buffer, len, 2);
  err = file68_save(os, mb, version, gzip);
  istream68_destroy(os);

  return err;
}

static const char * save_sc68(istream68_t * os, const disk68_t * mb, int len, int version);

/* Save disk into file. */
int file68_save(istream68_t * os, const disk68_t * mb, int version, int gzip)
{
  int len;
  const char * fname  = 0;
  const char * errstr = 0;
  istream68_t * null_os = 0;
  istream68_t * org_os  = 0;

  const char * header;
  int headsz;

  get_header(version, &header, &headsz);

  if (!os) {
    /* mia ??? */
  }

  /* Get filename (for error message) */
  fname = istream68_filename(os);

  /* Create a null stream to calculate total size.
     Needed by gzip stream that can't seek back. */
  null_os = istream68_null_create(fname);
  if (istream68_open(null_os)) {
    errstr = "open";
  } else {
    errstr = save_sc68(null_os, mb, 0, version);
  }
  if (errstr) {
    goto error;
  }
  len = istream68_length(null_os) - headsz;
  if (len <= 0) {
    errstr = "invalid stream length";
    goto error;
  }

  /* Wrap to gzip stream */
  if (gzip) {
    istream68_z_option_t gzopt;

    org_os = os;
    gzopt = istream68_z_default_option;
    gzopt.level = gzip;
    gzopt.name  = 0;
    os = istream68_z_create(org_os, 2, gzopt);
    if (istream68_open(os)) {
      errstr = "open";
      goto error;
    }
  }

  errstr = save_sc68(os, mb, len, version);

error:
  if (org_os) {
    /* Was gzipped: clean-up */
    istream68_destroy(os);
  }
  istream68_destroy(null_os);

  return errstr
    ? error68("file68: %s error -- %s",errstr,fname)
    : 0;
}

static const char * save_sc68(istream68_t * os, const disk68_t * mb, int len, int version)
{
  const char * errstr = 0;

  int opened = 0;

  const music68_t * mus;
  char * mname, * aname, * cname, * data;

  const char * header;
  int headsz;

  get_header(version, &header, &headsz);

  if (!os) {
    errstr = "null stream";
    goto error;
  }

  /* Check number of music */
  if (mb->nb_mus <= 0 || mb->nb_mus > SC68_MAX_TRACK) {
    errstr = "invalid number of track";
    goto error;
  }

  /* SC68 file header string */
  if (istream68_write(os, header, headsz) != headsz) {
    errstr = "header write";
    goto error;
  }
  /* SC68 disk-info chunks */
  if (save_chunk(os, CH68_BASE, 0, len)
      || save_noname  (os, CH68_FNAME,   mb->tags.tag.title.val)
      || save_noname  (os, CH68_ANAME,   mb->tags.tag.artist.val)
      || save_nonzero (os, CH68_DEFAULT, mb->def_mus)
      || save_tags    (os, &mb->tags, TAG68_ID_CUSTOM, 0)
    ) {
    errstr = "chunk write";
    goto error;
  }

  /* Reset previous value for various string */
  mname = mb->tags.tag.title.val;
  aname = mb->tags.tag.artist.val;
  cname = data = 0;
  for (mus = mb->mus; mus < mb->mus + mb->nb_mus; mus++) {
    int flags
      = 0
      | (mus->hwflags.bit.ym        ? SC68_YM     : 0)
      | (mus->hwflags.bit.ste       ? SC68_STE    : 0)
      | (mus->hwflags.bit.amiga     ? SC68_AMIGA  : 0)
      | (mus->hwflags.bit.stechoice ? SC68_STE    : 0)
      | (mus->hwflags.bit.timers    ? SC68_TIMERS : 0)
      | (mus->hwflags.bit.timera    ? SC68_TIMERA : 0)
      | (mus->hwflags.bit.timerb    ? SC68_TIMERB : 0)
      | (mus->hwflags.bit.timerc    ? SC68_TIMERC : 0)
      | (mus->hwflags.bit.timerd    ? SC68_TIMERD : 0)
      ;

    /* Save track-name, author, composer, replay */
    if (0
        || save_chunk(os, CH68_MUSIC, 0, 0) == -1
        || save_differstr(os, CH68_MNAME, mus->tags.tag.title.val,  mname)
        || save_differstr(os, CH68_ANAME, mus->tags.tag.artist.val, aname)
        || save_tags(os, &mus->tags, TAG68_ID_CUSTOM, 0) /* skip title and artist */
      ) {
      errstr = "chunk write";
      goto error;
    }
    if (mus->tags.tag.title.val) {
      mname = mus->tags.tag.title.val;
    }
    if (mus->tags.tag.artist.val) {
      aname = mus->tags.tag.artist.val;
    }

    /* Save play parms */
    if (0
        || save_string (os, CH68_REPLAY, mus->replay)
        || save_nonzero(os, CH68_D0,     mus->d0)
        || save_nonzero(os, CH68_AT,    !mus->has.pic     * mus->a0)
        || save_nonzero(os, CH68_FRQ,    (mus->frq != 50) * mus->frq)
        || save_nonzero(os, CH68_FRAME,  mus->has.time    * mus->first_fr)
        || save_nonzero(os, CH68_LOOP,   mus->has.loop * (mus->loops > 1) * mus->loops)
        || ( mus->has.loop &&
             save_number(os, CH68_LOOPFR,  mus->loops_fr) )
        || save_number (os, CH68_TYP,    flags)
        || ( mus->has.sfx &&
             save_chunk(os, CH68_SFX, 0, 0) )
      ) {
      errstr = "chunk write";
      goto error;
    }

    /* Save music data */
    if (mus->data && mus->data != data) {
      if (save_chunk(os, CH68_MDATA, mus->data, mus->datasz)) {
        errstr = "chunk write";
        goto error;
      }
      data = mus->data;
    }
  }

  /* SC68 last chunk */
  if (save_chunk(os, CH68_EOF, 0, 0)) {
    errstr = "chunk write";
    goto error;
  }

error:
  if (opened) {
    istream68_close(os);
  }
  return errstr;
}

#endif /* #ifndef _FILE68_NO_SAVE_FUNCTION_ */

const char * file68_identifier(int version)
{
  return version == 1
    ? file68_idstr_v1
    : file68_idstr_v2
    ;
}

const char * file68_mimetype(void)
{
  return file68_mimestr;
}

int file68_loader_init(void)
{
  file68_cat = msg68_cat("loader", "music file loader", DEBUG_FILE68_O);
  return 0;
}

void file68_loader_shutdown(void)
{
  msg68_cat_free(file68_cat);
  file68_cat = msg68_DEFAULT;
}