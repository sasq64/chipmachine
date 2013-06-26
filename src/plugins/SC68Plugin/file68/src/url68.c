/*
 * @file    url68.c
 * @brief   url parser and dispatcher
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-16 02:40:35 ben>
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
#include "url68.h"
#include "rsc68.h"

#include "string68.h"
#include "msg68.h"
#include "istream68_null.h"
#include "istream68_file.h"
#include "istream68_fd.h"
#include "istream68_curl.h"
#include "istream68_ao.h"

#include <string.h>
#include <ctype.h>

static int parse_protocol(char * protocol, int max, const char *url)
{
  int i, c;

  if (!url || !protocol || max < 4) {
    return 0;
  }
  *protocol = 0;
  for (c=i=0; i<max && (c=url[i], isalnum(c)); ++i)
    ;
  if (i<2 || i+2>=max || c!=':' || url[i+1]!='/' || url[i+2]!='/') {
    return 0;
  }
  memcpy(protocol, url, i);
  protocol[i] = 0;
  return i+3;
}

int url68_get_protocol(char * protocol, int max, const char *url)
{
  return -!parse_protocol(protocol, max, url);
}

int url68_local_protocol(const char * protocol)
{
  int i;

  static const char * local_proto[] = {
    "", "FILE","LOCAL","NULL"
    /* , "STDIN", "STDOUT" remove this (not seekable) */
  };
  const int n_proto = sizeof(local_proto)/sizeof(*local_proto);

  i = 0;
  if (protocol) {
    for (; i<n_proto && strcmp68(protocol, local_proto[i]); ++i)
      ;
  }
  return i < n_proto;
}

istream68_t * url68_stream_create(const char * url, int mode)
{
  char protocol[16];
  char tmp[512];
  const int max = sizeof(tmp)-1;
  istream68_t * isf = 0;
  int has_protocol;             /* in fact protocol:// length */

  has_protocol = parse_protocol(protocol, sizeof(protocol), url);
  if (has_protocol) {
    if (!strcmp68(protocol, "PASS")) {
      /* This is special pass thru protocol. It allows to send any
         other protocol:// or whatever to the default file handler.
         On some OS with some libC it may be useful.
      */
      url += has_protocol;      /* Skip protocol:// part */
      has_protocol = 0;         /* Allow fallback open   */
    } else if (!strcmp68(protocol, "RSC68")) {
      isf = rsc68_create_url(url, mode, 0);
    } else if (!strcmp68(protocol, "SC68")) {
      /* sc68://author/hw/title/track:loop */
      url += has_protocol;      /* Skip protocol:// part */
      strncpy(tmp, "rsc68://music/",max);
      strncpy(tmp+14, url, max-14);
      tmp[max] = 0;
      msg68(-1,"url is now [%s]\n",tmp);
      isf = rsc68_create_url(tmp, mode, 0);
    } else if (!strcmp68(protocol, "FILE") ||
               !strcmp68(protocol, "LOCAL")) {
      url += has_protocol;      /* Skip protocol:// part */
      has_protocol = 0;         /* Allow fallback open */
    } else if (!strcmp68(protocol, "NULL")) {
      isf = istream68_null_create(url);
    } else if (!strcmp68(protocol, "AUDIO")) {
      url += 5+3;
      isf  = istream68_ao_create(url,mode);
    } else if (!strcmp68(protocol, "STDIN")) {
      if (mode != 1) return 0;  /* stdin is READ_ONLY */
      isf = istream68_fd_create("stdin://",0,1);
      url = "/dev/stdin";       /* fallback */
      has_protocol = 0;         /* Allow fallback open */
    } else if (!strcmp68(protocol, "STDOUT")) {
      if (mode != 2) return 0;  /* stdout is WRITE_ONLY */
      isf = istream68_fd_create("stdout://",1,2);
      url = "/dev/stdout";      /* fallback */
      has_protocol = 0;         /* Allow fallback open */
    } else if (!strcmp68(protocol, "STDERR")) {
      if (mode != 2) return 0;  /* stderr is WRITE_ONLY */
      isf = istream68_fd_create("stderr://",2,2);
      url = "/dev/stderr";      /* fallback */
      has_protocol = 0;         /* Allow fallback open */
    } else {
      /* Try cURL for all unknown protocol */
      isf = istream68_curl_create(url,mode);
    }
  }

  /* Fallback open only if no protocol (or explicitly allowed) */
  if (!has_protocol) {
    if (!isf) {                 /* Default open as FILE */
      isf = istream68_file_create(url,mode);
    }
    if (!isf) {                 /* Fallback to file descriptor */
      isf = istream68_fd_create(url,-1,mode);
    }
  }

  msg68_debug("url68: create url='%s' %c%c => [%s,'%s']\n",
              strnevernull68(url),
              (mode&1) ? 'R' : '.',
              (mode&2) ? 'W' : '.',
              strok68(!isf),
              istream68_filename(isf));

  return isf;
}
