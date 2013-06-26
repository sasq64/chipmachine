/*
 * @file    registry68.c
 * @brief   ms-Windows registry
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-10-02 16:04:51 ben>
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
# include <config.h>
#endif

#include "registry68.h"

#ifdef USE_REGISTRY68

int registry68_support(void) { return 1; }

#include "msg68.h"
#include "string68.h"
#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <string.h>

#ifndef HKEY_INVALID
# define HKEY_INVALID ((HKEY)0)
#endif

/* this table *MUST* match enum registry68_key_e definition. */
static struct reg68_keytable_s {
  const char name[4];
  HKEY hkey;
} keys[REGISTRY68_LST+1] = {
  { /* 00 */ {'C','R','K',0}, HKEY_CLASSES_ROOT     },
  { /* 01 */ {'C','U','K',0}, HKEY_CURRENT_USER     },
  { /* 02 */ {'L','M','K',0}, HKEY_LOCAL_MACHINE    },
  { /* 03 */ {'U','S','K',0}, HKEY_USERS            },
  { /* 04 */ {'P','D','K',0}, HKEY_PERFORMANCE_DATA },
  { /* 05 */ {'C','C','K',0}, HKEY_CURRENT_CONFIG   },
  { /* 06 */ {'D','D','K',0}, HKEY_DYN_DATA         },
  { /* xx */ {'I','N','K',0}, HKEY_INVALID          }
};

static const char * keyhdlname(const registry68_key_t key)
{
  int i;

  for (i=0; i < REGISTRY68_LST && (HKEY)key != keys[i].hkey; ++i)
    ;
  return keys[i].name;
}

static const char * keyname(const enum registry68_key_e const key)
{
  return keys
    [(unsigned int)key >= (unsigned int)REGISTRY68_LST
     ? REGISTRY68_LST
     : key
      ] . name;
}

static HKEY keyhandle(const enum registry68_key_e const key)
{
  return keys
    [(unsigned int)key >= (unsigned int)REGISTRY68_LST
     ? REGISTRY68_LST
     : key
      ] . hkey;
}

registry68_key_t registry68_rootkey(enum registry68_key_e rootkey)
{
  registry68_key_t key = (registry68_key_t) keyhandle(rootkey);
  /* msg68_debug("registry68: rootkey %d '%s' => %p '%s'\n", */
  /*             rootkey, keyname(rootkey), (void*)key, keyhdlname(key)); */
  return key;
}

static void SetSystemError(char * str, int max)
{
  char registry68_errorstr[256];
  int err = GetLastError(), l;
  registry68_errorstr[0] = 0;

  if ( err == ERROR_SUCCESS) return;

  if (!str) {
    msg68_critical("registry68: race condition may occur\n");
    str = registry68_errorstr;
    max = sizeof(registry68_errorstr);
  }
  FormatMessage(
    /* source and processing options */
    FORMAT_MESSAGE_FROM_SYSTEM,
    /* pointer to message source */
    0,
    /* requested message identifier */
    err,
    /* language identifier for requested message */
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    /* pointer to message buffer */
    str,
    /* maximum size of message buffer */
    max,
    /* pointer to array of message inserts */
    0);
  str[max-1] = 0;
  l = strlen(str);
  while (--l>=0 && (str[l] == '\n' || str[l] == '\r' ||str[l] == ' '))
    str[l] = 0;
  msg68_error("registry68: system error -- '%s'\n", str);
}

static int GetOneLevelKey(HKEY hkey, char **kname, HKEY *hkeyres)
{
  char *s;
  LONG hres;

  /* Find next sub-key */
  s=strchr(*kname,'/');
  if (s) {
    s[0] = 0;
  }

  /* Open this sub-key */
  hres =
    RegOpenKeyEx(hkey,     // handle to open key
                 *kname,   // address of name of subkey to open
                 0,        // reserved
                 KEY_READ, // security access mask
                 hkeyres   // address of handle to open key
      );

  /* If next sub-key exist, advance pointer to beginning of key-name */
  if (s) {
    s[0] = '/';
    ++s;
  }

  if (hres != ERROR_SUCCESS) {
    *hkeyres = HKEY_INVALID;
    return -1;
  } else {
    *kname = s;
    return 0;
  }
}

static int rGetRegistryKey(HKEY hkey, char *kname, HKEY *hkeyres)
{
  HKEY subkey = HKEY_INVALID;
  int err;

  if (!kname) {
    *hkeyres = hkey;
    return 0;
  }

  err = GetOneLevelKey(hkey, &kname, &subkey);
  *hkeyres = subkey;
  if (!err && kname) {
    err = rGetRegistryKey(subkey, kname, hkeyres);
    RegCloseKey(subkey);
  }
  return err;
}

registry68_key_t registry68_open(registry68_key_t hkey, const char *kname_cst)
{
  HKEY hdl = HKEY_INVALID;
  int len;
  char kname[1024], error[256];

  if (!kname_cst) {
    msg68_critical("registry68: open '%s::<null> null pointer\n",
                   keyhdlname(hkey));
    goto error;
  }

  if ( (len = strlen(kname_cst)) >= sizeof(kname) ) {
    msg68_critical("registry68: key name too long '%s::%s'\n",
                   keyhdlname(hkey), kname_cst);
    goto error;
  }
  memcpy(kname, kname_cst, len+1);

  if (rGetRegistryKey((HKEY)hkey, kname, (HKEY *)&hdl)) {
    SetSystemError(error,sizeof(error)-1);
    hdl = HKEY_INVALID;
  }

error:
  msg68_trace("registry68: open '%s::%s' => [%s]\n",
              keyhdlname(hdl), kname_cst, strok68(hdl==HKEY_INVALID));

  return (registry68_key_t) hdl;
}

/* Get a string value from register path
 */
int registry68_gets(registry68_key_t rootkey,
                    const char * kname_cst, char *kdata, int kdatasz)
{
  DWORD vtype;
  HKEY hkey;
  int len;
  LONG hres;
  char * name, kname[1024], error[256];
  const char * kname_save = kname_cst;

  if (!kname_cst|| !kdata || kdatasz <= 0) {
    msg68_critical("registry68: gets -- invalid parameters\n");
    goto error_out;
  }
  kdata[0]=0;
  len = strlen(kname_cst);

  /* Extract rootkey from path. */
  if (len>=4 && (kname_cst[2]=='K'||kname_cst[2]=='k') && kname_cst[2]==':') {
    int i;

    kname_cst += 4; len -= 4;
    for (i=0; i<REGISTRY68_LST
           && keys[i].name[0]!=kname_cst[0]
           && keys[i].name[1]!=kname_cst[1]; ++i)
      ;
    rootkey = keys[i].hkey;
  }

  if (rootkey == HKEY_INVALID) {
    msg68_critical("registry68: gets '%s' -- invalid rootkey\n", kname_save);
    kdata = 0;
    goto error_out;
  }

  if (len >= sizeof(kname)) {
    msg68_critical("registry68: gets '%s' -- key-path too long\n", kname_save);
    kdata = 0;
    goto error_out;
  }
  memcpy(kname, kname_cst, len+1);
  name = strrchr(kname,'/');
  if (name) {
    *name++ = 0;
  }

  if (rGetRegistryKey((HKEY)rootkey, kname, &hkey)) {
    SetSystemError(error, sizeof(error)-1);
  } else {
    hres =
      RegQueryValueEx(hkey,             /* handle to key to query            */
                      name,             /* address of name of value to query */
                      NULL,             /* reserved                          */
                      &vtype,           /* address of buffer for value type  */
                      (LPBYTE)kdata,    /* address of data buffer            */
                      (LPDWORD)&kdatasz /* address of data buffer size       */
        );
    if (hres != ERROR_SUCCESS) {
      SetSystemError(error, sizeof(error)-1);
      kdata = 0;
    } else if (vtype != REG_SZ) {
      msg68_error("registry68: gets '%s' -- not zero terminated string\n",
                  kname_save);
      kdata = 0;
    }
    RegCloseKey(hkey);
  }
  if (name) {
    *--name = '/';
  }
error_out:

  /* msg68_debug("registry68: gets '%s' => [%s,'%s']\n", */
  /*             strnevernull68(kname_save), */
  /*             strok68(!kdata), */
  /*             strnevernull68(kdata)); */
  return -!kdata;
}

#else

int registry68_support(void) { return 0; }

registry68_key_t registry68_rootkey(enum registry68_key_e rootkey)
{
  return (registry68_key_t)0;
}

registry68_key_t registry68_open(registry68_key_t hkey, const char *kname)
{
  return (registry68_key_t)0;
}

int registry68_gets(registry68_key_t rootkey, const char * kname_cst,
                    char *kdata, int kdatasz)
{
  return -1;
}

#endif /* #ifdef USE_REGISTRY68 */
