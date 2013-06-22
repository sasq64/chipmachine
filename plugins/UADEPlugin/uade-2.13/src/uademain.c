 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "uae.h"
#include "gensound.h"
#include "sd-sound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "readcpu.h"
#include "newcpu.h"
#include "debug.h"
#include "osemu.h"
#include "compiler.h"

#include "uade.h"
#include "ossupport.h"

struct uae_prefs currprefs, changed_prefs;

int no_gui = 0;
int joystickpresent = 0;
int cloanto_rom = 0;

char warning_buffer[256];

/* If you want to pipe printer output to a file, put something like
 * "cat >>printerfile.tmp" above.
 * The printer support was only tested with the driver "PostScript" on
 * Amiga side, using apsfilter for linux to print ps-data.
 *
 * Under DOS it ought to be -p LPT1: or -p PRN: but you'll need a
 * PostScript printer or ghostscript -=SR=-
 */

/* People must provide their own name for this */
char sername[256] = "";

/* Slightly stupid place for this... */
/* ncurses.c might use quite a few of those. */
char *colormodes[] = { "256 colors", "32768 colors", "65536 colors",
    "256 colors dithered", "16 colors dithered", "16 million colors",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

void discard_prefs (struct uae_prefs *p)
{
    struct strlist **ps = &p->unknown_lines;
    while (*ps) {
	struct strlist *s = *ps;
	*ps = s->next;
	free (s->str);
	free (s);
    }
}

void default_prefs (struct uae_prefs *p)
{
    strcpy (p->description, "UAE default configuration");

    p->start_gui = 1;
    p->start_debugger = 0;

    p->unknown_lines = 0;
    /* Note to porters: please don't change any of these options! UAE is supposed
     * to behave identically on all platforms if possible. */
    p->illegal_mem = 0;
    p->no_xhair = 0;
    p->use_serial = 0;
    p->serial_demand = 0;
    p->parallel_demand = 0;
    p->automount_uaedev = 1;

    p->jport0 = 2;
    p->jport1 = 0;
    p->keyboard_lang = KBD_LANG_US;
    p->emul_accuracy = 2;
    p->test_drawing_speed = 0;

    p->produce_sound = 0;
    p->stereo = 0;
    p->sound_bits = DEFAULT_SOUND_BITS;
    p->sound_freq = DEFAULT_SOUND_FREQ;
    p->sound_minbsiz = DEFAULT_SOUND_MINB;
    p->sound_maxbsiz = DEFAULT_SOUND_MAXB;
    p->sound_interpol = 0;

    p->gfx_framerate = 1;
    p->gfx_width = 800;
    p->gfx_height = 600;
    p->gfx_lores = 0;
    p->gfx_linedbl = 0;
    p->gfx_afullscreen = 0;
    p->gfx_pfullscreen = 0;
    p->gfx_correct_aspect = 0;
    p->gfx_xcenter = 0;
    p->gfx_ycenter = 0;
    p->color_mode = 0;

    p->x11_use_low_bandwidth = 0;
    p->x11_use_mitshm = 0;
    p->x11_hide_cursor = 1;

    p->svga_no_linear = 0;

    p->win32_middle_mouse = 0;
    p->win32_sound_style = 0;
    p->win32_sound_tweak = 0;
    p->win32_logfile = 0;
    p->win32_iconified_nospeed = 0;
    p->win32_iconified_nosound = 0;

    p->immediate_blits = 0;
    p->blits_32bit_enabled = 0;

    strcpy (p->df[0], "df0.adf");
    strcpy (p->df[1], "df1.adf");
    strcpy (p->df[2], "df2.adf");
    strcpy (p->df[3], "df3.adf");

    strcpy (p->romfile, "kick.rom");
    strcpy (p->keyfile, "");
    strcpy (p->prtname, "");

    strcpy (p->path_rom, "./");
    strcpy (p->path_floppy, "./");
    strcpy (p->path_hardfile, "./");

    p->m68k_speed = 4;
    p->cpu_level = 2;
    p->cpu_compatible = 0;
    p->address_space_24 = 0;

    p->fastmem_size = 0x00000000;
    p->a3000mem_size = 0x00000000;
    p->z3fastmem_size = 0x00000000;
    p->chipmem_size = 0x00200000;
    p->bogomem_size = 0x00000000;
    p->gfxmem_size = 0x00000000;

}

void fixup_prefs_dimensions (struct uae_prefs *prefs)
{
    if (prefs->gfx_width < 320)
	prefs->gfx_width = 320;
    if (prefs->gfx_height < 200)
	prefs->gfx_height = 200;
    if (prefs->gfx_height > 300 && ! prefs->gfx_linedbl)
	prefs->gfx_height = 300;
    if (prefs->gfx_height > 600)
	prefs->gfx_height = 600;

    prefs->gfx_width += 7; /* X86.S wants multiples of 4 bytes, might be 8 in the future. */
    prefs->gfx_width &= ~7;
}

static void fix_options (void)
{
    int err = 0;

    if ((currprefs.chipmem_size & (currprefs.chipmem_size - 1)) != 0
	|| currprefs.chipmem_size < 0x80000
	|| currprefs.chipmem_size > 0x800000)
    {
	currprefs.chipmem_size = 0x200000;
	fprintf (stderr, "Unsupported chipmem size!\n");
	err = 1;
    }
    if ((currprefs.fastmem_size & (currprefs.fastmem_size - 1)) != 0
	|| (currprefs.fastmem_size != 0 && (currprefs.fastmem_size < 0x100000 || currprefs.fastmem_size > 0x800000)))
    {
	currprefs.fastmem_size = 0;
	fprintf (stderr, "Unsupported fastmem size!\n");
	err = 1;
    }
    if ((currprefs.gfxmem_size & (currprefs.gfxmem_size - 1)) != 0
	|| (currprefs.gfxmem_size != 0 && (currprefs.gfxmem_size < 0x100000 || currprefs.gfxmem_size > 0x800000)))
    {
	currprefs.gfxmem_size = 0;
	fprintf (stderr, "Unsupported graphics card memory size!\n");
	err = 1;
    }
    if ((currprefs.z3fastmem_size & (currprefs.z3fastmem_size - 1)) != 0
	|| (currprefs.z3fastmem_size != 0 && (currprefs.z3fastmem_size < 0x100000 || currprefs.z3fastmem_size > 0x4000000)))
    {
	currprefs.z3fastmem_size = 0;
	fprintf (stderr, "Unsupported Zorro III fastmem size!\n");
	err = 1;
    }
    if (currprefs.address_space_24 && (currprefs.gfxmem_size != 0 || currprefs.z3fastmem_size != 0)) {
	currprefs.z3fastmem_size = currprefs.gfxmem_size = 0;
	fprintf (stderr, "Can't use a graphics card or Zorro III fastmem when using a 24 bit\n"
		 "address space - sorry.\n");
    }
    if ((currprefs.bogomem_size & (currprefs.bogomem_size - 1)) != 0
	|| (currprefs.bogomem_size != 0 && (currprefs.bogomem_size < 0x80000 || currprefs.bogomem_size > 0x100000)))
    {
	currprefs.bogomem_size = 0;
	fprintf (stderr, "Unsupported bogomem size!\n");
	err = 1;
    }

    if (currprefs.chipmem_size > 0x200000 && currprefs.fastmem_size != 0) {
	fprintf (stderr, "You can't use fastmem and more than 2MB chip at the same time!\n");
	currprefs.fastmem_size = 0;
	err = 1;
    }
    if (currprefs.m68k_speed < -1 || currprefs.m68k_speed > 20) {
	fprintf (stderr, "Bad value for -w parameter: must be -1, 0, or within 1..20.\n");
	currprefs.m68k_speed = 4;
	err = 1;
    }
    if (currprefs.produce_sound < 0 || currprefs.produce_sound > 3) {
	fprintf (stderr, "Bad value for -S parameter: enable value must be within 0..3\n");
	currprefs.produce_sound = 0;
	err = 1;
    }
    if (currprefs.cpu_level < 2 && currprefs.z3fastmem_size > 0) {
	fprintf (stderr, "Z3 fast memory can't be used with a 68000/68010 emulation. It\n"
		 "requires a 68020 emulation. Turning off Z3 fast memory.\n");
	currprefs.z3fastmem_size = 0;
	err = 1;
    }
    if (currprefs.gfxmem_size > 0 && (currprefs.cpu_level < 2 || currprefs.address_space_24)) {
	fprintf (stderr, "Picasso96 can't be used with a 68000/68010 or 68EC020 emulation. It\n"
		 "requires a 68020 emulation. Turning off Picasso96.\n");
	currprefs.gfxmem_size = 0;
	err = 1;
    }

    currprefs.socket_emu = 0;

    if (err)
	fprintf (stderr, "Please use \"uae -h\" to get usage information.\n");
}

int quit_program = 0;

void uae_quit (void)
{
  quit_program = 1;
}

void write_log_standard (const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
#ifdef HAVE_VFPRINTF
    vfprintf (stderr, fmt, ap);
#else
    /* Technique stolen from GCC.  */
    {
	int x1, x2, x3, x4, x5, x6, x7, x8;
	x1 = va_arg (ap, int);
	x2 = va_arg (ap, int);
	x3 = va_arg (ap, int);
	x4 = va_arg (ap, int);
	x5 = va_arg (ap, int);
	x6 = va_arg (ap, int);
	x7 = va_arg (ap, int);
	x8 = va_arg (ap, int);
	fprintf (stderr, fmt, x1, x2, x3, x4, x5, x6, x7, x8);
    }
#endif
}


int uade_main (int argc, char **argv)
{
    default_prefs (&currprefs);

    uade_option (argc, argv);

    machdep_init ();

    if (! setup_sound ()) {
	fprintf (stderr, "Sound driver unavailable: Sound output disabled\n");
	currprefs.produce_sound = 0;
	exit(-1);
    }

    init_sound();

    fix_options ();
    changed_prefs = currprefs;
    check_prefs_changed_cpu();

    memory_init ();

    custom_init (); /* Must come after memory_init */

    reset_frame_rate_hack ();
    init_m68k(); /* must come after reset_frame_rate_hack (); */

    /* compiler_init (); */

    if (currprefs.start_debugger)
      activate_debugger ();

    m68k_go();

    close_sound ();
    dump_counts ();

    return 0;
}
