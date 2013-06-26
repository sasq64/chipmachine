/*
 * @file    option68.c
 * @brief   command line options
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-14 20:49:25 ben>
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
#include "msg68.h"
#include "alloc68.h"
#include "option68.h"
#include "string68.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static option68_t * opts;

#define FOREACH_OPT(opt) for (opt=opts; opt; opt = opt->next)

static inline int opt_isset(const option68_t * opt)
{
  return opt->has_arg < 0;
}

static inline void opt_free_str(option68_t * opt)
{
  if (opt->has_arg == ~option68_STR) {
    free68((void *) opt->val.str);
    opt->val.str = 0;
    opt->has_arg = option68_STR;
  }
}

static inline int opt_type(const option68_t * opt)
{
  return opt->has_arg >= 0
    ?  opt->has_arg
    : ~opt->has_arg
    ;
}

static inline void opt_unset(option68_t * opt)
{
  opt_free_str(opt);
  if (opt->has_arg < 0)
    opt->has_arg = ~opt->has_arg;
  opt->val.num = 0;
  opt->val.str = 0;
}

static inline int opt_set_bool(option68_t * opt, int val)
{
  opt_free_str(opt);
  opt->has_arg = ~option68_BOL;
  return opt->val.num = -!!val;
}

static inline int opt_set_int(option68_t * opt, int val)
{
  opt_free_str(opt);
  opt->has_arg = ~option68_INT;
  return opt->val.num = val;
}

static inline const char * opt_set_str(option68_t * opt, const char * val)
{
  opt_free_str(opt);
  opt->val.str = strdup68(val);
  if (opt->val.str)
    opt->has_arg = ~option68_STR;
  return opt->val.str;
}


static int opt_set_strtol(option68_t * opt, const char * val)
{
  int res = 0, ok = 0;
  int type = opt_type(opt);

  if (!val || !*val) {
    if (type == option68_BOL) {
      ok  = 1;
      res = 1;
    }
  } else if (!strcmp68(val,"yes")  ||
             !strcmp68(val,"true") ||
             !strcmp68(val,"on")) {
    ok  = 1;
    res = -1;
  } else if (!strcmp68(val,"no")    ||
             !strcmp68(val,"false") ||
             !strcmp68(val,"off")) {
    ok  = 1;
    res = 0;
  } else {
    ok = val[*val=='-'];
    if (ok >= '0' && ok <= '9') {
      res = strtol(val,0,0);
    } else {
      ok = 0;
      /* $$$ TODO */
    }
  }

  if (!ok) {
    res = opt->val.num;
  }
  if (type == option68_BOL) {
    res = opt_set_bool(opt, res);
  } else {
    res = opt_set_int(opt, res);
  }
  return res;
}

static option68_t * opt_of(const char * key)
{
  option68_t * opt;
  FOREACH_OPT(opt) if (!strcmp68(key, opt->name)) break;
  return opt;
}

int option68_type(const option68_t * opt)
{
  return opt ? opt_type(opt) : option68_ERR;
}

int option68_unset(option68_t * opt)
{
  int err = -1;
  if (opt) {
    opt_unset(opt);
    err = 0;
  }
  return err;
}

void option68_unset_all(void)
{
  option68_t * opt;
  FOREACH_OPT(opt) opt_unset(opt);
}

int option68_set(option68_t * opt, const char * str)
{
  int err = -1;
  if (opt) {
    err = 0;
    switch (opt_type(opt)) {
    case option68_STR:
      opt_set_str(opt,str); break;
    case option68_BOL:
    case option68_INT:
      opt_set_strtol(opt,str); break;
    default:
      err = -1;
    }
  }
  return err;
}

int option68_iset(option68_t * opt, int val)
{
  int err = -1;
  if (opt) {
    err = 0;
    switch (opt_type(opt)) {
    case option68_BOL:
      opt_set_bool(opt,val); break;
    case option68_INT:
      opt_set_int(opt,val); break;
    case option68_STR: {
      char tmp[128];
      snprintf(tmp,sizeof(tmp),"%d",val);
      tmp[sizeof(tmp)-1] = 0;
      opt_set_str(opt,tmp);
    } break;
    default:
      err = -1;
    }
  }
  return err;
}

int option68_parse(int argc, char ** argv, int reset)
{
  int i,n;
  option68_t * opt;

  /* Reset options */
  if (reset)
    option68_unset_all();

  /* Parse arguments */
  for (i=n=1; i<argc; ++i) {
    int negate = 0;
    char * arg = argv[i], * rearg;

    /* Check for `--' prefix */
    if (arg[0] != '-' || arg[1] != '-') {
      goto keep_it;             /* Not an option; keep it */
    }

    /* '--' breaks options parsing */
    if (!arg[2]) {
      argv[n++] = argv[i++];
      break;
    }
    arg += 2;

    /* Checking for sc68 prefixed options (--sc68- or --no-sc68-) */
    if (arg[0]=='n' && arg[1] == 'o' && arg[2] == '-') {
      negate = 1;
      arg += 3;
    }

    rearg = arg;
    FOREACH_OPT(opt) {
      const int opttype = opt_type(opt);
      arg = rearg;

      if (opt->prefix) {
        if (strncmp(arg,opt->prefix,opt->prefix_len)) {
          continue;             /* prefix does not match */
        }
        arg += opt->prefix_len;
      }

      if (strncmp(arg,opt->name,opt->name_len)) {
        continue;               /* name does not match */
      }

      arg += opt->name_len;
      if (*arg != 0 && *arg != '=') {
        continue;               /* name does not match (incomplet) */
      }

      if (0 == *arg) {
        if (opttype == option68_BOL) {
          opt_set_bool(opt,!negate); /* No arg required, set the option */
          break;
        }
        if (i+1 >= argc) {
          break;                /* $$$ should trigger an error */
        }
        arg = argv[++i];        /* Get next arg */
      } else {
        ++arg;
      }

      if (opttype == option68_STR) {
        /* string option; ``negate'' does not have much meaning. */
        opt_set_str(opt, arg);
      } else {
        opt_set_strtol(opt, arg);
        if (negate) opt_set_int(opt, ~opt->val.num);
      }
      break;
    }

    if (opt) continue;

    /* Not our option; keep it */
 keep_it:
    argv[n++] = argv[i];

  }

  /* Keep remaining arguments */
  for (; i<argc; ++i) {
    argv[n++] = argv[i];
  }
  argc = n;

  /* Get enviromment variables */
  FOREACH_OPT(opt) {
    if ( ! option68_isset(opt) )
      option68_getenv(opt, 1);
  }

  return argc;
}

int option68_init(void)
{
  opts = 0;
  return 0;
}

void option68_shutdown(void)
{
  option68_t * opt, * nxt;

  option68_unset_all();
  for (nxt=opts; (opt=nxt); ) {
    nxt = opt->next;
    opt->next = 0;
  }
  opts = 0;
}

int option68_append(option68_t * options, int n)
{
  int i;
  for (i=0; i<n; ++i) {
    if (!options[i].name || !*options[i].name) {
      msg68_warning("option68: invalid options name\n");
      continue;
    }
    if (options[i].next) {
      msg68_warning("option68: --%s%s already in used\n",
                    options[i].prefix ? options[i].prefix : "",
                    options[i].name);
      continue;
    }
    if (opt_isset(&options[i])) {
      msg68_warning("option68: --%s%s is already set\n",
                    options[i].prefix ? options[i].prefix : "",
                    options[i].name);
    }
    options[i].prefix_len = options[i].prefix ? strlen(options[i].prefix) : 0;
    options[i].name_len   = strlen(options[i].name);
    options[i].next       = opts;
    opts = options+i;
  }
  return 0;
}

option68_t * option68_get(const char * key, const int onlyset)
{
  option68_t * opt = 0;
  if (key && (opt = opt_of(key)) && onlyset && !opt_isset(opt)) {
    opt = 0;
  }
  return opt;
}

int option68_isset(const option68_t * option)
{
  return option
    ? opt_isset(option)
    : 0
    ;
}


/* Convert option name to envvar name */
static
char * opt2env(char * tmp, const int max, const char * name)
{
  int i,c;
  for (i=0; i<max && (c=*name++); ) {
    tmp[i++] = (c=='-')
      ? '_'
      : (c>'9') ? c+'A'-'a' : c
      ;
  }
  tmp[i] = 0;
  return tmp;
}

/* Get environment variable */
static
char * mygetenv(const char *name, const char * prefix, int prefix_len)
{
#ifndef HAVE_GETENV
  return 0;
#else
  char tmp[256];
  int  i = 0;
  if (prefix) {
    opt2env(tmp,sizeof(tmp)-1,prefix);
    i = prefix_len;
  }
  opt2env(tmp+i,sizeof(tmp)-1-i,name);
  return getenv(tmp);
#endif
}

const char * option68_getenv(option68_t * opt, int set)
{
  const char * val = opt
    ? mygetenv(opt->name, opt->prefix, opt->prefix_len)
    : 0
    ;

  if (val && set) {
    switch (opt_type(opt)) {
    case option68_STR: opt_set_str(opt,val); break;
    case option68_BOL: /* if (!val) val = "yes"; */
    case option68_INT: opt_set_strtol(opt,val); break;
    }
  }

  return val;
}

void option68_help(void * cookie, option68_help_t fct)
{
  if (fct) {
    char option[64] = "--sc68-", envvar[64];
    option68_t * opt;

    FOREACH_OPT(opt) {
      strncpy(option+7,opt->name,sizeof(option)-8);
      opt2env(envvar, sizeof(envvar)-1,option+2);
      switch (opt_type(opt)) {
      case option68_BOL: break;
      case option68_STR: strcat68(option,"=<str>",sizeof(option)); break;
      case option68_INT: strcat68(option,"=<int>",sizeof(option)); break;
      }
      fct (cookie, option, envvar, opt->desc);
    }
  }
}
