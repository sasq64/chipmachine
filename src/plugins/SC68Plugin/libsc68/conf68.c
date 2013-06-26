/*
 * @file    conf68.c
 * @brief   sc68 config file
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-06 14:23:19 ben>
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
#include "config.h"
#endif

#ifdef HAVE_IO68_CONFIG_OPTION68_H
# include "io68/config_option68.h"
#else
# include "io68/default_option68.h"
#endif

#include "conf68.h"

/* file68 headers */
#include <sc68/error68.h>
#include <sc68/file68.h>
#include <sc68/url68.h>
#include <sc68/string68.h>
#include <sc68/msg68.h>
#include <sc68/alloc68.h>
#include <sc68/option68.h>

/* standard headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef DEBUG_CONFIG68_O
# define DEBUG_CONFIG68_O 0
#endif
static int config68_cat = msg68_DEFAULT;

typedef union {
  int i;          /**< Used with CONFIG68_INT fields.     */
  const char * s; /**< Used with CONFIG68_STR fields.  */
} config68_value_t;

typedef struct _config68_field_s config68_field_t;

typedef struct
{
  int              exported; /**< exported as cli option.    */
  const char      *name;     /**< name of the entry.         */
  config68_type_t  type;     /**< Type of this config entry. */
  const char      *comment;  /**< Comment.                   */
  config68_value_t min;      /**< Minimum value allowed.     */
  config68_value_t max;      /**< Maximum value allowed.     */
  config68_value_t def;      /**< Default value the entry.   */
  config68_value_t val;      /**< Value for the entry.       */
} config68_entry_t;

struct _config68_s {
  int saved;    /**< True if config has been saved. */
  int size;     /**< Number of entries allocated.   */
  int n;        /**< Number of entries in used.     */

  /** Config entry table.
   * @warning Must be at the end of the struct.
   */
  config68_entry_t entries[1];
};


/* Defines for the config default values. */
#define AMIGA_BLEND      0x5000     /* Amiga default blending factor. */
#define DEFAULT_TIME     (3*60)     /* Track default time in second.  */
#define FORCE_TRACK      1          /* 0:no forced track              */
#define SKIP_TIME        4          /* Skip music time in sec.        */
#define MAX_TIME        (24*60*60)  /* 1 day should be enought.       */
#define DEFAULT_SEEKSPD 0x0F00      /* << 8 */
#define MAX_SEEKSPD     0x1F00

static const config68_entry_t conftab[] = {
  { 0,
    "version", CONFIG68_INT,
    "major*100+minor",
    {0}, {10000}, {PACKAGE_VERNUM}
  },
  { 0,                          /* controled by application */
    "sampling-rate", CONFIG68_INT,
    "sampling rate in Hz",
    {SAMPLING_RATE_MIN},{SAMPLING_RATE_MAX},{SAMPLING_RATE_DEF}
  },
  { 1,
    "amiga-blend", CONFIG68_INT,
    "Amiga left/right voices blending factor {32768:center}",
    {0},{65535},{AMIGA_BLEND}
  },

  { 0,                          /* could be export but is it useful */
    "force-track", CONFIG68_INT,
    "override default track {0:off}",
    {0}, {99}, {FORCE_TRACK}
  },
  { 0,                          /* could be export but is it useful */
    "force-loop", CONFIG68_INT,
    "override default loop {-1:off 0:inf}",
    {-1}, {100}, {-1}
  },
  { 1,
    "skip-time", CONFIG68_INT,
    "prevent short track from being played (in sec) {0:off}",
    {0}, {MAX_TIME}, {SKIP_TIME}
  },
  { 1,
    "default-time", CONFIG68_INT,
    "default track time (in second)",
    {0}, {MAX_TIME}, {DEFAULT_TIME}
  },
  { 0,                          /* could be export but is it useful */
    "seek-speed", CONFIG68_INT,
    "seek speed factor {0:OFF 256:1 512:2 ...}",
    {0}, {MAX_SEEKSPD}, {DEFAULT_SEEKSPD}
  },
  { 0,
    "total-time", CONFIG68_INT,
    "total playing time since first launch",
    {0}, {0}, {0}
  },
  { 0,
    "total-ms", CONFIG68_INT,
    "total-time adjustment",
    {0}, {999}, {0}
  },
  { 0,                          /* already exported by file68 */
    "music-path", CONFIG68_STR,
    "local sc68 music path",
    {0}, {0}, {0}
  },
  { 0,                          /* currently unsupported */
    "allow-remote", CONFIG68_INT,
    "enable remote access (using curl) (disable is not upported)",
    {0}, {1}, {1}
  },
  { 0,                          /* already exported by file68 */
    "remote-music-path", CONFIG68_STR,
    "remote sc68 music path",
    {0}, {0}, {0}
  }
};

static const int nconfig = sizeof(conftab) / sizeof(conftab[0]);

static const char config_header[] =
  "# -*- conf-mode -*-\n"
  "#\n"
  "# sc68 config file generated by " PACKAGE_STRING "\n"
  "#\n"
  "# " PACKAGE_URL "\n"
  "#\n"
  "# You can edit this file. If you remove it, sc68 will generate\n"
  "# a new one at start-up with default values, but you will lost your\n"
  "# total playing time. To avoid it, you should either save its value\n"
  "# or delete all lines you want to be resetted.\n"
  "# - *int* : integer values; \"C\" number format (e.g.0xFE0120).\n"
  "# - *str* : String values; quoted with (\"); must not contain (\").\n"
  "#\n";

static int is_symbol_char(int c)
{
  return
    (c>='0' && c<='9')
    || (c>='a' && c<='z')
    || (c>='A' && c<='Z')
    || c=='_'
    || c=='.';
}

static int digit(int c, unsigned int base)
{
  int n = -1;
  if (c <= '9') {
    n = c - '0';
  } else if (c <= 'Z') {
    n = c - 'A' + 10;
  } else if (c <= 'z'){
    n = c - 'a' + 10;
  }
  if ((unsigned int)n < base) {
    return n;
  }
  return -1;
}

#ifdef HAVE_STRTOL
# define mystrtoul strtol
#else

/** $$$ Need this function for dcplaya version. */
static long mystrtoul(const char * s,
                      char * * end,
                      unsigned int base)
{
  const char * start = s;
  unsigned long v = 0;
  int neg = 0, c;

  /* Skip starting spaces. */
  for (c = *s; (c == ' ' || c == 9 || c == 10 || c == 13); c = *++s)
    ;

  /* Get optionnal sign. */
  /* $$$ ben : Does not make sens with unsigned value ! */
  if (c == '-' || c == '+') {
    neg = (c == '-');
    c = *++s;
  }

  /* Get the base. */
  if (!base) {
    /* Assume default base is 10 */
    base = 10;

    /* Could be either octal(8) or hexidecimal(16) */
    if (c == '0') {
      base = 8;
      c = *++s;
      if (c == 'x' || c == 'X') {
        base = 16;
        c = *++s;
      }
    }
  } else if (base == 16 && c == '0') {
    /* Hexa mode must skip "0x" sequence */
    c = *++s;
    if (c == 'x' || c == 'X') {
      c = *++s;
    }
  }

  c = digit(c,base);
  if (c < 0) {
    s = start;
  } else {
    do {
      v = v * base + c;
      c = digit(*++s,base);
    } while (c >= 0);
  }

  if (end) {
    *end = (char *)s;
  }

  return neg ? -(signed long)v : v;
}

#endif

static int config_set_int(config68_t * conf, config68_entry_t *e, int v)
{
  int m,M;

  if (e->type != CONFIG68_INT) {
    TRACE68(config68_cat,
            "conf: set int name='%s' bad type (%d)\n", e->name, e->type);
    return -1;
  }
  m = e->min.i;
  M = e->max.i;

  /*   TRACE68(config68_cat, */
  /*           "conf: set int name='%s' [%d..%d] cur:%d req:%d \n", */
  /*           e->name, m, M, e->val.i, v); */

  if (m != M) {
    if (m==0 && M == 1) {
      v = !!v;
    } else if (v < m) {
      v = m;
    } else if (v > M) {
      v = M;
    }
  }

  if (v != e->val.i) {
    conf->saved = 0;
    e->val.i = v;
    TRACE68(config68_cat,
            "conf: set int name='%s' [%d..%d] new:%d\n",
            e->name, m, M, e->val.i);
  }
  return 0;
}

static int config_set_str(config68_t * conf, config68_entry_t *e,
                          const char * s)
{
  int err = 0;
  int m,M;

  if (e->type != CONFIG68_STR) {
    return -1;
  }

  m = e->min.i;
  M = e->max.i;
  if (m != M) {
    int v = s ? strlen(s) : 0;
    if (v < m || v > M) {
      s = 0;
      err = -1;
    }
  }

  if (!s) {
    if (e->val.s) {
      free68((void*)e->val.s);
      e->val.s = 0;
      conf->saved = 0;
    }
  } else if (!e->val.s || strcmp(s,e->val.s)) {
    free68((void*)e->val.s);
    e->val.s = strdup68(s);
    err = -!e->val.s;
    conf->saved = 0;
  }

  return err;
}


/* Check config values and correct invalid ones
 */
int config68_valid(config68_t * conf)
{
  int err = 0;
  int i;

  if (!conf) {
    return -1;
  }

  for (i=0; i < conf->n; i++) {
    config68_entry_t *e = conf->entries+i;
    switch (e->type) {
    case CONFIG68_INT:
      err |= config_set_int(conf, e, e->val.i);
      break;
    case CONFIG68_STR:
      err |= config_set_str(conf, e, e->val.s);
      break;
    default:
      err = -1;
    }
  }

  return -!!err;
}

static int keycmp(const char * k1, const char * k2)
{
  int c1,c2;

  if (k1 == k2) return 0;
  if (!k1) return -1;
  if (!k2) return  1;
  do {
    c1 = *k1++; if (c1 == '_') c1 = '-';
    c2 = *k2++; if (c2 == '_') c2 = '-';
  } while (c1 == c2 && c1);
  return c1 - c2;
}


int config68_get_idx(const config68_t * conf, const char * name)
{
  if (!conf) {
    return -1;
  }
  if (name) {
    int i;
    for (i=0; i<conf->n; i++) {
      if (!keycmp(name, conf->entries[i].name)) {
        return i;
      }
    }
  }
  return -1;
}

config68_type_t config68_range(const config68_t * conf, int idx,
                               int * min, int * max, int * def)
{
  config68_type_t type = CONFIG68_ERR;
  int vmin = 0 , vmax = 0, vdef = 0;

  if (conf && idx >= 0 && idx < conf->n) {
    type = conf->entries[idx].type;
    vmin = conf->entries[idx].min.i;
    vmax = conf->entries[idx].max.i;
    vdef = conf->entries[idx].def.i;
  }
  if (min) *min = vmin;
  if (max) *max = vmax;
  if (def) *def = vdef;
  return type;
}

config68_type_t config68_get(const config68_t * conf,
                             int * v,
                             const char ** name)
{
  int idx;
  config68_type_t type = CONFIG68_ERR;

  if (conf) {
    idx = v ? *v : -1;
    if (idx == -1 && name) {
      idx = config68_get_idx(conf, *name);
    }
    if (idx >= 0 && idx < conf->n) {
      switch (type = conf->entries[idx].type) {
      case CONFIG68_INT:
        if (v) *v = conf->entries[idx].val.i;
        break;

      case CONFIG68_STR:
        if (name) *name = conf->entries[idx].val.s
                    ? conf->entries[idx].val.s
                    : conf->entries[idx].def.s;
        break;

      default:
        type = CONFIG68_ERR;
        break;
      }
    }
  }
  return type;
}

config68_type_t config68_set(config68_t * conf, int idx, const char * name,
                             int v, const char * s)
{
  config68_type_t type = CONFIG68_ERR;
  if (conf) {
    if (name) {
      idx = config68_get_idx(conf, name);
    }
    if (idx >= 0 && idx < conf->n) {
      switch (type = conf->entries[idx].type) {
      case CONFIG68_INT:
        config_set_int(conf, conf->entries+idx, v);
        break;

      case CONFIG68_STR:
        if (!config_set_str(conf, conf->entries+idx, s)) {
          break;
        }
      default:
        type = CONFIG68_ERR;
        break;
      }
    }
  }
  return type;
}

static int save_config_entry(istream68_t * os, const config68_entry_t * e)
{
  int i,err = 0;
  char tmp[64];

  err |= istream68_puts(os, "\n# ") < 0;
  err |= istream68_puts(os, e->comment) < 0;

  switch (e->type) {
  case CONFIG68_INT:
    sprintf(tmp, "; *int* [%d..%d]", e->min.i, e->max.i);
    err |= istream68_puts(os, tmp) < 0;
    sprintf(tmp, " (%d)\n", e->def.i);
    err |= istream68_puts(os, tmp) < 0;
    break;

  case CONFIG68_STR:
    err |= istream68_puts(os, "; *str* (\"") < 0;
    err |= istream68_puts(os, e->def.s) < 0;
    err |= istream68_puts(os, "\")\n") < 0;
    break;

  default:
    istream68_puts(os, e->name);
    istream68_puts(os, ": invalid type\n");
    return -1;
  }

  /* transform name */
  for (i=0; e->name[i]; ++i) {
    int c = e->name[i];
    tmp[i] = (c == '-') ? '_' : c;
  }
  tmp[i] = 0;

  switch (e->type) {
  case CONFIG68_INT:
    err |= istream68_puts(os, tmp) < 0;
    err |= istream68_putc(os, '=') < 0;
    sprintf(tmp, "%d", e->val.i);
    err |= istream68_puts(os, tmp) < 0;
    TRACE68(config68_cat,"conf: save name='%s'=%d\n",e->name,e->val.i);
    break;

  case CONFIG68_STR: {
    const char * s = e->val.s ? e->val.s : e->def.s;
    if (!s) {
      err |= istream68_putc(os, '#') < 0;
    }
    err |= istream68_puts(os, tmp) < 0;
    err |= istream68_putc(os, '=') < 0;
    err |= istream68_putc(os, '"') < 0;
    err |= istream68_puts(os, s) < 0;
    err |= istream68_putc(os, '"') < 0;
    TRACE68(config68_cat,"conf: save name='%s'=\"%s\"\n",e->name,s);
  } break;

  default:
    break;
  }
  err |= istream68_putc(os, '\n') < 0;
  return err;
}

int config68_save(config68_t * conf)
{
  int i,err;
  istream68_t * os=0;
  const int sizeof_config_hd = sizeof(config_header)-1; /* Remove '\0' */

  TRACE68(config68_cat,"conf: saving ...\n", conf);

  if (!conf) {
    err = error68(0,"conf: null pointer");
    goto error;
  }
  os = url68_stream_create("RSC68://config", 2);
  err = istream68_open(os);
  if (!err) {
    TRACE68(config68_cat,"conf: save into \"%s\"\n",
            istream68_filename(os));
    err =
      - (istream68_write(os, config_header, sizeof_config_hd)
         != sizeof_config_hd);
  }
  for (i=0; !err && i < conf->n; ++i) {
    err = save_config_entry(os, conf->entries+i);
  }

 error:
  istream68_close(os);
  istream68_destroy(os);

  TRACE68(config68_cat, "config68_save => [%s]\n",strok68(err));
  return err;
}


/* Load config from file
 */
int config68_load(config68_t * conf)
{
  istream68_t * is = 0;
  char s[1024], * word;
  int err;
  config68_type_t type;

  TRACE68(config68_cat, "conf: loading ...\n",conf);

  err = config68_default(conf);
  if (err) {
    goto error;
  }

  is = url68_stream_create("RSC68://config", 1);
  err = istream68_open(is);
  if (err) {
    goto error;
  }
  TRACE68(config68_cat, "conf: filename='%s'\n",
          istream68_filename(is));

  for(;;) {
    char * name;
    int i, len, c = 0, idx;

    len = istream68_gets(is, s, sizeof(s));
    if (len == -1) {
      err = -1;
      break;
    }
    if (len == 0) {
      break;
    }

    i = 0;

    /* Skip space */
    while (i < len && (c=s[i++], (c == ' ' || c == 9)))
      ;

    if (!is_symbol_char(c)) {
      continue;
    }

    /* Get symbol name. */
    name = s+i-1;
    while (i < len && is_symbol_char(c = s[i++]))
      if (c == '_') s[i-1] = c = '-';
    s[i-1] = 0;

    /* TRACE68(config68_cat,"conf: load get key name='%s\n", name); */

    /* Skip space */
    while (i < len && (c == ' ' || c == 9)) {
      c=s[i++];
    }

    /* Must have '=' */
    if (c != '=') {
      continue;
    }
    c=s[i++];
    /* Skip space */
    while (i < len && (c == ' ' || c == 9)) {
      c=s[i++];
    }

    if (c == '"') {
      type = CONFIG68_STR;
      word = s + i;
      /*       TRACE68(config68_cat, */
      /*               "conf: load name='%s' looks like a string(%d)\n", */
      /*               name, type); */
    } else if (c == '-' || digit(c, 10) != -1) {
      type = CONFIG68_INT;
      word = s + i - 1;
      /*       TRACE68(config68_cat, */
      /*               "conf: load name='%s' looks like an int(%d)\n", name, type); */
    } else {
      TRACE68(config68_cat,
              "conf: load name='%s' looks like nothing valid\n", name);
      continue;
    }
    /*     TRACE68(config68_cat, */
    /*             "conf: load name='%s' not parsed value='%s'\n", name, word); */

    idx = config68_get_idx(conf, name);
    if (idx < 0) {
      /* Create this config entry */
      TRACE68(config68_cat, "conf: load name='%s' unknown\n", name);
      continue;
    }
    if (conf->entries[idx].type != type) {
      TRACE68(config68_cat, "conf: load name='%s' types differ\n", name);
      continue;
    }

    switch (type) {
    case CONFIG68_INT:
      config_set_int(conf, conf->entries+idx, mystrtoul(word, 0, 0));
      TRACE68(config68_cat, "conf: load name='%s'=%d\n",
              conf->entries[idx].name, conf->entries[idx].val.i);
      break;
    case CONFIG68_STR:
      while (i < len && (c=s[i++], c && c != '"'))
        ;
      s[i-1] = 0;
      config_set_str(conf, conf->entries+idx, word);
      TRACE68(config68_cat, "conf: load name='%s'=\"%s\"\n",
              conf->entries[idx].name, conf->entries[idx].val.s);
    default:
      break;
    }
  }
  if (!err) {
    err = config68_valid(conf);
  }

 error:
  istream68_destroy(is);
  TRACE68(config68_cat, "conf: loaded => [%s]\n",strok68(err));
  return err;
}

/* Fill config struct with default values.
 */
int config68_default(config68_t * conf)
{
  int i;

  if(!conf) {
    return -1;
  }
  for (i=0; i < conf->n; i++) {
    config68_entry_t * e = conf->entries+i;
    switch (e->type) {
    case CONFIG68_INT:
      e->val.i = e->def.i;
      break;
    case CONFIG68_STR:
      free68((void*)e->val.s);
      e->val.s = 0;
    default:
      break;
    }
  }
  conf->saved = 0;
  return config68_valid(conf);
}

config68_t * config68_create(int size)
{
  config68_t * c;
  int i,j;

  TRACE68(config68_cat, "config68: create size=%d\n",size);

  if (size < nconfig) {
    size = nconfig;
  }
  c = alloc68(sizeof(*c)-sizeof(c->entries)+sizeof(*c->entries)*size);
  if (c) {
    c->size = size;
    c->saved = 0;
    for (j=i=0; i<nconfig; ++i) {
      c->entries[j] = conftab[i];
      switch(c->entries[j].type) {
      case CONFIG68_INT:
        config_set_int(c, c->entries+j, c->entries[j].def.i);
        ++j;
        break;

      case CONFIG68_STR:
        c->entries[j].val.s = 0;
        c->entries[j].def.s = 0;
        config_set_str(c, c->entries+j, 0);
        ++j;
        break;

      default:
        break;
      }
    }
    c->n = j;
  }
  TRACE68(config68_cat, "config68: create => [%s]\n",strok68(!c));

  return c;
}

void config68_destroy(config68_t * c)
{
  TRACE68(config68_cat, "config68: destroy [%p]\n",c);
  if (c) {
    int i;

    for (i=0; i<c->n; ++i) {
      if (c->entries[i].type == CONFIG68_STR) {
        free68((void*)c->entries[i].val.s);
      }
    }
    free68(c);
  }
}

option68_t * config68_options;
int config68_option_count;

int config68_init(void)
{
  if (config68_cat == msg68_DEFAULT) {
    int f = msg68_cat("conf","config file", DEBUG_CONFIG68_O);
    if (f > 0) config68_cat = f;
  }

  if (!config68_options) {
    int i,n;
    option68_t * options = 0;

    /* count exported config key. */
    for (i=n=0; i<nconfig; ++i) {
      n += !!conftab[i].exported;
    }

    TRACE68(config68_cat,"config68: got %d exportable keys\n",n);

    if (n > 0) {
      options = alloc68(n*sizeof(*options));
      if (!options) {
        msg68_error("conf: alloc error\n");
      } else {
        int j;
        for (i=j=0; i<nconfig; ++i) {
          if (!conftab[i].exported) continue;
          options[j].has_arg = (conftab[i].type == CONFIG68_INT)
            ? option68_INT : option68_STR;
          options[j].prefix  = "sc68-";
          options[j].name    = conftab[i].name;
          options[j].cat     = "config";
          options[j].desc    = conftab[i].comment;
          options[j].val.num = 0;
          options[j].val.str = 0;
          options[j].next    = 0;
          options[j].name_len =
            options[j].prefix_len = 0;
          TRACE68(config68_cat,"config68: export %s %s%s\n",
                  options[j].cat, options[j].prefix, options[j].name);
          ++j;
        }
      }
    }
    config68_options      = options;
    config68_option_count = n;
  }
  return 0;
}

void config68_shutdown()
{
  /* release options */
  if (config68_options) {
    int i;
    for (i=0; i<config68_option_count; ++i) {
      if (config68_options[i].next) {
        msg68_critical("config68: option #%d '%s' still attached\n", i, config68_options[i].name);
        break;
      }
    }
    if (i == config68_option_count)
      free68(config68_options);
    config68_options = 0;
    config68_option_count = 0;
  }

  /* release debug feature. */
  if (config68_cat != msg68_DEFAULT) {
    msg68_cat_free(config68_cat);
    config68_cat = msg68_DEFAULT;
  }
}
