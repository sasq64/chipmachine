/*
 * @file    api68.c
 * @brief   sc68 API
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2011 Benjamin Gerard
 *
 * Time-stamp: <2011-11-15 16:12:38 ben>
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

#include <sc68/istream68.h> /* Need istream68.h before sc68.h */

#include "sc68.h"

#include "mixer68.h"
#include "conf68.h"
#include "emu68/emu68.h"
#include "emu68/excep68.h"
#include "emu68/ioplug68.h"
#include "io68/io68.h"

/* file68 includes */
#include <sc68/error68.h>
#include <sc68/string68.h>
#include <sc68/alloc68.h>
#include <sc68/file68.h>
#include <sc68/url68.h>
#include <sc68/rsc68.h>
#include <sc68/msg68.h>
#include <sc68/option68.h>
#include <sc68/audio68.h>

/* stardard includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PLAY_MAX_INST 1000000u  /* # of instructions for music play code */
#define INIT_MAX_INST 10000000u /* # of instructions for music init code */
#define TIME_DEF    (3 * 60)          /* default music time in seconds  */
#define TIMEMS_DEF  (TIME_DEF * 1000) /* default music time in millisec */

/* TOS emulator */
#define TRAP_ADDR 0x600
static u8 trap_func[] = {
#include "sc68/trap68.h"
};

/** Error messages stack. */
struct _sc68_estack_s {
  char str[4][256];             /**< Error string stack.  */
  int  cnt;                     /**< Error counter.       */
};
typedef struct _sc68_estack_s sc68_estack_t;

/** sc68 instance. */
struct _sc68_s {
  char           name[32];    /**< short name.                           */
  int            version;     /**< sc68 version.                         */
  void         * cookie;      /**< User private data.                    */

  /** 68k emulator. */
  emu68_t      * emu68;       /**< 68k emulator instance.                */
  emu68_parms_t  emu68_parms; /**< 68k emulator parameters.              */

  /** All IO chips.
   *  @notice Do not change order; debug68 haxxx depends on it).
   */
  io68_t *ymio,*mwio,*shifterio,*paulaio,*mfpio;

  ym_t         * ym;          /**< YM emulator.                          */
  mw_t         * mw;          /**< MicroWire emulator.                   */
  paula_t      * paula;       /**< Amiga emulator.                       */

  int            tobe3;       /**< free disk memory be release on close. */
  disk68_t     * disk;        /**< Current loaded disk.                  */
  music68_t    * mus;         /**< Current playing music.                */
  int            track;       /**< Current playing track.                */
  int            track_to;    /**< Track to set (0:n/a, -1:end req).     */
  int            loop_to;     /**< Loop to set (-1:default 0:infinite).  */
  /* int            force_loop;  /\**< Loop to set if default (-1:music).    *\/ */
  int            track_here;  /**< Force first track here.               */
  unsigned int   playaddr;    /**< Current play address in 68 memory.    */
  int            seek_to;     /**< Seek to this time (-1:n/a)            */

  /* $$$ MUST REMOVE CONFIG FROM INSTANCE ... */

  config68_t   * config;      /**< Config.                               */

  int            remote;      /**< Allow remote access.                  */


  /** Playing time info. */
  struct {
    unsigned int def_ms;      /**< default time in ms.                   */
    unsigned int length_ms;   /**< current track length in ms.           */
    unsigned int elapsed_ms;  /**< number of elapsed ms.                 */
    unsigned int total;       /**< total sec so far.                     */
    unsigned int total_ms;    /**< total ms correction.                  */
  } time;

  /** IRQ handler. */
  struct {
    int pc;                   /**< value of PC at last IRQ.              */
    int vector;               /**< what was the last IRQ type.           */
  } irq;

  /** Mixer info struture. */
  struct
  {
    unsigned int   rate;         /**< Sampling rate in hz.               */
    u32          * buffer;       /**< Current PCM buffer.                */
    u32          * bufptr;       /**< Current PCM position.              */
    int            bufmax;       /**< buffer allocated size.             */
    int            bufreq;       /**< Required buffer size for track.    */
    int            buflen;       /**< PCM count in buffer.               */
    int            stdlen;       /**< Default number of PCM per pass.    */
    unsigned int   cycleperpass; /**< Number of 68K cycles per pass.     */
    int            aga_blend;    /**< Amiga LR blend factor [0..65536].  */

    unsigned int   pass_count;   /**< Pass counter.                      */
    unsigned int   loop_count;   /**< Loop counter.                      */
    unsigned int   pass_total;   /**< Total number of pass.              */
    unsigned int   loop_total;   /**< Total number of loop (0:infinite). */
    unsigned int   pass_2loop;   /**< Number of pass before next loop.   */

    /* int            stp;          /\**< pitch frq (fixed int 8).           *\/ */
    /* int            max_stp;      /\**< max pitch frq (0:no seek).         *\/ */
  } mix;

  sc68_music_info_t info;        /**< Disk and track info struct.        */
  sc68_estack_t     estack;      /**< Error messages stack storage.      */

};

#ifndef DEBUG_SC68_O
# ifndef DEBUG
#  define DEBUG_SC68_O 0
# else
#  define DEBUG_SC68_O 1
# endif
#endif

static int           sc68_cat = msg68_NEVER;
static int           sc68_id;        /* counter for auto generated name. */
static volatile int  sc68_init_flag; /* Library init flag     */
static sc68_estack_t sc68_estack;    /* Library error message */
static int           sc68_sampling_rate_def = SAMPLING_RATE_DEF;
extern option68_t  * config68_options; /* conf68.c */
extern int config68_option_count;      /* conf68.c */
static int dbg68k;
static const char not_available[] = SC68_NOFILENAME;

static inline const char * ok_int(const int const err) {
  return !err ? "success" : "failure";
}

static const char * sc68_name_not_null(sc68_t * sc68)
{
  return sc68 ? sc68->name : "(nil)";
}

#ifndef HAVE_STPCPY
static char *stpcpy(char *dst, const char *src) {
  char c;
  while (c = *src++, c)
    *dst++ = c;
  *dst = 0;
  return dst;
}
#endif

static int stream_read_68k(sc68_t * sc68, unsigned int dest,
                           istream68_t * is, unsigned int sz)
{
  u8 * mem68 = emu68_memptr(sc68->emu68, dest, sz);

  if (!mem68) {
    return sc68_error_add(sc68, "libsc68: stream error -- %s",
                          emu68_error_get(sc68->emu68));
  }
  return (istream68_read(is, mem68, sz) == sz) ? 0 : -1;
}

static int init_emu68(sc68_t * const sc68, int * argc, char ** argv)
{
  int err;

  /* Initialize emu68 */
  sc68_debug(sc68,"libsc68: initialise 68k emulator <%p,%s>\n",
             sc68,sc68_name_not_null(sc68));
  err = emu68_init(argc, argv);
  if (err) {
    sc68_error_add(sc68, "libsc68: failed to initialise 68k emulator");
    goto error;
  }

  /* Initialize chipset */
  sc68_debug(sc68,"libsc68: initialise chipsets\n");
  err = io68_init(argc, argv);
  if (err) {
    sc68_error_add(sc68, "libsc68: failed to chipsets");
  }

error:
  return err;
}

static void safe_io68_destroy(io68_t **pio)
{
  io68_destroy(*pio);
  *pio = 0;
}

static void safe_emu68_destroy(emu68_t **pemu)
{
  emu68_destroy(*pemu);
  *pemu = 0;
}

static void safe_destroy(sc68_t * sc68)
{
  sc68_debug(sc68,"libsc68: safe destroy <%p,%s>\n",
             sc68,sc68_name_not_null(sc68));
  sc68_debug(sc68,"libsc68: - unplug all\n");
  emu68_ioplug_unplug_all(sc68->emu68);
  sc68_debug(sc68,"libsc68: - destroy io\n");
  safe_io68_destroy(&sc68->ymio);
  safe_io68_destroy(&sc68->mwio);
  safe_io68_destroy(&sc68->shifterio);
  safe_io68_destroy(&sc68->paulaio);
  safe_io68_destroy(&sc68->mfpio);
  sc68_debug(sc68,"libsc68: - destroy 68k\n");
  safe_emu68_destroy(&sc68->emu68);
}

#define Bpeek(EMU68,ADDR) emu68_peek((EMU68), (ADDR))

static u16 Wpeek(emu68_t* const emu68, addr68_t  addr)
{
  return (u16) ( ( (u16) Bpeek(emu68, addr+0) << 8 ) |
                 ( (u16) Bpeek(emu68, addr+1)      ) );
}

static u32 Lpeek(emu68_t* const emu68, addr68_t  addr)
{
  return (u32) ( ( (u32) Bpeek(emu68, addr+0) << 24 ) |
                 ( (u32) Bpeek(emu68, addr+1) << 16 ) |
                 ( (u32) Bpeek(emu68, addr+2) <<  8 ) |
                 ( (u32) Bpeek(emu68, addr+3)       ) );
}


static int irqhandler(emu68_t* const emu68, int vector, void * cookie)
{
  sc68_t     * sc68 = cookie;
  const char * irqname;
  u32 sr, pc;

  sc68->irq.pc     = emu68->reg.pc;
  sc68->irq.vector = vector;

  if ( vector == HWTRACE_VECTOR )
    return 0;

  irqname = emu68_exception_name(vector);
  if (!irqname) irqname = "?";
  if ( vector < 256 ) {
    sr = Wpeek(emu68, emu68->reg.a[7]);
    pc = Lpeek(emu68, emu68->reg.a[7]+2);
    sc68->irq.pc = pc;
  } else {
    sr = pc = 0;
  }
  if (vector <= 48)
  sc68_debug(sc68,
             "libsc68: 68k interruption -- emu68<%p,%s> sc68<%p,%s>\n"
             "         vector : %02x\n"
             "         type   : %s\n"
             "         pc:%08x sr:%04x\n"
             "         d0:%08x d1:%08x d2:%08x d3:%08x\n"
             "         d4:%08x d5:%08x d6:%08x d7:%08x\n"
             "         a0:%08x a1:%08x a2:%08x a3:%08x\n"
             "         a4:%08x a5:%08x a6:%08x a7:%08x\n",
             emu68, emu68->name, sc68, sc68->name,
             vector, irqname, pc, sr,
             emu68->reg.d[0], emu68->reg.d[1],
             emu68->reg.d[2], emu68->reg.d[3],
             emu68->reg.d[4], emu68->reg.d[5],
             emu68->reg.d[6], emu68->reg.d[7],
             emu68->reg.a[0], emu68->reg.a[1],
             emu68->reg.a[2], emu68->reg.a[3],
             emu68->reg.a[4], emu68->reg.a[5],
             emu68->reg.a[6], emu68->reg.a[7]);

  if ( vector >= 32 && vector < 48 ) {
    int cmd = Wpeek(emu68, emu68->reg.a[7]+6);
    sc68_debug(sc68,
               "         trap #%x func:%02d ($%04X)\n",
               vector-32, cmd, cmd);
  }
  if (vector < 32)
    return 1;                   /* Request execution break  */
  return 0;                     /* Continue execution normally */
}

static int init68k(sc68_t * sc68, int log2mem, int emu68_debug)
{
  int err = -1;
  emu68_parms_t * const parms = &sc68->emu68_parms;

  if (sc68->emu68) {
    sc68_debug(sc68,"libsc68: init 68k -- found previous emu68\n");
    safe_destroy(sc68);
  }

  /* setup parameters. */
  parms->name    = "sc68/emu68";
  parms->log2mem = log2mem;
  parms->clock   = EMU68_ATARIST_CLOCK;
  parms->debug   = emu68_debug;

  sc68_debug(sc68,
             "libsc68: init 68k -- '%s' mem:%d-bit(%dkB) clock:%uhz debug:%s\n",
             parms->name,
             parms->log2mem,parms->log2mem>10?1<<(parms->log2mem-10):0,
             parms->clock,parms->debug?"On":"Off");

  /* Do initialization. */
  sc68->emu68 = emu68_create(parms);
  if (!sc68->emu68) {
    sc68_error_add(sc68,"libsc68: create 68k emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- CPU emulator created\n");

  /* Install cookie and interruption handler (debug mode only). */
  emu68_set_handler(sc68->emu68, emu68_debug ? irqhandler : 0);
  emu68_set_cookie(sc68->emu68, sc68);
  sc68->irq.pc = sc68->irq.vector = -1;

  /* Setup critical 68K registers (SR and SP) */
  sc68->emu68->reg.sr   = 0x2000;
  sc68->emu68->reg.a[7] = sc68->emu68->memmsk+1-4;

  /* Initialize chipset */

  sc68->ymio = ymio_create(sc68->emu68,0);
  sc68->ym   = ymio_emulator(sc68->ymio);
  if (!sc68->ymio) {
    sc68_error_add(sc68,"libsc68: create YM emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- chipset -- YM-2149\n");

  sc68->mwio = mwio_create(sc68->emu68,0);
  sc68->mw   = mwio_emulator(sc68->mwio);
  if (!sc68->mwio) {
    sc68_error_add(sc68,"libsc68: create MW emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- chipset -- MicroWire\n");

  sc68->shifterio = shifterio_create(sc68->emu68,0);
  if (!sc68->shifterio) {
    sc68_error_add(sc68,"libsc68: create Shifter emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- chipset -- ST shifter\n");

  sc68->paulaio = paulaio_create(sc68->emu68,0);
  sc68->paula   = paulaio_emulator(sc68->paulaio);
  if (!sc68->paulaio) {
    sc68_error_add(sc68,"libsc68: create Paula emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- chipset -- Amiga Paula\n");

  sc68->mfpio = mfpio_create(sc68->emu68);
  if (!sc68->mfpio) {
    sc68_error_add(sc68,"libsc68: create MFP emulator failed");
    goto error;
  }
  sc68_debug(sc68,"libsc68: init 68k -- chipset -- MFP\n");

  err = 0;
 error:
  if (err) {
    safe_destroy(sc68);
  }

  sc68_debug(sc68,"libsc68: init 68k -- %s\n", strok68(err));

  return err;
}

/* Get integer config vaue.
 * - just command line if available
 * - else get the value from the config file
 * - fallback to default otherwise.
 */
static int myconfig_get_int(config68_t * c,
                            const char * name,
                            int def)
{
  option68_t    * opt;
  int             v = def, idx = -1;

  switch (option68_type(opt = option68_get(name, 1))) {
  case option68_BOL: case option68_INT:
    v = opt->val.num;
    sc68_debug(0,"libsc68: get config from cli -- name='%s' val=%d\n", name, v);
  default:
    if ( config68_get(c, &idx, &name) == CONFIG68_INT ) {
      v = idx;
      sc68_debug(0,"libsc68: get config from cfg -- name='%s' val=%d\n", name, v);
    }
  }
  return v;
}

/* Get string config value
 * @myconfig_get_int for details,
 */
static const char * myconfig_get_str(config68_t * c,
                                     const char * name,
                                     const char * def)
{
  option68_t    * opt;
  const char    * v = def, *key = name;
  int             idx = -1;

  switch (option68_type(opt = option68_get(name, 1))) {
  case option68_STR:
    v = opt->val.str;
    sc68_debug(0,"libsc68: get config from cli -- name='%s' val='%s'\n", name, v);
  default:
    if ( config68_get(c, &idx, &key) == CONFIG68_STR ) {
      v = key;
      sc68_debug(0,"libsc68: get config from cfg -- name='%s' val=%d\n", name, v);
    }
  }
  return v;
}


static void set_config(sc68_t * sc68)
{
  config68_t * c = sc68->config;
  const char * lmusic, * rmusic;

  sc68->version       = myconfig_get_int(c, "version",       PACKAGE_VERNUM);
  sc68->remote        = myconfig_get_int(c, "allow-remote",  1);
  sc68->mix.aga_blend = myconfig_get_int(c, "amiga-blend",   0x4000);
  sc68->track_here    = myconfig_get_int(c, "force-track",   0);
  sc68->time.def_ms   = myconfig_get_int(c, "default-time",  TIME_DEF) * 1000;
  sc68->mix.rate      = myconfig_get_int(c, "sampling-rate", SAMPLING_RATE_DEF);
  /* sc68->force_loop    = myconfig_get_int(c, "force-loop",    -1); */
  /* sc68->mix.max_stp   = myconfig_get_int(c, "seek-speed",    0xF00); */
  sc68->time.total    = myconfig_get_int(c, "total-time",    0);
  sc68->time.total_ms = myconfig_get_int(c, "total-ms",      0);

  if (sc68->time.def_ms < 1000) {
    sc68->time.def_ms = TIMEMS_DEF;
  }

  rsc68_get_path(0,0,&lmusic, &rmusic);
  if (!lmusic) {
    lmusic = myconfig_get_str(c, "music_path", 0);
    rsc68_set_music(lmusic);
  }
  if (!rmusic) {
    rmusic = myconfig_get_str(c, "remote_music_path", 0);
    rsc68_set_remote_music(rmusic);
  }

}

static void myconfig_set_int(config68_t * c,
                             const char * name,
                             int v)
{
  if (c) {
    config68_set(c, -1, name, v, 0);
  }
}

static void get_config(sc68_t * sc68)
{
  config68_t * c = sc68->config;

  myconfig_set_int(c, "version",       PACKAGE_VERNUM);
  /* Do not change user config ! */
  /*   myconfig_set_int(c, "aga_blend",   sc68->mix.aga_blend); */
  /*   myconfig_set_int(c, "force_track",   sc68->track_here); */
  /*   myconfig_set_int(c, "default_time",  (sc68->time.def_ms+999)/1000u); */
  /*   myconfig_set_int(c, "sampling_rate", sc68->mix.rate); */
  /*   myconfig_set_int(c, "force_loop",    sc68->force_loop); */
  /*   myconfig_set_int(c, "seek_speed",    sc68->mix.max_stp); */
  /*   myconfig_set_int(c, "allow_remote",  sc68->remote); */
  myconfig_set_int(c, "total_time",    sc68->time.total);
  myconfig_set_int(c, "total_ms",      sc68->time.total_ms);
}

int sc68_config_load(sc68_t * sc68)
{
  int err = -1;

  if (sc68) {
    if (!sc68->config) {
      sc68->config = config68_create(0);
    }
    err = config68_load(sc68->config);
    set_config(sc68);
  }
  sc68_debug(sc68,"libsc68: load config -- %s\n",strok68(err));
  return err;
}

int sc68_config_save(sc68_t * sc68)
{
  int err = -1;
  if (sc68) {
    get_config(sc68);
    err = config68_save(sc68->config);
  }
  sc68_debug(sc68,"libsc68: save config -- %s\n",strok68(err));
  return err;
}

int sc68_config_idx(sc68_t * sc68, const char * name)
{
  return config68_get_idx(sc68 ? sc68->config : 0, name);
}

config68_type_t sc68_config_range(sc68_t * sc68, int idx,
                                  int * min, int * max, int * def)
{
  return config68_range(sc68 ? sc68->config : 0, idx, min, max, def);
}

config68_type_t sc68_config_get(sc68_t * sc68,
                                int * idx, const char ** name)
{
  return sc68
    ? config68_get(sc68->config, idx, name)
    : CONFIG68_ERR;
}

config68_type_t sc68_config_set(sc68_t * sc68,
                                int idx,
                                const char * name,
                                int v,
                                const char * s)
{
  return sc68
    ? config68_set(sc68->config,idx,name,v,s)
    : CONFIG68_ERR;
}

void sc68_config_apply(sc68_t * sc68)
{
  if (sc68) {
    config68_valid(sc68->config);
    set_config(sc68);
  }
}


int sc68_init(sc68_init_t * init)
{
  int err = -1;
  option68_t * opt;

  static option68_t emuopt = {
    option68_BOL, "sc68-","dbg68k","debug","force 68k to run in debug mode"
  };

  sc68_init_t dummy_init;

  if (sc68_init_flag) {
    err = sc68_error_add(0, "libsc68: already initialized");
    goto error_no_shutdown;
  }

  if (!init) {
    memset(&dummy_init,0,sizeof(dummy_init));
    init = &dummy_init;
  }

  sc68_cat = msg68_cat("sc68","sc68 library",DEBUG_SC68_O);

  /* 1st thing to do : set debug handler. */
  msg68_set_handler((msg68_t)init->msg_handler);
  msg68_set_cookie(0);

  msg68_cat_filter(init->debug_clr_mask,init->debug_set_mask);

  /* Init config module */
  config68_init();

  /* Intialize file68. */
  init->argc = file68_init(init->argc, init->argv);

  option68_append(&emuopt,1);
  option68_append(config68_options,config68_option_count);
  init->argc = option68_parse(init->argc, init->argv, 0);

  /* Initialize emulators. */
  err = init_emu68(0, &init->argc, init->argv);

  /* Set default sampling rate. */
  if (!err) {
    err = sc68_sampling_rate(0, SC68_SPR_DEFAULT);
    if (err > 0) {
      sc68_sampling_rate_def = err;
      err = 0;
    }
  }

  /* $$$: No good */
  opt = option68_get("debug", 1);
  if (opt) {
    int val = strtol(opt->val.str,0,0);
    msg68_cat_filter(~0,val);
  }

  opt    = option68_get("dbg68k", 1);
  dbg68k = opt ? opt->val.num : 0;

  sc68_init_flag = !err;

  if (err) {
    sc68_shutdown();
  }
 error_no_shutdown:
  sc68_debug(0,"libsc68: initialized -- %s\n", ok_int(err));
  return -!!err;
}

void sc68_shutdown(void)
{
  sc68_debug(0,"libsc68: shutdowning\n");
  if (sc68_init_flag) {
    sc68_init_flag = 0;
    file68_shutdown();
    config68_shutdown();          /* always after file68_shutdown() */
  }
  sc68_debug(0,"libsc68: shutdowned -- %s\n",ok_int(0));
}

sc68_t * sc68_create(sc68_create_t * create)
{
  const int log2mem = 0;
  sc68_t *sc68 = 0;
  sc68_create_t dummy_create;

  if (!create) {
    memset(&dummy_create,0,sizeof(dummy_create));
    create = &dummy_create;
  }
  sc68_debug(0,"libsc68: creating new instance\n");

  /* Alloc SC68 struct. */
  sc68 = calloc68(sizeof(sc68_t));
  if (!sc68) {
    goto error;
  }

  /* User private data. */
  sc68->cookie = create->cookie;

  /* Pick a short name */
  if (create->name) {
    strncpy(sc68->name, create->name, sizeof(sc68->name));
  } else {
    snprintf(sc68->name,sizeof(sc68->name),"sc68#%02d",++sc68_id);
  }
  sc68->name[sizeof(sc68->name)-1] = 0;

  /* Load config file */
  sc68_config_load(sc68);

  /* Override config. */
  if (create->sampling_rate) {
    sc68->mix.rate = create->sampling_rate;
  }
  if (!sc68->mix.rate) {
    sc68->mix.rate = sc68_sampling_rate_def;
  }
  if (!sc68->time.def_ms) {
    sc68->time.def_ms = TIMEMS_DEF;
  }

  /* Create 68k emulator and pals. */
  if (init68k(sc68, log2mem, create->emu68_debug || dbg68k)) {
    goto error;
  }

  /* Set IO chipsets sampling rates */
  sc68->mix.rate = sc68_sampling_rate(sc68, sc68->mix.rate);
  if (sc68->mix.rate <= 0) {
    sc68_error_add(sc68,"invalid sampling rate -- *%dhz*\n", sc68->mix.rate);
    goto error;
  }

  create->sampling_rate = sc68->mix.rate;
  sc68_debug(sc68,"sampling rate -- *%dhz*\n", create->sampling_rate);

  /* Finally gets all pathes. */
  /*   rsc68_get_path(&init->shared_path, */
  /*               &init->user_path, */
  /*               &init->lmusic_path, */
  /*               &init->rmusic_path); */

  /*   debugmsg68(-1,"sc68_create: shared-path=[%s]\n",init->shared_path); */
  /*   debugmsg68(-1,"sc68_create: user-path=[%s]\n",init->user_path); */
  /*   debugmsg68(-1,"sc68_create: music-path=[%s]\n",init->lmusic_path); */
  /*   debugmsg68(-1,"sc68_create: music-rpath=[%s]\n",init->rmusic_path); */

  /*   sc68_unic = sc68; */

  sc68_debug(0,"create *%s* -- %s\n", sc68->name, ok_int(0));
  return sc68;

 error:
  sc68_destroy(sc68);
  sc68 = 0;
  sc68_debug(sc68,"libsc68: create -- %s\n", ok_int(-1));
  return 0;
}

void sc68_destroy(sc68_t * sc68)
{
  sc68_debug(sc68,"libsc68: destroy sc68<%p,%s>\n", sc68, sc68_name_not_null(sc68));
  if (sc68) {
    sc68_free(sc68->mix.buffer);
    sc68_close(sc68);
    sc68_config_save(sc68);
    config68_destroy(sc68->config); sc68->config = 0;
    safe_destroy(sc68);
    sc68_free(sc68);
  }
  sc68_debug(sc68,"libsc68: sc68<%p> destroyed\n", sc68);
}

const char * sc68_name(sc68_t * sc68)
{
  return sc68 ? sc68->name : 0;
}

int sc68_sampling_rate(sc68_t * sc68, int hz)
{
  switch (hz) {
  case SC68_SPR_QUERY:
    hz = sc68 ? sc68->mix.rate : sc68_sampling_rate_def;
    break;
  default:
    if (sc68) {
      hz = ymio_sampling_rate(sc68->ymio,hz);
      sc68_debug(sc68,"sampling rate after ym -- *%dhz*\n",hz);
      hz = mwio_sampling_rate(sc68->mwio,hz);
      sc68_debug(sc68,"sampling rate after after microwire -- *%dhz*\n",hz);
      hz = paulaio_sampling_rate(sc68->paulaio,hz);
      sc68_debug(sc68,"sampling rate after after paula -- *%dhz*\n",hz);
      sc68->mix.rate = hz;
      audio68_sampling_rate(hz);
      msg68_notice("%s: sampling rate -- *%dhz*\n", sc68->name, hz);
    } else {
      const int min = SAMPLING_RATE_MIN;
      const int max = SAMPLING_RATE_MAX;
      const int def = SAMPLING_RATE_DEF;
      if (hz == SC68_SPR_DEFAULT)
        hz = def;
      if (hz <  min) hz = min;
      if (hz >  max) hz = max;
      /* Assuming interface audio accepts this value. */
      audio68_sampling_rate(hz);
      sc68_sampling_rate_def = hz;
      msg68_notice("libsc68: default sampling rate -- *%dhz*\n", hz);
    }
  }
  return hz;
}

void sc68_set_share(sc68_t * sc68, const char * path)
{
  rsc68_set_share(path);
}

void sc68_set_user(sc68_t * sc68, const char * path)
{
  rsc68_set_user(path);
}

static unsigned int calc_disk_time(sc68_t * sc68, disk68_t * d)
{
  return (sc68 && sc68->disk == d && d->nb_mus == 1 && d->mus == sc68->mus)
    ? sc68->time.length_ms
    : d->time_ms;
}

static inline int loop_total(int loop_to, int mus_loop)
{
  return
    (loop_to == -1) ? mus_loop : loop_to;
}

static unsigned int fr_to_ms(unsigned int frames, unsigned int hz)
{
  u64 ms;

  ms  = frames;
  ms *= 1000u;
  ms /= hz;

  return (unsigned int) ms;
}

/* static unsigned int ms_to_fr(unsigned int ms, unsigned int hz) */
/* { */
/*   u64 fr; */

/*   fr  = ms; */
/*   fr *= hz; */
/*   fr /= 1000u; */

/*   return (unsigned int ) fr; */
/* } */

static void stop_track(sc68_t * sc68)
{
  assert(sc68);
  assert(sc68->disk);

  if (sc68->mix.buflen) {
    msg68_warning("libsc68: discard data -- *%d pcm*\n", sc68->mix.buflen);
  }

  sc68->mus             = 0;
  sc68->track           = 0;
  sc68->seek_to         = -1;
  sc68->track_to        = 0;

  sc68->time.total     += sc68->time.elapsed_ms / 1000u;
  sc68->time.total_ms  += sc68->time.elapsed_ms % 1000u;
  sc68->time.total     += sc68->time.total_ms / 1000u;
  sc68->time.total_ms  %= 1000u;
  sc68->time.elapsed_ms = 0;
  sc68->time.length_ms  = 0;

  sc68->mix.pass_count  = 0;
  sc68->mix.loop_count  = 0;
  sc68->mix.bufptr      = 0;
  sc68->mix.buflen      = 0;
}

static int reset_emulators(sc68_t * sc68, const hwflags68_t * const hw)
{
  assert(sc68);
  assert(sc68->emu68);
  assert(hw);

  emu68_ioplug_unplug_all(sc68->emu68);
  emu68_mem_reset(sc68->emu68);

  if (hw->bit.amiga) {
    sc68_debug(sc68," -> Add Paula hardware\n");
    emu68_ioplug(sc68->emu68, sc68->paulaio);
    sc68_debug(sc68," -> Set PAULA as interruptible\n");
    emu68_set_interrupt_io(sc68->emu68, sc68->paulaio);
  }
  if (hw->bit.ym) {
    sc68_debug(sc68," -> Add SHIFTER hardware\n");
    emu68_ioplug(sc68->emu68, sc68->shifterio);
    sc68_debug(sc68," -> Add YM hardware\n");
    emu68_ioplug(sc68->emu68, sc68->ymio);
    sc68_debug(sc68," -> Add MFP hardware\n");
    emu68_ioplug(sc68->emu68, sc68->mfpio);
    sc68_debug(sc68," -> Set MFP as interruption\n");
    emu68_set_interrupt_io(sc68->emu68, sc68->mfpio);
  }
  if (hw->bit.ste) {
    sc68_debug(sc68," -> Add MW (STE) hardware\n");
    emu68_ioplug(sc68->emu68, sc68->mwio);
  }
  emu68_reset(sc68->emu68);
  emu68_memset(sc68->emu68,0,0,0); /* Really ? Quiet an heavy operation */


  /* Init exceptions */
  if (1) {
    u8 * memptr;
    memptr        = emu68_memptr(sc68->emu68,0,1<<15);
    memptr[0]     = 0x4e; /* RTE in case of uninitilize interrupt vectors */
    memptr[1]     = 0x73; /* RTE ... */
    memptr[0x41a] = 0x00; /* Zound Dragger: should be fixed in music */
    memptr[0x41b] = 0x10; /* Zound Dragger: ... */
  }

  /* Install TOS trap emulator unless AMIGA */
  if (!hw->bit.amiga) {
    int status;
    sc68_debug(sc68," -> Load TOS trap emulator @$%06x-$%06x\n",
               TRAP_ADDR,TRAP_ADDR+sizeof(trap_func)-1);
    emu68_memput(sc68->emu68,TRAP_ADDR,trap_func, sizeof(trap_func));
    sc68->emu68->reg.a[7] = sc68->emu68->memmsk+1-16;
    sc68->emu68->reg.pc   = TRAP_ADDR;
    sc68->emu68->reg.sr   = 0x2300;
    sc68->emu68->cycle    = 0;
    sc68_debug(sc68," -> Running trap init code -- $%06x ...\n", TRAP_ADDR);
    status = emu68_finish(sc68->emu68, INIT_MAX_INST);
    if ( status != EMU68_NRM ) {
      sc68_debug(sc68,
                 " -> trap init -- ERROR -- status %d (%s)"
                 " [@%06x %02x]\n",
                 status, emu68_status_name(status), sc68->irq.pc, sc68->irq.vector);
      sc68_error_add(sc68, "libsc68: abnormal 68K status %d (%s) in trap code",
                     status, emu68_status_name(status));
      return SC68_ERROR;
    }
  }
  return SC68_OK;
}

static int load_replay(sc68_t * sc68, const char * replay, int a0)
{
  int err, size;
  istream68_t * is;
  char rname[256];
  assert(sc68);
  assert(replay);

  sc68_debug(sc68, " -> external replay -- %s\n", replay);
  strcpy(rname,"RSC68://replay/");    /* $$$ to be change a sc68://replay/... */
  strcat68(rname, replay, sizeof(rname)-1);
  rname[sizeof(rname)-1] = 0;
  is = url68_stream_create(rname, 1);
  err = istream68_open(is);
  err = err || (size = istream68_length(is), size < 0);
  err = err || stream_read_68k(sc68, a0, is, size);
  istream68_destroy(is);
  if (err) {
    sc68_error_add(sc68,
                   "libsc68: failed to load external replay -- %s",
                   rname);
    return SC68_ERROR;
  }
  sc68_debug(sc68," -> external replay -- [%06x-%06x]\n", a0, a0+size-1);
  return a0 + ((size + 1) & ~1);
}

static int run_music_init(sc68_t * sc68, music68_t * m, int a0, int a6)
{
  int status;

  assert(sc68);
  assert(m);

  /* Set 68K registers. */
  sc68->emu68->reg.d[0] = m->d0;
  sc68->emu68->reg.d[1] = !m->hwflags.bit.ste;
  sc68->emu68->reg.d[2] = m->datasz;
  sc68->emu68->reg.a[0] = a0;
  sc68->emu68->reg.a[6] = a6;           /* original replay address  */
  if (m->has.asid_trk)
    sc68->emu68->reg.d[7] = 0
      | ( ('A'+m->has.asid_tA) << 24 )
      | ( ('A'+m->has.asid_tB) << 16 )
      | ( ('A'+m->has.asid_tC) <<  8 )
      | ( ('A'+m->has.asid_tX) <<  0 );
    
  sc68->emu68->reg.a[7] = sc68->emu68->memmsk+1-16;
  sc68->emu68->reg.pc   = sc68->playaddr;
  sc68->emu68->reg.sr   = 0x2300;
  sc68->emu68->cycle    = 0;

  /* Run music init code. */
  sc68_debug(sc68," -> Running music init code ...\n");
  emu68_pushl(sc68->emu68, sc68->playaddr+8);
  status = emu68_finish(sc68->emu68, INIT_MAX_INST);
  if ( status != EMU68_NRM ) {
    sc68_debug(sc68,
               " -> music init -- ERROR -- status %d (%s)"
               " [@%06x %02x]\n",
               status, emu68_status_name(status), sc68->irq.pc, sc68->irq.vector);
    sc68_error_add(sc68, "libsc68: abnormal 68K status %d (%s) in init code",
                   status, emu68_status_name(status));
    return SC68_ERROR;
  }
  return SC68_OK;
}

static int change_track(sc68_t * sc68, int track)
{
  disk68_t  * d;
  music68_t * m;
  int         a0,a6;

  assert(sc68);
  assert(sc68->disk);
  assert(track > 0 && track <= sc68->disk->nb_mus);

  stop_track(sc68);

  d = sc68->disk;
  m = d->mus + track - 1;

  /* ReInit 68K & IO */
  reset_emulators(sc68, &m->hwflags);

  /* Set music replay address in 68K memory. */
  sc68->playaddr = a0 = m->a0;
  sc68_debug(sc68," -> play address -- $%06x\n", sc68->playaddr);

  if (m->has.asid_trk) {
    if (a0 = load_replay(sc68, "asidifier", a0), a0 == SC68_ERROR)
      return SC68_ERROR;
  }
  a6 = a0;

  /* Load external replay into 68K memory. */
  if (m->replay && (a0 = load_replay(sc68, m->replay, a0), a0 == SC68_ERROR))
    return SC68_ERROR;

  /* Copy music data into 68K memory */
  if (emu68_memput(sc68->emu68, a0, (u8 *)m->data, m->datasz)) {
    sc68_error_add(sc68,"libsc68: %s",emu68_error_get(sc68->emu68));
    return SC68_ERROR;
  }
  sc68_debug(sc68," -> music data -- [%06x-%06x]\n", a0, a0+m->datasz-1);

  /* Run music init code */
  if ( run_music_init(sc68, m, a0, a6) == SC68_ERROR )
    return SC68_ERROR;

  /* Ensure sampling rate */
  if (sc68->mix.rate <= 0)
    sc68->mix.rate = sc68_sampling_rate_def;
  if (sc68->mix.rate < SAMPLING_RATE_MIN ||
      sc68->mix.rate > SAMPLING_RATE_MAX) {
    sc68_error_add(sc68,"libsc68: invalid sampling rate -- %dhz\n", sc68->mix.rate);
    return SC68_ERROR;
  }

  assert(m->first_fr && m->total_fr);

  sc68->mix.loop_count = 0;
  sc68->mix.loop_total = loop_total(sc68->loop_to, m->loops);
  sc68->mix.pass_2loop = m->loops_fr ? m->first_fr : 0;
  sc68->mix.pass_count = 0;
  sc68->mix.pass_total = sc68->mix.loop_total
    ? m->first_fr + (sc68->mix.loop_total-1) * m->loops_fr
    : 0
    ;
  sc68->time.length_ms = fr_to_ms(sc68->mix.pass_total, m->frq);

  sc68_debug(sc68," -> loops           : %u\n", sc68->mix.loop_total);
  sc68_debug(sc68," -> first pass ms   : %u\n", m->first_ms);
  sc68_debug(sc68," -> loops pass ms   : %u\n", m->loops_ms);
  sc68_debug(sc68," -> total ms        : %u\n", sc68->time.length_ms);

  sc68_debug(sc68," -> first frames    : %u\n", m->first_fr);
  sc68_debug(sc68," -> loops frames    : %u\n", m->loops_fr);
  sc68_debug(sc68," -> total frames    : %u\n", sc68->mix.pass_total);


  sc68_debug(sc68," -> replay rate     : %u\n", m->frq);
  sc68->mix.cycleperpass =
    ( m->frq % 50u == 0 && sc68->emu68->clock == EMU68_ATARIST_CLOCK)
    ? 160256u * 50u / m->frq    /* exact value for genuine Atari ST */
    : (sc68->emu68->clock / m->frq)
    ;
  sc68_debug(sc68," -> cycle (exact)   : %u\n", sc68->mix.cycleperpass);

  if (m->hwflags.bit.ym) {
    cycle68_t cycles;
    /* Make sure ym-cycle are mutiple of 32 as required by current
     * emulator.
     *
     * $$$ I am not sure this works in all cases of frequency but it
     * is should be ok as far as cpu and ym frequency are multiple.
     *
     * $$$ In fact I am now pretty sure it does not work if
     * frequencies are not multiple.
     */
    cycles = ymio_cycle_cpu2ym(sc68->ymio,sc68->mix.cycleperpass);
    cycles = (cycles+31) & ~31;
    sc68->mix.cycleperpass = ymio_cycle_ym2cpu(sc68->ymio,cycles);

    /* verify */
    cycles = ymio_cycle_cpu2ym(sc68->ymio,sc68->mix.cycleperpass);
    sc68_debug(sc68," -> ym cycles       : %u [%s]\n",
               cycles, strok68(cycles&31));
  }
  sc68->mix.cycleperpass = (sc68->mix.cycleperpass+31) & ~31;
  sc68_debug(sc68," -> cycle (round)   : %u\n", sc68->mix.cycleperpass);

  if (m->frq == 60 && sc68->shifterio) {
    sc68_debug(sc68," -> Force shifter to 60Hz\n");
    shifterio_reset(sc68->shifterio,60);
 }

  /* Compute size of buffer needed for cycleperpass length at current rate. */
  if (1) {
    u64 len;
    len  = sc68->mix.rate;
    len *= sc68->mix.cycleperpass;
    len /= sc68->emu68->clock;
    sc68->mix.stdlen = (int) len;
    sc68_debug(sc68," -> std buffer len  : %u\n", sc68->mix.stdlen);
  }

  /* Compute *REAL* required size (in PCM) for buffer and realloc */
  if (1) {
    sc68->mix.bufreq = m->hwflags.bit.ym
      ? ymio_buffersize(sc68->ymio, sc68->mix.cycleperpass)
      : sc68->mix.stdlen
      ;
    sc68_debug(sc68," -> mix buffer len  : %u\n", sc68->mix.bufreq);

    /* Should not happen. Anyway it does not hurt. */
    if (m->hwflags.bit.amiga && sc68->mix.stdlen > sc68->mix.bufreq)
      sc68->mix.bufreq = sc68->mix.stdlen;

    sc68_debug(sc68," -> required PCM buffer size -- *%u pcm*\n",
               sc68->mix.bufreq);
    sc68_debug(sc68," ->  current PCM buffer size -- *%u pcm*\n",
               sc68->mix.bufmax);

    if (sc68->mix.bufreq > sc68->mix.bufmax) {
      sc68_free(sc68->mix.buffer);
      sc68->mix.bufmax = 0;
      sc68_debug(sc68," -> Alloc new PCM buffer -- *%u pcm*\n",
                 sc68->mix.bufreq);
      sc68->mix.buffer = sc68_alloc(sc68->mix.bufreq << 2);
      if (!sc68->mix.buffer) {
        sc68_error_add(sc68,"libsc68: failed to allocate new sample buffer");
        return SC68_ERROR;
      }
      sc68->mix.bufmax = sc68->mix.bufreq;
    }
  }
  sc68_debug(sc68," -> buffer length -- %u pcm\n", sc68->mix.bufreq);

  /* Setup internal info struct */
  if (sc68_music_info(sc68, &sc68->info, track, 0))
    return SC68_ERROR;
  sc68->mus           = m;
  sc68->track         = track;

  sc68_debug(sc68, "libsc68: track ready -- %s %s - %s - %s\n",
             sc68->info.trk.time,
             sc68->info.artist,
             sc68->info.album,
             sc68->info.title);

  return SC68_OK;
}


/** Start current music of current disk.
 */
static int apply_change_track(sc68_t * sc68)
{
  int track;

  assert(sc68);
  assert(sc68->disk);

  if (track = sc68->track_to, !track)
    return SC68_OK;
  sc68->track_to = 0;

  /* -1 : stop */
  if (track == -1) {
    sc68_debug(sc68,"libsc68: stop requested\n");
    sc68->mix.buflen = 0; /* warning removal in stop_track() */
    stop_track(sc68);
    return SC68_END;
  }

  sc68_debug(sc68,"libsc68: change track requested -- *%02d*\n", track);
  if (track < 1 || track > sc68->disk->nb_mus) {
    sc68_error_add(sc68, "libsc68: track out of range -- *%02d*", track);
    return SC68_ERROR;
  }

  if (change_track(sc68, track) != SC68_OK)
    return SC68_ERROR;

  return SC68_CHANGE;
}

static unsigned int calc_current_ms(sc68_t * sc68)
{
  u64 ms;
  ms  = sc68->mix.pass_count;
  ms *= sc68->mix.cycleperpass * 1000u;
  ms /= sc68->emu68->clock;
  return sc68->time.elapsed_ms = (unsigned int) ms;
}

int sc68_process(sc68_t * sc68, void * buf16st, int * _n)
{
  int ret;

  if (!sc68) {
    ret = SC68_ERROR;
  } else if (!_n) {
    /* Flush internal PCM buffer and apply change track request. */
    ret = SC68_IDLE | apply_change_track(sc68);
  } else if (!buf16st) {
    ret = SC68_ERROR;
  } else {
    int n = *_n;
    ret = (n < 0) ? SC68_ERROR : SC68_IDLE;

    while (n > 0) {
      int len;

      /* No more pcm in internal buffer ... */
      if (!sc68->mix.buflen) {
        int status;

        /* Checking for loop */
        if (sc68->mix.pass_2loop && !--sc68->mix.pass_2loop) {
          sc68->mix.pass_2loop = sc68->mus->loops_fr;
          sc68->mix.loop_count++;
          ret |= SC68_LOOP;
        }

        /* Checking for end */
        if (sc68->mix.pass_total && sc68->mix.pass_count >= sc68->mix.pass_total) {
          int next_track;
          next_track     = sc68->track+1;
          sc68->track_to = (next_track > sc68->disk->nb_mus) ? -1 : next_track;
          sc68->seek_to  = -1;
        }

        ret |= apply_change_track(sc68);
        if (ret & (SC68_END|SC68_CHANGE)) /* exit on error|end|change */
          break;
        ret &= ~SC68_IDLE;              /* No more idle */

        /* Run 68K emulator */
        emu68_pushl(sc68->emu68, sc68->playaddr+8);
        status = emu68_finish(sc68->emu68, PLAY_MAX_INST);
        if (status == EMU68_NRM)
          status = emu68_interrupt(sc68->emu68, sc68->mix.cycleperpass);
        if (status != EMU68_NRM) {
          sc68_error_add(sc68, "libsc68: abnormal 68K status %d (%s) in play pass %u",
                         status, emu68_status_name(status), sc68->mix.pass_count);
          ret = SC68_ERROR;
          break;
        }

        /* Reset pcm pointer. */
        sc68->mix.bufptr = sc68->mix.buffer;
        sc68->mix.buflen = sc68->mix.bufreq;

        /* Fill pcm buufer depending on architecture */
        if (sc68->mus->hwflags.bit.amiga) {
          /* Amiga - Paula */
          paula_mix(sc68->paula,(s32*)sc68->mix.bufptr,sc68->mix.buflen);
          mixer68_blend_LR(sc68->mix.bufptr, sc68->mix.bufptr, sc68->mix.buflen,
                           sc68->mix.aga_blend, 0, 0);
        } else {
          if (sc68->mus->hwflags.bit.ym) {
            int err =
              ymio_run(sc68->ymio, (s32*)sc68->mix.bufptr,
                       sc68->mix.cycleperpass);
            if (err < 0) {
              ret = SC68_ERROR;
              sc68->mix.buflen = 0;
              break;
            }
            sc68->mix.buflen = err;
          } else {
            mixer68_fill(sc68->mix.bufptr, sc68->mix.buflen = sc68->mix.bufreq, 0);
          }

          if (sc68->mus->hwflags.bit.ste)
            /* STE / MicroWire */
            mw_mix(sc68->mw, (s32 *)sc68->mix.bufptr, sc68->mix.buflen);
          else
            /* Else simply process with left channel duplication. */
            mixer68_dup_L_to_R(sc68->mix.bufptr, sc68->mix.bufptr,
                               sc68->mix.buflen, 0);
        }

        /* Advance time */
        calc_current_ms(sc68);
        sc68->mix.pass_count++;
      }

      assert(sc68->mix.buflen > 0);

      /* Copy to destination buffer. */
      len = sc68->mix.buflen <= n ? sc68->mix.buflen : n;
      mixer68_copy((u32 *)buf16st,sc68->mix.bufptr,len);
      buf16st = (u32 *)buf16st + len;
      sc68->mix.bufptr += len;
      sc68->mix.buflen -= len;
      n                -= len;
    }

    *_n -= n;
  }
  return ret;
}

int sc68_verify(istream68_t * is)
{
  return file68_verify(is);
}

int sc68_verify_url(const char * url)
{
  return file68_verify_url(url);
}

int sc68_verify_mem(const void * buffer, int len)
{
  return file68_verify_mem(buffer,len);
}

int sc68_is_our_url(const char * url, const char *exts, int * is_remote)
{
  return file68_is_our_url(url,exts,is_remote);
}

static int load_disk(sc68_t * sc68, disk68_t * d, int free_on_close)
{
  int track;
  if (!sc68 || !d) {
    goto error;
  }

  if (sc68->disk) {
    sc68_error_add(sc68,"libsc68: %s","disk already loaded");
    goto error;
  }

  sc68->tobe3 = !!free_on_close;
  sc68->disk  = d;
  sc68->track = 0;
  sc68->mus   = 0;

  track = sc68->track_here;
  if (track > d->nb_mus) {
    track = d->def_mus;
  }

  return sc68_play(sc68, track, -1);

 error:
  sc68_free(d);
  return -1;
}

int sc68_load(sc68_t * sc68, istream68_t * is)
{
  return load_disk(sc68, file68_load(is), 1);
}

int sc68_load_url(sc68_t * sc68, const char * url)
{
  return load_disk(sc68, file68_load_url(url), 1);
}

int sc68_load_mem(sc68_t * sc68, const void * buffer, int len)
{
  return load_disk(sc68, file68_load_mem(buffer, len), 1);
}


sc68_disk_t sc68_load_disk(istream68_t * is)
{
  return (sc68_disk_t) file68_load(is);
}

sc68_disk_t sc68_load_disk_url(const char * url)
{
  return (sc68_disk_t) file68_load_url(url);
}

sc68_disk_t sc68_disk_load_mem(const void * buffer, int len)
{
  return (sc68_disk_t) file68_load_mem(buffer, len);
}

void sc68_disk_free(sc68_disk_t disk)
{
  sc68_free(disk);
}

int sc68_open(sc68_t * sc68, sc68_disk_t disk)
{
  if (!disk) {
    sc68_close(sc68);
    return -1; /* Not an error but notifiy no disk has been loaded */
  }
  if (!sc68) {
    return -1;
  }
  return load_disk(sc68, disk, 0);
}

void sc68_close(sc68_t * sc68)
{
  if (sc68 && sc68->disk) {
    sc68->mix.buflen = 0; /* warning removal in stop_track() */
    stop_track(sc68);
    if (sc68->tobe3)
      sc68_free(sc68->disk);
    sc68->tobe3     = 0;
    sc68->disk      = 0;
  }
}

int sc68_tracks(sc68_t * sc68)
{
  return (sc68 && sc68->disk) ? sc68->disk->nb_mus : -1;
}

int sc68_play(sc68_t * sc68, int track, int loop)
{
  disk68_t * d;

  if (!sc68) {
    return -1;
  }
  d = sc68->disk;
  if (!d) {
    return -1;
  }

  /* track == -1 : read current track or current loop. */
  if (track == -1) {
    return loop
      ? sc68->mix.loop_count
      : sc68->track;
  }

  /* track == 0 : try force-track. */
  if (track == 0 && sc68->track_here) {
    track = sc68->track_here;
    if (track > d->def_mus) track = 0;
  }

  /* track == 0 : force-track out of range, restore disk default. */
  if (track == 0) {
    track = d->def_mus + 1;
  }

  /* Check track range. */
  if (track <= 0 || track > d->nb_mus) {
    return sc68_error_add(sc68,"libsc68: track #%d out of range [1..%d]",
                          track, d->nb_mus);
  }

  /* Set change track. Real track loading occurs during process thread to
     avoid multi-threading bug. */
  sc68->track_to = track;
  sc68->seek_to  = -1;
  sc68->loop_to  = loop;

  return 0;
}

int sc68_stop(sc68_t * sc68)
{
  if (!sc68 || !sc68->disk) {
    return -1;
  }
  sc68->track_to = -1;
  sc68->seek_to  = -1;
  return 0;
}

/** $$$ loop stuff is broken if loop differs some tracks */
int sc68_seek(sc68_t * sc68, int time_ms, int * is_seeking)
{
  disk68_t * d;

  if (!sc68 || (d=sc68->disk, !d)) {
    return -1;
  }

  if (time_ms == -1) {
    /* Read current */
    if (is_seeking) {
      *is_seeking = sc68->seek_to != -1;
    }
    if (!sc68->mus) {
      return -1;
    } else {
      int loop = loop_total(sc68->loop_to, sc68->mus->loops);
      return (sc68->mus->start_ms * loop) + sc68->time.elapsed_ms;
    }
  } else {
    return -1;

#if 0
    int i,n;
    unsigned int start_ms;
    unsigned int end_ms = 0;

    for (i=0, n=d->nb_mus ; i<n; ++i) {
      unsigned int ms = (unsigned int) time_ms;
      int loop =
        loop_total(sc68->loop_to, d->mus[i].loops);

      start_ms = end_ms;
      end_ms   = start_ms
        + ((d->mus+i == sc68->mus)
           ? sc68->time.length_ms :
           d->mus[i].time_ms) * loop;

      if (ms >= start_ms && ms < end_ms) {
        unsigned int cur_ms  = start_ms;
        sc68_debug(sc68,"Find track #%d [%u - %u]\n", i+1, start_ms, end_ms);
        if (i+1 == sc68->track) {
          /* Same track : return current time */
          cur_ms += sc68->time.elapsed_ms;
        } else {
          /* Change track : current time is start of new track */
          sc68->track_to = i+1;
        }

        if (ms > cur_ms) {
          /* real seek forward */
          if (is_seeking) *is_seeking = 1;
          sc68->seek_to = ms - start_ms;
          sc68_debug(sc68,"SEEK-TO %d, cur:%d\n", sc68->seek_to, cur_ms);
        } else {
          if (is_seeking) *is_seeking = 0;
          sc68->seek_to = -1;
          sc68_debug(sc68,"NO-SEEK-TO cur:%d\n", cur_ms);
        }
        return (int) cur_ms;
      }
    }
    sc68_debug(sc68,"-> Not in disk range !!! [%d>%d]\n",time_ms, d->time_ms);
    return -1;
#endif

  }
}

int sc68_music_info(sc68_t * sc68, sc68_music_info_t * info, int track, sc68_disk_t disk)
{
  disk68_t  * d;
  music68_t * m;

  static const char * hwtable[8] = {
    "none",
    "Yamaha-2149",
    "MicroWire (STE)",
    "Yamaha-2149 & MicroWire (STE)",
    "Amiga/Paula",
    "Yamaha-2149 & Amiga/Paula",
    "MicroWire (STE) & Amiga/Paula",
    "Yamaha-2149 & MicroWire (STE) & Amiga/Paula",
  };

  /* If disk is given use it, else use sc68 disk. */
  d = disk ? disk : (sc68 ? sc68->disk : 0);
  if (!d || !info)
    return -1;

  /* Asking for current or default track */
  if (track == 0 || track == -1)
    track = (disk || !sc68->track) ? d->def_mus+1 : sc68->track;

  if (track <= 0 || track > d->nb_mus)
    return -1;

  /* Asking for the track being played by sc68, simply copy internal
   * info struct, unless off course it is the one being filled.
   */
  if (sc68 && track == sc68->track && info != &sc68->info) {
    *info = sc68->info;
    return 0;
  }
  m = d->mus + track - 1;

  info->tracks      = d->nb_mus;
  info->start_ms    = m->start_ms;
  info->loop_ms     = m->loops_ms;
  info->replay      = m->replay ? m->replay : "built-in";
  info->rate        = m->frq;
  info->addr        = m->a0;

  info->dsk.track   = d->def_mus+1;
  info->dsk.time_ms = calc_disk_time(sc68,d);
  strtime68(info->dsk.time, info->tracks, (info->dsk.time_ms+999u)/1000u);
  info->dsk.ym      = d->hwflags.bit.ym;
  info->dsk.ste     = d->hwflags.bit.ste;
  info->dsk.amiga   = d->hwflags.bit.amiga;
  info->dsk.hw      = hwtable[info->dsk.ym+(info->dsk.ste<<1)+(info->dsk.amiga<<2)];
  info->dsk.tags    = file68_tag_count(d, 0);
  info->dsk.tag     = (sc68_tag_t *) d->tags.array;

  info->trk.track   = track;
  info->trk.time_ms = (sc68 && m == sc68->mus) ? sc68->time.length_ms : m->total_ms;
  strtime68(info->trk.time, track,(info->trk.time_ms+999u)/1000u);

  info->trk.ym     = m->hwflags.bit.ym;
  info->trk.ste    = m->hwflags.bit.ste;
  info->trk.amiga  = m->hwflags.bit.amiga;
  info->trk.hw      = hwtable[info->trk.ym+(info->trk.ste<<1)+(info->trk.amiga<<2)];

  info->trk.tags   = file68_tag_count(d, track);
  info->trk.tag    = (sc68_tag_t *) m->tags.array;

  info->album      = d->tags.tag.title.val;
  info->title      = m->tags.tag.title.val;
  info->artist     = m->tags.tag.artist.val;

  return 0;
}

#if 0
sc68_music_info_t * sc68_music_info(sc68_t * sc68, int track, sc68_disk_t disk)
{
  sc68_music_info_t * info = 0;
  int infosz;

  disk68_t * d;
  music68_t * m = 0;
  int hw;
  hwflags68_t hwf;
  int loop, force_loop, loop_to;

  char * tagdata;
  const char * key, * val;
  int tag_cnt, tag_len;

  static const char * hwtable[8] = {
    "none",
    "Yamaha-2149",
    "MicroWire (STE)",
    "Yamaha-2149 & MicroWire (STE)",
    "Amiga/Paula",
    "Yamaha-2149 & Amiga/Paula",
    "MicroWire (STE) & Amiga/Paula",
    "Yamaha-2149 & MicroWire (STE) & Amiga/Paula",
  };

  if (!sc68 && !disk)
    return 0;

  d = disk ? disk : sc68->disk;
  if (!d)
    return 0;

  /* -1 : use current track or default track (depending if disk was given) */
  if (track == -1)
    track = disk ? d->def_mus+1 : sc68->track;

  if (track <= 0 || track > d->nb_mus)
    return 0;

  /* count and measure tags */
  tag_cnt = tag_len = 0;
  for (tag_cnt = 0;
       !file68_tag_enum(d, track, tag_cnt, &key, &val);
       ++tag_cnt) {
    tag_len += strlen(key) + strlen(val) + 2;
  }

  /* alloc the info struct */
  infosz
    = sizeof(*info)
    - sizeof(info->tag)
    + sizeof(*info->tag) * tag_cnt
    + tag_len
    ;
  if (info = sc68_calloc(infosz), !info) {
    return 0;
  }
  info->tags = tag_cnt;

  /* copy tag data */
  tagdata = (char *)(info->tag+tag_cnt);
  for (tag_cnt = 0;
       !file68_tag_enum(d, track, tag_cnt, &key, &val);
       ++tag_cnt) {
    info->tag[tag_cnt].key = tagdata;
    tagdata = stpcpy(tagdata, key) + 1;
    info->tag[tag_cnt].val = tagdata;
    tagdata = stpcpy(tagdata, val) + 1;
  }

  info->title = (tag_cnt > TAG68_ID_TITLE)
    ? info->tag[TAG68_ID_TITLE].val
    : not_available
    ;

  info->author = (tag_cnt > TAG68_ID_ARTIST)
    ? info->tag[TAG68_ID_ARTIST].val
    : not_available
    ;

  m = d->mus + ((!track) ? d->def_mus : (track - 1));
  info->track  = track;
  info->tracks = d->nb_mus;

  force_loop   = sc68 ? sc68->force_loop : -1;
  loop_to      = sc68 ? sc68->loop_to    : -1;
  loop         = calc_loop_total(force_loop, loop_to, m->loop);

  if (!track) {
    /* disk info */
    info->replay   = 0;
    info->time_ms  = calc_disk_time(sc68,d);
    info->start_ms = 0;
    hwf.all        = d->hwflags.all;
    track          = info->tracks;
  } else {
    /* track info */
    info->replay   = m->replay ? m->replay : "built-in";
    info->time_ms  = (sc68 && m == sc68->mus)
      ? sc68->time.length_ms : (m->time_ms ? m->time_ms : TIMEMS_DEF);
    info->start_ms = m->start_ms;
    hwf.all        = m->hwflags.all;
  }

  loop = calc_loop_total(force_loop,
                         (sc68 && m==sc68->mus) ? loop_to : 1,
                         m->loop);
  info->time_ms  *= loop;
  info->start_ms *= loop;

  /* If there is a track remapping always use it. */
  if (m->track) {
    track = info->track = m->track;
  }

  /* info->author    = m->tags.tag.artist.val ? m->tags.tag.artist.val  : "n/a"; */
  /* info->composer  = /\* m->composer  ? m->composer  : *\/ "n/a"; */
  /* info->ripper    = /\* m->ripper    ? m->ripper    : *\/ "n/a"; */
  /* info->converter = /\* m->converter ? m->converter : *\/ "n/a"; */

  hw = 7 &
    (  (hwf.bit.ym      ? SC68_YM    : 0)
       | (hwf.bit.ste   ? SC68_STE   : 0)
       | (hwf.bit.amiga ? SC68_AMIGA : 0)
       )
    ;

  info->hw.ym    = hwf.bit.ym;
  info->hw.ste   = hwf.bit.ste;
  info->hw.amiga = hwf.bit.amiga;
  info->rate     = m->frq;
  info->addr     = m->a0;
  info->hwname   = hwtable[hw];
  strtime68(info->time, track, (info->time_ms+999u)/1000u);

  return info;
}
#endif

istream68_t * sc68_stream_create(const char * url, int mode)
{
  return url68_stream_create(url,mode);
}

void sc68_error_flush(sc68_t * sc68)
{
  (sc68 ? &sc68->estack : &sc68_estack)->cnt = 0;
}

const char * sc68_error_get(sc68_t * sc68)
{
  sc68_estack_t * const estack = sc68 ? &sc68->estack : &sc68_estack;
  const char * err = 0;
  if (estack->cnt > 0) {
    err = estack->str[--estack->cnt];
  } else {
    estack->cnt = 0;
  }
  return err;
}

int sc68_error_add(sc68_t * sc68, const char * fmt, ...)
{
  va_list list;
  sc68_estack_t * const estack = sc68 ? &sc68->estack : &sc68_estack;
  const int maxstr = sizeof(estack->str) / sizeof(estack->str[0]);
  const int maxlen = sizeof(estack->str) / maxstr;
  char tmp[256] = { 0 };

  va_start(list, fmt);
  error68_va(fmt,list);
  va_end(list);

  va_start(list, fmt);
  vsnprintf(tmp,sizeof(tmp),fmt,list);
  tmp[sizeof(tmp)-1] = 0;
  va_end(list);

  if (estack->cnt < 0) {
    estack->cnt = 0;
  }
  if (estack->cnt >= maxstr) {
    memmove(estack->str[0],estack->str[1],(maxstr-1)*maxlen);
    estack->cnt = maxstr-1;
  }
  strncpy(estack->str[estack->cnt], tmp, maxlen);
  estack->str[estack->cnt][maxlen-1] = 0;
  estack->cnt++;

  return -1;
}

void * sc68_alloc(unsigned int n)
{
  void * const ptr = alloc68(n);
  if (!ptr)
    sc68_error_add(0,"libsc68: memory allocation error -- %u bytes", n);
  return ptr;
}

void * sc68_calloc(unsigned int n)
{
  void * const ptr = calloc68(n);
  if (!ptr)
    sc68_error_add(0,"libsc68: memory allocation error -- %u bytes", n);
  return ptr;
}

void sc68_free(void * data)
{
  free68(data);
}

void sc68_debug(sc68_t * sc68, const char * fmt, ...)
{
  va_list list;
  va_start(list,fmt);
  msg68x_va(sc68_cat,sc68,fmt,list);
  va_end(list);
}

/* Hidden exported function to be used by some of my tool to haxxx
 * into emulators as they need.
 */
SC68_API
void sc68_emulators(sc68_t    * sc68,
                    emu68_t  ** p_emu68,
                    io68_t  *** p_ios68)
{
  if (sc68) {
    if (p_emu68) * p_emu68 =  sc68->emu68;
    if (p_ios68) * p_ios68 = &sc68->ymio;
  }
}

void ** sc68_cookie_ptr(sc68_t * sc68)
{
  return sc68
    ? &sc68->cookie
    : 0
    ;
}

const char * sc68_mimetype(void) {
  return SC68_MIMETYPE;
}