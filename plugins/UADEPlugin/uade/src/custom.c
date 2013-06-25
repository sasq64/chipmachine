 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * Copyright 1995-1998 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "uae.h"
#include "gensound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "readcpu.h"
#include "newcpu.h"
#include "cia.h"
#include "audio.h"
#include "osemu.h"

#include "uadectl.h"

static unsigned int n_consecutive_skipped = 0;
static unsigned int total_skipped = 0;

#define SPRITE_COLLISIONS

/* Mouse and joystick emulation */

static int buttonstate[3];
static int mouse_x, mouse_y;
int joy0button, joy1button;
unsigned int joy0dir, joy1dir;

/* Events */

unsigned long int cycles, nextevent, is_lastline;
static int rpt_did_reset;
struct ev eventtab[ev_max];

static int vpos;
static uae_u16 lof;
static int next_lineno;
static int lof_changed = 0;

static const int dskdelay = 2; /* FIXME: ??? */

static uae_u32 sprtaba[256],sprtabb[256];

/*
 * Hardware registers of all sorts.
 */

static void custom_wput_1 (int, uaecptr, uae_u32) REGPARAM;

static uae_u16 cregs[256];

uae_u16 intena,intreq;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */

static uae_u32 cop1lc,cop2lc,copcon;
 
int maxhpos = MAXHPOS_PAL;
int maxvpos = MAXVPOS_PAL;
int minfirstline = MINFIRSTLINE_PAL;
int vblank_endline = VBLANK_ENDLINE_PAL;
int vblank_hz = VBLANK_HZ_PAL;
static int fmode;
static unsigned int beamcon0, new_beamcon0;
static int ntscmode = 0;

#define MAX_SPRITES 32

/* This is but an educated guess. It seems to be correct, but this stuff
 * isn't documented well. */
enum sprstate { SPR_stop, SPR_restart, SPR_waiting_start, SPR_waiting_stop };
static enum sprstate sprst[8];
static int spron[8];
static uaecptr sprpt[8];
static int sprxpos[8], sprvstart[8], sprvstop[8];

static unsigned int sprdata[MAX_SPRITES], sprdatb[MAX_SPRITES], sprctl[MAX_SPRITES], sprpos[MAX_SPRITES];
static int sprarmed[MAX_SPRITES], sprite_last_drawn_at[MAX_SPRITES];
static int last_sprite_point, nr_armed;

static uae_u32 bpl1dat, bpl2dat, bpl3dat, bpl4dat, bpl5dat, bpl6dat, bpl7dat, bpl8dat;
static uae_s16 bpl1mod, bpl2mod;

static uaecptr bplpt[8];
#ifndef SMART_UPDATE
static char *real_bplpt[8];
#endif

static unsigned int bplcon0, bplcon1, bplcon2, bplcon3, bplcon4;
static int nr_planes_from_bplcon0, corrected_nr_planes_from_bplcon0;
static unsigned int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static unsigned int ddfstrt, ddfstop;

static uae_u32 dskpt;
static uae_u16 dsklen, dsksync;
static int dsklength;

/* The display and data fetch windows */

enum diw_states
{
    DIW_waiting_start, DIW_waiting_stop
};

static int plffirstline, plflastline, plfstrt, plfstop, plflinelen;
static int diwfirstword, diwlastword;
static enum diw_states diwstate, hdiwstate;

/* Sprite collisions */
static uae_u16 clxdat, clxcon;
static int clx_sprmask;

enum copper_states {
    COP_stop,
    COP_rdelay1, COP_read1, COP_read2,
    COP_bltwait,
    COP_wait1, COP_wait1b, COP_wait2,
};

struct copper {
    /* The current instruction words.  */
    unsigned int i1, i2;
    enum copper_states state;
    /* Instruction pointer.  */
    uaecptr ip;
    int hpos, vpos, count;
    unsigned int ignore_next;
    unsigned int do_move;
    enum diw_states vdiw;
};

static struct copper cop_state;

static void prepare_copper_1 (void);

int dskdmaen; /* used in cia.c */

/*
 * Statistics
 */

/* Used also by bebox.cpp */
static unsigned long int msecs = 0, lastframetime = 0;
unsigned long int frametime = 0, timeframes = 0;
static unsigned long int seconds_base;
int bogusframe;


static int current_change_set;

static struct sprite_draw *curr_sprite_positions, *prev_sprite_positions;
static struct color_change *curr_color_changes, *prev_color_changes;
static struct draw_info *curr_drawinfo, *prev_drawinfo;
static struct color_entry *curr_color_tables, *prev_color_tables;

static int next_color_change, next_sprite_draw, next_delay_change;
static int next_color_entry, remembered_color_entry;
static int color_src_match, color_dest_match, color_compare_result;

/* These few are only needed during/at the end of the scanline, and don't
 * have to be remembered. */
static int decided_bpl1mod, decided_bpl2mod, decided_nr_planes, decided_res;

static char thisline_changed;


#ifdef SMART_UPDATE
#define MARK_LINE_CHANGED do { thisline_changed = 1; } while (0)
#else
#define MARK_LINE_CHANGED do { ; } while (0)
#endif

static int modulos_added, plane_decided, color_decided, very_broken_program;

/*
 * helper functions
 */

int rpt_available = 0;

void reset_frame_rate_hack (void)
{
    if (currprefs.m68k_speed != -1)
	return;

    if (! rpt_available) {
	currprefs.m68k_speed = 0;
	return;
    }

    rpt_did_reset = 1;
    is_lastline = 0;
    write_log ("Resetting frame rate hack\n");
}

static inline void prepare_copper (void)
{
    if (cop_state.vpos > vpos
	|| cop_state.state == COP_stop)
    {
	eventtab[ev_copper].active = 0;
	return;
    }
    prepare_copper_1 ();
}

void check_prefs_changed_custom (void)
{
    currprefs.gfx_framerate = changed_prefs.gfx_framerate;
    /* Not really the right place... */
    if (currprefs.jport0 != changed_prefs.jport0
	|| currprefs.jport1 != changed_prefs.jport1) {
	currprefs.jport0 = changed_prefs.jport0;
	currprefs.jport1 = changed_prefs.jport1;
    }
    currprefs.immediate_blits = changed_prefs.immediate_blits;
    currprefs.blits_32bit_enabled = changed_prefs.blits_32bit_enabled;
	
}

static inline void setclr (uae_u16 *p, uae_u16 val)
{
    if (val & 0x8000)
	*p |= val & 0x7FFF;
    else
	*p &= ~val;
}

inline int current_hpos (void)
{
    return cycles - eventtab[ev_hsync].oldcycles;
}

static int broken_plane_sub[8];

/* set PAL or NTSC timing variables */

static void init_hz (void)
{
    int isntsc;

    beamcon0 = new_beamcon0;

    isntsc = beamcon0 & 0x20 ? 0 : 1;

    if (!isntsc) {
	maxvpos = MAXVPOS_PAL;
	maxhpos = MAXHPOS_PAL;
	minfirstline = MINFIRSTLINE_PAL;
	vblank_endline = VBLANK_ENDLINE_PAL;
	vblank_hz = VBLANK_HZ_PAL;
    } else {
	maxvpos = MAXVPOS_NTSC;
	maxhpos = MAXHPOS_NTSC;
	minfirstline = MINFIRSTLINE_NTSC;
	vblank_endline = VBLANK_ENDLINE_NTSC;
	vblank_hz = VBLANK_HZ_NTSC;
    }
    // write_log ("Using %s timing\n", isntsc ? "NTSC" : "PAL");
}

/* Mousehack stuff */

#define defstepx (1<<16)
#define defstepy (1<<16)
#define defxoffs 0
#define defyoffs 0

static const int docal = 60, xcaloff = 40, ycaloff = 20;
static const int calweight = 3;
static int lastsampledmx, lastsampledmy;
static int lastspr0x,lastspr0y,lastdiffx,lastdiffy,spr0pos,spr0ctl;
static int mstepx,mstepy,xoffs=defxoffs,yoffs=defyoffs;
static int sprvbfl;

static int lastmx, lastmy;
static int newmousecounters;
static int ievent_alive = 0;

static int timehack_alive = 0;

static uae_u32 timehack_helper (void)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    if (m68k_dreg (regs, 0) == 0)
	return timehack_alive;

    timehack_alive = 10;

    gettimeofday (&tv, NULL);
    put_long (m68k_areg (regs, 0), tv.tv_sec - (((365 * 8 + 2) * 24 - 2) * 60 * 60));
    put_long (m68k_areg (regs, 0) + 4, tv.tv_usec);
    return 0;
#else
    return 2;
#endif
}

 /*
  * register functions
  */
static inline uae_u16 DENISEID (void)
{
    if (currprefs.chipset_mask & CSMASK_AGA)
	return 0xF8;
    if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
	return 0xFC;
    return 0xFFFF;
}
static inline uae_u16 DMACONR (void)
{
    return (dmacon | 0 | 0x2000);
}
static inline uae_u16 INTENAR (void)
{
    return intena;
}
uae_u16 INTREQR (void)
{
    return intreq | (currprefs.use_serial ? 0x0001 : 0);
}
static inline uae_u16 ADKCONR (void)
{
    return adkcon;
}
static inline uae_u16 VPOSR (void)
{
    unsigned int csbit = ntscmode ? 0x1000 : 0;
    csbit |= (currprefs.chipset_mask & CSMASK_AGA) ? 0x2300 : 0;
    csbit |= (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0x2000 : 0;
    return (vpos >> 8) | lof | csbit;
}
static void VPOSW (uae_u16 v)
{
    if (lof != (v & 0x8000))
	lof_changed = 1;
    lof = v & 0x8000;
    /*
     * This register is much more fun on a real Amiga. You can program
     * refresh rates with it ;) But I won't emulate this...
     */
}

static inline uae_u16 VHPOSR (void)
{
    return (vpos << 8) | current_hpos();
}

static inline void COP1LCH (uae_u16 v) { cop1lc = (cop1lc & 0xffff) | ((uae_u32)v << 16); }
static inline void COP1LCL (uae_u16 v) { cop1lc = (cop1lc & ~0xffff) | (v & 0xfffe); }
static inline void COP2LCH (uae_u16 v) { cop2lc = (cop2lc & 0xffff) | ((uae_u32)v << 16); }
static inline void COP2LCL (uae_u16 v) { cop2lc = (cop2lc & ~0xffff) | (v & 0xfffe); }

static void COPJMP1 (uae_u16 a)
{
    cop_state.ip = cop1lc;
    cop_state.do_move = 0;
    cop_state.ignore_next = 0;
    cop_state.state = COP_read1;
    cop_state.vpos = vpos;
    cop_state.hpos = current_hpos () & ~1;
    cop_state.count = current_hpos () & ~1;
    prepare_copper ();
    if (eventtab[ev_copper].evtime == cycles && eventtab[ev_copper].active)
	abort ();
    events_schedule ();
}

static void COPJMP2 (uae_u16 a)
{
    cop_state.ip = cop2lc;
    cop_state.do_move = 0;
    cop_state.ignore_next = 0;
    cop_state.state = COP_read1;
    cop_state.vpos = vpos;
    cop_state.hpos = current_hpos () & ~1;
    cop_state.count = current_hpos () & ~1;
    prepare_copper ();
    if (eventtab[ev_copper].evtime == cycles && eventtab[ev_copper].active)
	abort ();
    events_schedule ();
}

static inline void COPCON (uae_u16 a)
{
    copcon = a;
}

static void DMACON (uae_u16 v)
{
    int i;
    uae_u16 oldcon = dmacon;

    setclr(&dmacon,v); dmacon &= 0x1FFF;
    /* ??? post_decide_line (); */

    /* FIXME? Maybe we need to think a bit more about the master DMA enable
     * bit in these cases. */
    if ((dmacon & DMA_COPPER) > (oldcon & DMA_COPPER)) {
	cop_state.ip = cop1lc;
	cop_state.do_move = 0;
	cop_state.ignore_next = 0;
	cop_state.state = COP_read1;
	cop_state.vpos = vpos;
	cop_state.hpos = current_hpos () & ~1;
	cop_state.count = current_hpos () & ~1;
	prepare_copper ();
	if (eventtab[ev_copper].evtime == cycles && eventtab[ev_copper].active)
	    abort ();
    }

    update_audio ();

    for (i = 0; i < 4; i++) {
	struct audio_channel_data *cdp = audio_channel + i;

	cdp->dmaen = (dmacon & 0x200) && (dmacon & (1<<i));
	if (cdp->dmaen) {
	    if (cdp->state == 0) {
		cdp->state = 1;
		cdp->pt = cdp->lc;
		cdp->ptend = cdp->lc + 2 * (cdp->len ? cdp->len : 65536);
		cdp->wper = cdp->per;
		cdp->wlen = cdp->len;
		cdp->data_written = 2;
		cdp->evtime = eventtab[ev_hsync].evtime - cycles;
	    }
	} else {
	    if (cdp->state == 1 || cdp->state == 5) {
		cdp->state = 0;
		cdp->current_sample = 0;
	    }
	}
    }

    events_schedule();
}

/*static int trace_intena = 0;*/

static inline void INTENA (uae_u16 v)
{
/*    if (trace_intena)
	fprintf (stderr, "INTENA: %04x\n", v);*/
    setclr(&intena,v); regs.spcflags |= SPCFLAG_INT;
}
void INTREQ (uae_u16 v)
{
    setclr(&intreq,v);
    regs.spcflags |= SPCFLAG_INT;
}

static void ADKCON (uae_u16 v)
{
    unsigned long t;

    update_audio ();

    setclr (&adkcon,v);
    t = adkcon | (adkcon >> 4);
    audio_channel[0].adk_mask = (((t >> 0) & 1) - 1);
    audio_channel[1].adk_mask = (((t >> 1) & 1) - 1);
    audio_channel[2].adk_mask = (((t >> 2) & 1) - 1);
    audio_channel[3].adk_mask = (((t >> 3) & 1) - 1);
}

static void BEAMCON0 (uae_u16 v)
{
  new_beamcon0 = v & 0x20;
}

static void BPLPTH (int hpos, uae_u16 v, int num)
{
}
static void BPLPTL (int hpos, uae_u16 v, int num)
{
}

static void BPLCON0 (int hpos, uae_u16 v)
{
}
static inline void BPLCON1 (int hpos, uae_u16 v)
{
}
static inline void BPLCON2 (int hpos, uae_u16 v)
{
}
static inline void BPLCON3 (int hpos, uae_u16 v)
{
}
static inline void BPLCON4 (int hpos, uae_u16 v)
{
}

static void BPL1MOD (int hpos, uae_u16 v)
{
}

static void BPL2MOD (int hpos, uae_u16 v)
{
}

/* We could do as well without those... */
static inline void BPL1DAT (uae_u16 v) {}
static inline void BPL2DAT (uae_u16 v) {}
static inline void BPL3DAT (uae_u16 v) {}
static inline void BPL4DAT (uae_u16 v) {}
static inline void BPL5DAT (uae_u16 v) {}
static inline void BPL6DAT (uae_u16 v) {}
static inline void BPL7DAT (uae_u16 v) {}
static inline void BPL8DAT (uae_u16 v) {}

static void DIWSTRT (int hpos, uae_u16 v)
{
}

static void DIWSTOP (int hpos, uae_u16 v)
{
}

static void DIWHIGH (int hpos, uae_u16 v)
{
}

static void DDFSTRT (int hpos, uae_u16 v)
{
}
static void DDFSTOP (int hpos, uae_u16 v)
{
}

static void FMODE (uae_u16 v)
{
}

static void BLTADAT (uae_u16 v)
{
}
/*
 * "Loading data shifts it immediately" says the HRM. Well, that may
 * be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
 * loaded for every word so that AFWM and ALWM can be applied.
 */
static void BLTBDAT (uae_u16 v)
{
}
static void BLTCDAT (uae_u16 v) {}

static void BLTAMOD (uae_u16 v) {}
static void BLTBMOD (uae_u16 v) {}
static void BLTCMOD (uae_u16 v) {}
static void BLTDMOD (uae_u16 v) {}

static void BLTCON0 (uae_u16 v) {}
/* The next category is "Most useless hardware register".
 * And the winner is... */
static void BLTCON0L (uae_u16 v) {}
static void BLTCON1 (uae_u16 v) {}

static void BLTAFWM (uae_u16 v) {}
static void BLTALWM (uae_u16 v) {}

static void BLTAPTH (uae_u16 v) {}
static void BLTAPTL (uae_u16 v) {}
static void BLTBPTH (uae_u16 v) {}
static void BLTBPTL (uae_u16 v) {}
static void BLTCPTH (uae_u16 v) {}
static void BLTCPTL (uae_u16 v) {}
static void BLTDPTH (uae_u16 v) {}
static void BLTDPTL (uae_u16 v) {}

static void BLTSIZE (uae_u16 v)
{
  fprintf(stderr,"blitter stroken in BLTSIZE (custom.c)...\n");
}

static void BLTSIZV (uae_u16 v)
{
  fprintf(stderr,"blitter stroken in BLTSIZE (custom.c)...\n");
}

static void BLTSIZH (uae_u16 v)
{
  fprintf(stderr,"blitter stroken in BLTSIZE (custom.c)...\n");
}

static inline void SPRxCTL_1 (uae_u16 v, int num)
{
}
static inline void SPRxPOS_1 (uae_u16 v, int num)
{
}
static inline void SPRxDATA_1 (uae_u16 v, int num)
{
}
static inline void SPRxDATB_1 (uae_u16 v, int num)
{
}
static void SPRxDATA (int hpos, uae_u16 v, int num) {}
static void SPRxDATB (int hpos, uae_u16 v, int num) {}
static void SPRxCTL (int hpos, uae_u16 v, int num) {}
static void SPRxPOS (int hpos, uae_u16 v, int num) {}
static void SPRxPTH (int hpos, uae_u16 v, int num)
{
}
static void SPRxPTL (int hpos, uae_u16 v, int num)
{
}
static void CLXCON (uae_u16 v)
{
}
static uae_u16 CLXDAT (void)
{
  return 0;
}
static void COLOR (int hpos, uae_u16 v, int num)
{
}

static void DSKSYNC (uae_u16 v) {}
static void DSKDAT (uae_u16 v) {}
static void DSKPTH (uae_u16 v) {}
static void DSKPTL (uae_u16 v) {}

static void DSKLEN (uae_u16 v)
{
  fprintf(stderr,"dsklen striken...\n");
}

static uae_u16 DSKBYTR (void)
{
  fprintf(stderr,"dksbytr striken...\n");
  return 0;
}

static uae_u16 DSKDATR (void)
{
  fprintf(stderr,"dskdatr striken...\n");
  return 0;
}

static uae_u16 potgo_value;

static void POTGO (uae_u16 v)
{
    potgo_value = v;
}

static uae_u16 POTGOR (void)
{
    uae_u16 v = (potgo_value | (potgo_value << 1)) & 0xAA00;
    v |= v >> 1;

    if (JSEM_ISMOUSE (0, &currprefs)) {
	if (buttonstate[2])
	    v &= 0xFBFF;

	if (buttonstate[1])
	    v &= 0xFEFF;
    } else if (JSEM_ISJOY0 (0, &currprefs) || JSEM_ISJOY1 (0, &currprefs)) {
	if (joy0button & 2) v &= 0xfbff;
	if (joy0button & 4) v &= 0xfeff;
    }

    if (JSEM_ISJOY0 (1, &currprefs) || JSEM_ISJOY1 (1, &currprefs)) {
	if (joy1button & 2) v &= 0xbfff;
	if (joy1button & 4) v &= 0xefff;
    }

    return v;
}

static uae_u16 POT0DAT (void)
{
    static uae_u16 cnt = 0;
    if (JSEM_ISMOUSE (0, &currprefs)) {
	if (buttonstate[2])
	    cnt = ((cnt + 1) & 0xFF) | (cnt & 0xFF00);
	if (buttonstate[1])
	    cnt += 0x100;
    }

    return cnt;
}
static uae_u16 JOY0DAT (void)
{
  return 0;
}
static uae_u16 JOY1DAT (void)
{
  return 0;
}
static void JOYTEST (uae_u16 v)
{
}

/*
 * Here starts the copper code. Can you believe it used to be worse?
 */

static inline void copper_adjust_diw (struct copper *cst)
{
    if (cst->vdiw == DIW_waiting_start && vpos == plffirstline)
	cst->vdiw = DIW_waiting_stop;
    if (cst->vdiw == DIW_waiting_stop && vpos == plflastline)
	cst->vdiw = DIW_waiting_start;
}

/* Determine which cycles are available for the copper in a display
 * with a agiven number of planes.  */
static int cycles_for_plane[9][8] = {
    { 0, -1, 0, -1, 0, -1, 0, -1 },
    { 0, -1, 0, -1, 0, -1, 0, -1 },
    { 0, -1, 0, -1, 0, -1, 0, -1 },
    { 0, -1, 0, -1, 0, -1, 0, -1 },
    { 0, -1, 0, -1, 0, -1, 0, -1 },
    { 0, -1, 1, -1, 0, -1, 0, -1 },
    { 0, -1, 1, -1, 0, -1, 1, -1 },
    { 1, -1, 1, -1, 1, -1, 1, -1 },
    { 1, -1, 1, -1, 1, -1, 1, -1 }
};

static unsigned int waitmasktab[256];

#define COP_OFFSET 4

static inline int copper_in_playfield (enum diw_states diw, int hpos)
{
    hpos -= COP_OFFSET;
    return diw == DIW_waiting_stop && hpos >= plfstrt && hpos < plfstrt + plflinelen;
}

static inline int copper_cant_read (enum diw_states diw, int hpos, int planes)
{
    int t;

    if (hpos >= ((maxhpos - 2) & ~1))
	return 1;

    if (currprefs.chipset_mask & CSMASK_AGA)
	/* FIXME */
	return 0;

    if (! copper_in_playfield (diw, hpos))
	return 0;

    t = cycles_for_plane[planes][hpos & 7];
#if 0
    if (t == -1)
	abort ();
#endif
    return t;
}

static void update_copper_1 (int until_hpos)
{
    unsigned int vp, hp, vcmp, hcmp;
    int c_hpos = cop_state.hpos;
    int c_vpos = cop_state.vpos;

    vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
    hp = cop_state.count & (cop_state.i2 & 0xFE);
    vcmp = ((cop_state.i1 & (cop_state.i2 | 0x8000)) >> 8);
    hcmp = (cop_state.i1 & cop_state.i2 & 0xFE);

    if (cop_state.state == COP_wait2)
	cop_state.state = COP_wait1b;

    for (;;) {
	if (c_hpos == (maxhpos & ~1)) {
	    c_hpos = 0;
	    cop_state.count = 0;
	    c_vpos++;
	}

	if (! dmaen (DMA_COPPER))
	    cop_state.state = COP_stop;

	if (c_vpos > vpos
	    || c_hpos > until_hpos
	    || cop_state.state == COP_stop)
	    break;

	if (c_hpos - COP_OFFSET == plfstrt)
	    copper_adjust_diw (&cop_state);

	switch (cop_state.state) {
	 case COP_rdelay1:
	    cop_state.state = COP_read1;
	    break;

	 case COP_read1:
	    if (copper_cant_read (cop_state.vdiw, c_hpos, corrected_nr_planes_from_bplcon0))
		break;

	    if (cop_state.do_move) {
		cop_state.do_move = 0;
		if (cop_state.i1 < (copcon & 2 ? ((currprefs.chipset_mask & CSMASK_AGA) ? 0 : 0x40u) : 0x80u)) {
		    cop_state.state = COP_stop;
		    break;
		}
		switch (cop_state.i1) {
		 case 0x088:
		    cop_state.ip = cop1lc;
		    cop_state.state = COP_rdelay1;
		    break;
		 case 0x08A:
		    cop_state.ip = cop2lc;
		    cop_state.state = COP_rdelay1;
		    break;
		 default:
		    custom_wput_1 (c_hpos, cop_state.i1, cop_state.i2);
		    break;
		}
		if (cop_state.state != COP_read1)
		    break;
	    }
	    cop_state.i1 = chipmem_bank.wget (cop_state.ip);
	    cop_state.ip += 2;
	    cop_state.state = COP_read2;
	    break;

	 case COP_read2:
	    if (copper_cant_read (cop_state.vdiw, c_hpos, corrected_nr_planes_from_bplcon0))
		break;
	    cop_state.i2 = chipmem_bank.wget (cop_state.ip);
	    cop_state.ip += 2;
	    if (cop_state.ignore_next) {
		cop_state.ignore_next = 0;
		cop_state.state = COP_read1;
		break;
	    }
	    if ((cop_state.i1 & 1) == 0) {
		cop_state.state = COP_read1;
		cop_state.do_move = 1;
	    } else {
		vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
		hp = cop_state.count & (cop_state.i2 & 0xFE);
		vcmp = ((cop_state.i1 & (cop_state.i2 | 0x8000)) >> 8);
		hcmp = (cop_state.i1 & cop_state.i2 & 0xFE);

		if ((cop_state.i2 & 1) == 0) {
		    cop_state.state = COP_wait1;
		    if (cop_state.i1 == 0xFFFF && cop_state.i2 == 0xFFFE)
			cop_state.state = COP_stop;
		} else {
		    /* Skip instruction.  */
		    if ((vp > vcmp || (vp == vcmp && hp >= hcmp))
			&& ((cop_state.i2 & 0x8000) != 0 || ! (DMACONR() & 0x4000)))
			cop_state.ignore_next = 1;
		    cop_state.state = COP_read1;
		}
	    }
	    break;

	 case COP_wait1:
	    if (copper_cant_read (cop_state.vdiw, c_hpos, corrected_nr_planes_from_bplcon0))
		break;

	    /* fall through */

	 case COP_wait1b:
	    {
		int do_wait2 = cop_state.state == COP_wait1b;
		cop_state.state = COP_wait2;
		if (vp == vcmp && corrected_nr_planes_from_bplcon0 < 8) {
		    int t = cop_state.count + 2;
		    int next_count = (t & waitmasktab[cop_state.i2 & 0xfe]) | hcmp;
		    int nexthpos;
		    if (next_count < t)
			next_count = t;
		    nexthpos = c_hpos + next_count - cop_state.count;
		    if (nexthpos < (maxhpos & ~1)) {
			if (c_hpos - COP_OFFSET < plfstrt && nexthpos - COP_OFFSET >= plfstrt)
			    copper_adjust_diw (&cop_state);
			c_hpos = nexthpos;
			cop_state.count = next_count;
			do_wait2 = 1;
		    }
		}
		if (! do_wait2)
		    break;
	    }

	    /* fall through */

	 case COP_wait2:
	    if (vp < vcmp) {
		if (c_hpos - COP_OFFSET < plfstrt)
		    copper_adjust_diw (&cop_state);

		c_vpos++;
		c_hpos = 0;
		cop_state.count = 0;
		continue;
	    }
	    hp = cop_state.count & (cop_state.i2 & 0xFE);
	    if (vp == vcmp && hp < hcmp)
		break;
	    /* Now we know that the comparisons were successful.  */
	    if ((cop_state.i2 & 0x8000) == 0x8000 || ! (DMACONR() & 0x4000))
		cop_state.state = COP_read1;
	    else {
		/* We need to wait for the blitter.  It won't stop while
		 * we're in update_copper, so we _could_ as well proceed to
		 * until_hpos in one big step.  There are some tricky
		 * issues to be considered, though, so use the slow method
		 * for now.  */
		cop_state.state = COP_bltwait;
	    }
	    break;

	 default:
	    /* Delay cycles.  */
	    break;	    
	}

	c_hpos += 2;
	if (corrected_nr_planes_from_bplcon0 < 8 || ! copper_in_playfield (cop_state.vdiw, c_hpos))
	    cop_state.count += 2;
    }
    cop_state.hpos = c_hpos;
    cop_state.vpos = c_vpos;
}

static inline void update_copper (int until_hpos)
{
    if (cop_state.vpos > vpos
	|| cop_state.hpos > until_hpos
	|| cop_state.state == COP_stop)
	return;
    update_copper_1 (until_hpos);
}

static int dangerous_reg (int reg)
{
    /* Safe:
     * Bitplane pointers, control registers, modulos and data.
     * Sprite pointers, control registers, and data.
     * Color registers.  */
    if (reg >= 0xE0 && reg < 0x1C0)
	return 0;
    return 1;
}

static void prepare_copper_1 (void)
{
    struct copper cst = cop_state;
    unsigned int vp, hp, vcmp, hcmp;

    vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
    hp = cop_state.count & (cop_state.i2 & 0xFE);
    vcmp = ((cop_state.i1 & (cop_state.i2 | 0x8000)) >> 8);
    hcmp = (cop_state.i1 & cop_state.i2 & 0xFE);

    if (cst.state == COP_wait2)
	cst.state = COP_wait1b;

    for (;;) {
	if (cst.hpos == (maxhpos & ~1)) {
	    cst.hpos = 0;
	    cst.count = 0;
	    cst.vpos++;
	}

	if (! dmaen (DMA_COPPER))
	    cst.state = COP_stop;

	if (cst.vpos > vpos
	    || cst.state == COP_stop)
	{
	    eventtab[ev_copper].active = 0;
	    return;
	}

	if (cst.hpos - COP_OFFSET == plfstrt)
	    copper_adjust_diw (&cst);

	switch (cst.state) {
	 case COP_rdelay1:
	    cst.state = COP_read1;
	    break;

	 case COP_read1:
	    if (copper_cant_read (cst.vdiw, cst.hpos, corrected_nr_planes_from_bplcon0))
		break;

	    if (cst.do_move) {
		cst.do_move = 0;
		if (cst.i1 < (copcon & 2 ? ((currprefs.chipset_mask & CSMASK_AGA) ? 0 : 0x40u) : 0x80u)) {
		    cst.state = COP_stop;
		    eventtab[ev_copper].active = 0;
		    return;
		} else if (dangerous_reg (cst.i1)) {
		    eventtab[ev_copper].active = 1;
		    eventtab[ev_copper].oldcycles = cycles;
		    eventtab[ev_copper].evtime = cycles + cst.hpos - current_hpos () + (cst.vpos - cop_state.vpos) * maxhpos;
		    return;
		}
	    }

	    cst.i1 = chipmem_bank.wget (cst.ip);
	    cst.ip += 2;
	    cst.state = COP_read2;
	    break;

	 case COP_read2:
	    if (copper_cant_read (cst.vdiw, cst.hpos, corrected_nr_planes_from_bplcon0))
		break;
	    cst.i2 = chipmem_bank.wget (cst.ip);
	    cst.ip += 2;
	    if (cst.ignore_next) {
		cst.ignore_next = 0;
		cst.state = COP_read1;
		break;
	    }
	    if ((cst.i1 & 1) == 0) {
		cst.state = COP_read1;
		cst.do_move = 1;
	    } else {
		vp = vpos & (((cst.i2 >> 8) & 0x7F) | 0x80);
		hp = cst.count & (cst.i2 & 0xFE);
		vcmp = ((cst.i1 & (cst.i2 | 0x8000)) >> 8);
		hcmp = (cst.i1 & cst.i2 & 0xFE);

		if ((cst.i2 & 1) == 0) {
		    cst.state = COP_wait1;
		    if (cst.i1 == 0xFFFF && cst.i2 == 0xFFFE)
			cst.state = COP_stop;
		} else {
		    /* Skip instruction.  */
		    if ((vp > vcmp || (vp == vcmp && hp >= hcmp))
			&& ((cst.i2 & 0x8000) != 0 || ! (DMACONR() & 0x4000)))
			cst.ignore_next = 1;
		    cst.state = COP_read1;
		}
	    }
	    break;

	 case COP_wait1:
	    if (copper_cant_read (cst.vdiw, cst.hpos, corrected_nr_planes_from_bplcon0))
		break;

	    /* fall through */

	 case COP_wait1b:
	    {
		int do_wait2 = cst.state == COP_wait1b;
		cst.state = COP_wait2;
		if (vp == vcmp && corrected_nr_planes_from_bplcon0 < 8) {
		    int t = cst.count + 2;
		    int next_count = (t & waitmasktab[cst.i2 & 0xfe]) | hcmp;
		    int nexthpos;
		    if (next_count < t)
			next_count = t;
		    nexthpos = cst.hpos + next_count - cst.count;
		    if (nexthpos < (maxhpos & ~1)) {
			if (cst.hpos - COP_OFFSET < plfstrt && nexthpos - COP_OFFSET >= plfstrt)
			    copper_adjust_diw (&cst);
			cst.hpos = nexthpos;
			cst.count = next_count;
			do_wait2 = 1;
		    }
		}
		if (! do_wait2)
		    break;
	    }

	    /* fall through */
	    
	 case COP_wait2:
	    if (vp < vcmp) {
		if (cst.hpos - COP_OFFSET < plfstrt)
		    copper_adjust_diw (&cst);

		cst.vpos++;
		cst.count = 0;
		cst.hpos = 0;
		continue;
	    }
	    hp = cst.count & (cst.i2 & 0xFE);
	    if (vp == vcmp && hp < hcmp)
		break;
#if 0
	    if (nexthpos != -1)
		if (nexthpos != cst.count)
		    fprintf (stderr,"ERROR\n");
	    nexthpos = -1;
#endif
	    /* Now we know that the comparisons were successful.  */
	    if ((cst.i2 & 0x8000) == 0x8000 || ! (DMACONR() & 0x4000))
		cst.state = COP_read1;
	    else {
		/* We need to wait for the blitter.  It won't stop while
		 * we're in update_copper, so we _could_ as well proceed to
		 * until_hpos in one big step.  There are some tricky
		 * issues to be considered, though, so use the slow method
		 * for now.  */
		cst.state = COP_bltwait;
	    }
	    break;

	 default:
	    /* Delay cycles.  */
	    break;	    
	}

	cst.hpos += 2;
	if (corrected_nr_planes_from_bplcon0 < 8 || ! copper_in_playfield (cst.vdiw, cst.hpos))
	    cst.count += 2;
    }
}

static void do_copper (void)
{
  //    return;
    
  int hpos = current_hpos ();
  update_copper (hpos);
  prepare_copper ();
  if (eventtab[ev_copper].evtime == cycles && eventtab[ev_copper].active)
    abort ();
}

static void adjust_array_sizes (void)
{
#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
    if (delta_sprite_draw) {
	void *p1,*p2;
	int mcc = max_sprite_draw + 200 + delta_sprite_draw;
	delta_sprite_draw = 0;
	p1 = realloc (sprite_positions[0], mcc * sizeof (struct sprite_draw));
	p2 = realloc (sprite_positions[1], mcc * sizeof (struct sprite_draw));
	if (p1) sprite_positions[0] = p1;
	if (p2) sprite_positions[1] = p2;
	if (p1 && p2) {
	    fprintf (stderr, "new max_sprite_draw=%d\n",mcc);
	    max_sprite_draw = mcc;
	}
    }
    if (delta_color_change) {
	void *p1,*p2;
	int mcc = max_color_change + 200 + delta_color_change;
	delta_color_change = 0;
	p1 = realloc (color_changes[0], mcc * sizeof (struct color_change));
	p2 = realloc (color_changes[1], mcc * sizeof (struct color_change));
	if (p1) color_changes[0] = p1;
	if (p2) color_changes[1] = p2;
	if (p1 && p2) {
	    fprintf (stderr, "new max_color_change=%d\n",mcc);
	    max_color_change = mcc;
	}
    }
    if (delta_delay_change) {
	void *p;
	int mcc = max_delay_change + 200 + delta_delay_change;
	delta_delay_change = 0;
	p = realloc (delay_changes, mcc * sizeof (struct delay_change));
	if (p) {
	    fprintf (stderr, "new max_delay_change=%d\n",mcc);
	    delay_changes = p;
	    max_delay_change = mcc;
	}
    }
#endif
}

static void vsync_handler (void)
{
    if (currprefs.m68k_speed == -1)
	rpt_did_reset = 0;

    INTREQ (0x8020);
    if (bplcon0 & 4)
	lof ^= 0x8000;

    if (quit_program > 0)
	return;

    /* For now, let's only allow this to change at vsync time.  It gets too
     * hairy otherwise.  */
    if (beamcon0 != new_beamcon0)
	init_hz ();

    lof_changed = 0;

    cop_state.ip = cop1lc;
    cop_state.state = COP_read1;
    cop_state.vpos = 0;
    cop_state.hpos = 0;
    cop_state.count = 0;
    cop_state.do_move = 0;
    cop_state.ignore_next = 0;
    cop_state.vdiw = DIW_waiting_start;

#ifdef HAVE_GETTIMEOFDAY
    {
	struct timeval tv;
	unsigned long int newtime;

	gettimeofday(&tv,NULL);
	newtime = (tv.tv_sec-seconds_base) * 1000 + tv.tv_usec / 1000;

	if (!bogusframe) {
	    lastframetime = newtime - msecs;
	    frametime += lastframetime;
	    timeframes++;
	}
	msecs = newtime;
	bogusframe = 0;
    }
#endif
    if (ievent_alive > 0)
	ievent_alive--;
    if (timehack_alive > 0)
	timehack_alive--;
    CIA_vsync_handler();
}

static void hsync_handler (void)
{
    int nr;

    eventtab[ev_hsync].evtime += cycles - eventtab[ev_hsync].oldcycles;
    eventtab[ev_hsync].oldcycles = cycles;

    CIA_hsync_handler();
  
    update_audio ();
    
    /* Sound data is fetched at the beginning of each line */
    for (nr = 0; nr < 4; nr++) {
	struct audio_channel_data *cdp = audio_channel + nr;

	if (cdp->data_written == 2) {
	    cdp->data_written = 0;

#if AUDIO_DEBUG	   
	    if (cdp->state != 0 && cdp->pt >= cdp->ptend) {
		fprintf(stderr, "Audio DMA fetch overrun on channel %d: %.8x/%.8x\n", nr, cdp->pt, cdp->ptend);
	    }
#endif

	    cdp->nextdat = chipmem_bank.wget(cdp->pt);

	    cdp->nextdatpt = cdp->pt;
	    cdp->nextdatptend = cdp->ptend;

	    if (cdp->wlen != 1)
		cdp->pt += 2;

	    if (cdp->state == 2 || cdp->state == 3) {
		if (cdp->wlen == 1) {
		    cdp->pt = cdp->lc;
		    cdp->ptend = cdp->lc + 2 * (cdp->len ? cdp->len : 65536);
		    cdp->wlen = cdp->len;
		    cdp->intreq2 = 1;
		} else {
		    cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
		}
	    }
	}
    }
  
    if (++vpos == (maxvpos + (lof != 0))) {
	vpos = 0;
	vsync_handler();
    }
  
    is_lastline = vpos + 1 == maxvpos + (lof != 0) && currprefs.m68k_speed == -1 && ! rpt_did_reset;
}

static void init_eventtab (void)
{
    int i;

    for(i = 0; i < ev_max; i++) {
	eventtab[i].active = 0;
	eventtab[i].oldcycles = 0;
    }

    eventtab[ev_cia].handler = CIA_handler;
    eventtab[ev_copper].handler = do_copper;
    eventtab[ev_hsync].handler = hsync_handler;
    eventtab[ev_hsync].evtime = maxhpos + cycles;
    eventtab[ev_hsync].active = 1;

    events_schedule ();
}

void customreset (void)
{
    int i;
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
#endif

    CIA_reset ();
    cycles = 0;
    regs.spcflags &= SPCFLAG_BRK;

    vpos = 0;
    lof = 0;

    ievent_alive = 0;
    timehack_alive = 0;

    clx_sprmask = 0xFF;
    clxdat = 0;

    memset (sprarmed, 0, sizeof sprarmed);
    nr_armed = 0;

    dmacon = intena = 0;
    cop_state.state = COP_stop;
    diwstate = DIW_waiting_start;
    hdiwstate = DIW_waiting_start;
    copcon = 0;
    cycles = 0;

    bplcon4 = 0x11; /* Get AGA chipset into ECS compatibility mode */
    bplcon3 = 0xC00;

    new_beamcon0 = ntscmode ? 0x00 : 0x20;
    init_hz ();

    audio_reset ();

    init_eventtab ();

#ifdef HAVE_GETTIMEOFDAY
    gettimeofday (&tv, NULL);
    seconds_base = tv.tv_sec;
    bogusframe = 1;
#endif
}

void dumpcustom (void)
{
    write_log ("DMACON: %x INTENA: %x INTREQ: %x VPOS: %x HPOS: %x CYCLES: %ld\n", DMACONR(),
	       (unsigned int)intena, (unsigned int)intreq, (unsigned int)vpos, (unsigned int)current_hpos(), cycles);
    write_log ("COP1LC: %08lx, COP2LC: %08lx\n", (unsigned long)cop1lc, (unsigned long)cop2lc);
    if (timeframes) {
	write_log ("Average frame time: %d ms [frames: %d time: %d]\n",
		   frametime / timeframes, timeframes, frametime);
	if (total_skipped)
	    write_log ("Skipped frames: %d\n", total_skipped);
    }
}

int intlev (void)
{
    uae_u16 imask = intreq & intena;
    if (imask && (intena & 0x4000)){
	if (imask & 0x2000) return 6;
	if (imask & 0x1800) return 5;
	if (imask & 0x0780) return 4;
	if (imask & 0x0070) return 3;
	if (imask & 0x0008) return 2;
	if (imask & 0x0007) return 1;
    }
    return -1;
}

static void gen_custom_tables (void)
{
    int i;
    for (i = 0; i < 256; i++) {
	unsigned int j;
	sprtaba[i] = ((((i >> 7) & 1) << 0)
		      | (((i >> 6) & 1) << 2)
		      | (((i >> 5) & 1) << 4)
		      | (((i >> 4) & 1) << 6)
		      | (((i >> 3) & 1) << 8)
		      | (((i >> 2) & 1) << 10)
		      | (((i >> 1) & 1) << 12)
		      | (((i >> 0) & 1) << 14));
	sprtabb[i] = sprtaba[i] * 2;
	for (j = 0; j < 511; j = (j << 1) | 1)
	    if ((i & ~j) == 0)
		waitmasktab[i] = ~j;
    }
}

void custom_init (void)
{
    gen_custom_tables ();
}

/* Custom chip memory bank */

static uae_u32 custom_lget (uaecptr) REGPARAM;
static uae_u32 custom_wget (uaecptr) REGPARAM;
static uae_u32 custom_bget (uaecptr) REGPARAM;
static void custom_lput (uaecptr, uae_u32) REGPARAM;
static void custom_wput (uaecptr, uae_u32) REGPARAM;
static void custom_bput (uaecptr, uae_u32) REGPARAM;

addrbank custom_bank = {
    custom_lget, custom_wget, custom_bget,
    custom_lput, custom_wput, custom_bput,
    default_xlate, default_check
};

static uae_u32 REGPARAM2 custom_wget (uaecptr addr)
{
    switch (addr & 0x1FE) {
     case 0x002: return DMACONR();
     case 0x004: return VPOSR();
     case 0x006: return VHPOSR();

     case 0x008: return DSKDATR();

     case 0x00A: return JOY0DAT();
     case 0x00C: return JOY1DAT();
     case 0x00E: return CLXDAT();
     case 0x010: return ADKCONR();

     case 0x012: return POT0DAT();
     case 0x016: return POTGOR();
     case 0x01A: return DSKBYTR();
     case 0x01C: return INTENAR();
     case 0x01E: return INTREQR();
     case 0x07C: return DENISEID();
     default:
       //        fprintf(stderr,"Non-read register read in custom chipset ($dff%x)",addr);
	custom_wput(addr,0);
	return 0xffff;
    }
}

static uae_u32 REGPARAM2 custom_bget (uaecptr addr)
{
    return custom_wget(addr & 0xfffe) >> (addr & 1 ? 0 : 8);
}

static uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
    return ((uae_u32)custom_wget(addr & 0xfffe) << 16) | custom_wget((addr+2) & 0xfffe);
}

static void REGPARAM2 custom_wput_1 (int hpos, uaecptr addr, uae_u32 value)
{
    addr &= 0x1FE;
    switch (addr) {
     case 0x020: DSKPTH (value); break;
     case 0x022: DSKPTL (value); break;
     case 0x024: DSKLEN (value); break;
     case 0x026: DSKDAT (value); break;

     case 0x02A: VPOSW (value); break;
     case 0x2E:  COPCON (value); break;
     case 0x34: POTGO (value); break;
     case 0x040: BLTCON0 (value); break;
     case 0x042: BLTCON1 (value); break;

     case 0x044: BLTAFWM (value); break;
     case 0x046: BLTALWM (value); break;

     case 0x050: BLTAPTH (value); break;
     case 0x052: BLTAPTL (value); break;
     case 0x04C: BLTBPTH (value); break;
     case 0x04E: BLTBPTL (value); break;
     case 0x048: BLTCPTH (value); break;
     case 0x04A: BLTCPTL (value); break;
     case 0x054: BLTDPTH (value); break;
     case 0x056: BLTDPTL (value); break;

     case 0x058: BLTSIZE (value); break;

     case 0x064: BLTAMOD (value); break;
     case 0x062: BLTBMOD (value); break;
     case 0x060: BLTCMOD (value); break;
     case 0x066: BLTDMOD (value); break;

     case 0x070: BLTCDAT (value); break;
     case 0x072: BLTBDAT (value); break;
     case 0x074: BLTADAT (value); break;

     case 0x07E: DSKSYNC (value); break;

     case 0x080: COP1LCH (value); break;
     case 0x082: COP1LCL (value); break;
     case 0x084: COP2LCH (value); break;
     case 0x086: COP2LCL (value); break;

     case 0x088: COPJMP1 (value); break;
     case 0x08A: COPJMP2 (value); break;

     case 0x08E: DIWSTRT (hpos, value); break;
     case 0x090: DIWSTOP (hpos, value); break;
     case 0x092: DDFSTRT (hpos, value); break;
     case 0x094: DDFSTOP (hpos, value); break;

     case 0x096: DMACON (value); break;
     case 0x098: CLXCON (value); break;
     case 0x09A: INTENA (value); break;
     case 0x09C: INTREQ (value); break;
     case 0x09E: ADKCON (value); break;

     case 0x0A0: AUDxLCH (0, value); break;
     case 0x0A2: AUDxLCL (0, value); break;
     case 0x0A4: AUDxLEN (0, value); break;
     case 0x0A6: AUDxPER (0, value); break;
     case 0x0A8: AUDxVOL (0, value); break;
     case 0x0AA: AUDxDAT (0, value); break;

     case 0x0B0: AUDxLCH (1, value); break;
     case 0x0B2: AUDxLCL (1, value); break;
     case 0x0B4: AUDxLEN (1, value); break;
     case 0x0B6: AUDxPER (1, value); break;
     case 0x0B8: AUDxVOL (1, value); break;
     case 0x0BA: AUDxDAT (1, value); break;

     case 0x0C0: AUDxLCH (2, value); break;
     case 0x0C2: AUDxLCL (2, value); break;
     case 0x0C4: AUDxLEN (2, value); break;
     case 0x0C6: AUDxPER (2, value); break;
     case 0x0C8: AUDxVOL (2, value); break;
     case 0x0CA: AUDxDAT (2, value); break;

     case 0x0D0: AUDxLCH (3, value); break;
     case 0x0D2: AUDxLCL (3, value); break;
     case 0x0D4: AUDxLEN (3, value); break;
     case 0x0D6: AUDxPER (3, value); break;
     case 0x0D8: AUDxVOL (3, value); break;
     case 0x0DA: AUDxDAT (3, value); break;

     case 0x0E0: BPLPTH (hpos, value, 0); break;
     case 0x0E2: BPLPTL (hpos, value, 0); break;
     case 0x0E4: BPLPTH (hpos, value, 1); break;
     case 0x0E6: BPLPTL (hpos, value, 1); break;
     case 0x0E8: BPLPTH (hpos, value, 2); break;
     case 0x0EA: BPLPTL (hpos, value, 2); break;
     case 0x0EC: BPLPTH (hpos, value, 3); break;
     case 0x0EE: BPLPTL (hpos, value, 3); break;
     case 0x0F0: BPLPTH (hpos, value, 4); break;
     case 0x0F2: BPLPTL (hpos, value, 4); break;
     case 0x0F4: BPLPTH (hpos, value, 5); break;
     case 0x0F6: BPLPTL (hpos, value, 5); break;
     case 0x0F8: BPLPTH (hpos, value, 6); break;
     case 0x0FA: BPLPTL (hpos, value, 6); break;
     case 0x0FC: BPLPTH (hpos, value, 7); break;
     case 0x0FE: BPLPTL (hpos, value, 7); break;

     case 0x100: BPLCON0 (hpos, value); break;
     case 0x102: BPLCON1 (hpos, value); break;
     case 0x104: BPLCON2 (hpos, value); break;
     case 0x106: BPLCON3 (hpos, value); break;

     case 0x108: BPL1MOD (hpos, value); break;
     case 0x10A: BPL2MOD (hpos, value); break;

     case 0x110: BPL1DAT (value); break;
     case 0x112: BPL2DAT (value); break;
     case 0x114: BPL3DAT (value); break;
     case 0x116: BPL4DAT (value); break;
     case 0x118: BPL5DAT (value); break;
     case 0x11A: BPL6DAT (value); break;
     case 0x11C: BPL7DAT (value); break;
     case 0x11E: BPL8DAT (value); break;

     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
	COLOR (hpos, value & 0xFFF, (addr & 0x3E) / 2);
	break;
     case 0x120: case 0x124: case 0x128: case 0x12C:
     case 0x130: case 0x134: case 0x138: case 0x13C:
	SPRxPTH (hpos, value, (addr - 0x120) / 4);
	break;
     case 0x122: case 0x126: case 0x12A: case 0x12E:
     case 0x132: case 0x136: case 0x13A: case 0x13E:
	SPRxPTL (hpos, value, (addr - 0x122) / 4);
	break;
     case 0x140: case 0x148: case 0x150: case 0x158:
     case 0x160: case 0x168: case 0x170: case 0x178:
	SPRxPOS (hpos, value, (addr - 0x140) / 8);
	break;
     case 0x142: case 0x14A: case 0x152: case 0x15A:
     case 0x162: case 0x16A: case 0x172: case 0x17A:
	SPRxCTL (hpos, value, (addr - 0x142) / 8);
	break;
     case 0x144: case 0x14C: case 0x154: case 0x15C:
     case 0x164: case 0x16C: case 0x174: case 0x17C:
	SPRxDATA (hpos, value, (addr - 0x144) / 8);
	break;
     case 0x146: case 0x14E: case 0x156: case 0x15E:
     case 0x166: case 0x16E: case 0x176: case 0x17E:
	SPRxDATB (hpos, value, (addr - 0x146) / 8);
	break;

     case 0x36: JOYTEST (value); break;
     case 0x5A: BLTCON0L (value); break;
     case 0x5C: BLTSIZV (value); break;
     case 0x5E: BLTSIZH (value); break;
     case 0x1E4: DIWHIGH (hpos, value); break;
     case 0x10C: BPLCON4 (hpos, value); break;
     case 0x1FC: FMODE (value); break;

       /* shd's beamcon 0 patch */
    case 0x1DC: BEAMCON0 (value); break;

    }
}

static void REGPARAM2 custom_wput (uaecptr addr, uae_u32 value)
{
    int hpos = current_hpos ();
    update_copper (hpos);
    custom_wput_1 (hpos, addr, value);
}

static void REGPARAM2 custom_bput (uaecptr addr, uae_u32 value)
{
    static int warned = 0;
    /* Is this correct now? (There are people who bput things to the upper byte of AUDxVOL). */
    uae_u16 rval = (value << 8) | (value & 0xFF);
    //    uae_u16 rval = value & 0xFF;
    //    addr=(uaecptr) (((char*) addr)-1);
    custom_wput(addr, rval);
    if (!warned) {
        warned++;
	fprintf(stderr, "uade: Byte put to custom register (0x%x to $%x)\n", rval, addr);
    }
}

static void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
    custom_wput(addr & 0xfffe, value >> 16);
    custom_wput((addr+2) & 0xfffe, (uae_u16)value);
}
