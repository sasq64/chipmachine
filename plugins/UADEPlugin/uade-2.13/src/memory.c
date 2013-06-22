 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"

#include "uade.h"

#ifdef USE_MAPPED_MEMORY
#include <sys/mman.h>
#endif

int ersatzkickfile = 0;

uae_u32 allocated_chipmem;
uae_u32 allocated_fastmem;
uae_u32 allocated_bogomem;
uae_u32 allocated_gfxmem;
uae_u32 allocated_z3fastmem;
uae_u32 allocated_a3000mem;

#ifdef SAVE_MEMORY_BANKS
addrbank *mem_banks[65536];
#else
addrbank mem_banks[65536];
#endif

#ifdef NO_INLINE_MEMORY_ACCESS
inline uae_u32 longget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).lget, addr);
}
inline uae_u32 wordget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).wget, addr);
}
inline uae_u32 byteget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).bget, addr);
}
inline void longput (uaecptr addr, uae_u32 l)
{
    call_mem_put_func (get_mem_bank (addr).lput, addr, l);
}
inline void wordput (uaecptr addr, uae_u32 w)
{
    call_mem_put_func (get_mem_bank (addr).wput, addr, w);
}
inline void byteput (uaecptr addr, uae_u32 b)
{
    call_mem_put_func (get_mem_bank (addr).bput, addr, b);
}
#endif

static uae_u32 chipmem_mask, kickmem_mask, bogomem_mask, a3000mem_mask;

/* A dummy bank that only contains zeros */

static uae_u32 dummy_lget (uaecptr) REGPARAM;
static uae_u32 dummy_wget (uaecptr) REGPARAM;
static uae_u32 dummy_bget (uaecptr) REGPARAM;
static void dummy_lput (uaecptr, uae_u32) REGPARAM;
static void dummy_wput (uaecptr, uae_u32) REGPARAM;
static void dummy_bput (uaecptr, uae_u32) REGPARAM;
static int dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

static uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal lget at %08lx\n", addr);

    return 0;
}

static uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal wget at %08lx\n", addr);

    return 0;
}

static uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal bget at %08lx\n", addr);

    return 0;
}

static void REGPARAM2 dummy_lput (uaecptr addr, uae_u32 l)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal lput at %08lx\n", addr);
}
static void REGPARAM2 dummy_wput (uaecptr addr, uae_u32 w)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal wput at %08lx\n", addr);
}
static void REGPARAM2 dummy_bput (uaecptr addr, uae_u32 b)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal bput at %08lx\n", addr);
}

static int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal check at %08lx\n", addr);

    return 0;
}

/* A3000 "motherboard resources" bank.  */
static uae_u32 mbres_lget (uaecptr) REGPARAM;
static uae_u32 mbres_wget (uaecptr) REGPARAM;
static uae_u32 mbres_bget (uaecptr) REGPARAM;
static void mbres_lput (uaecptr, uae_u32) REGPARAM;
static void mbres_wput (uaecptr, uae_u32) REGPARAM;
static void mbres_bput (uaecptr, uae_u32) REGPARAM;
static int mbres_check (uaecptr addr, uae_u32 size) REGPARAM;

static int mbres_val = 0;

static uae_u32 REGPARAM2 mbres_lget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal lget at %08lx\n", addr);

    return 0;
}

static uae_u32 REGPARAM2 mbres_wget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal wget at %08lx\n", addr);

    return 0;
}

static uae_u32 REGPARAM2 mbres_bget (uaecptr addr)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal bget at %08lx\n", addr);

    return (addr & 0xFFFF) == 3 ? mbres_val : 0;
}

static void REGPARAM2 mbres_lput (uaecptr addr, uae_u32 l)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal lput at %08lx\n", addr);
}
static void REGPARAM2 mbres_wput (uaecptr addr, uae_u32 w)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal wput at %08lx\n", addr);
}
static void REGPARAM2 mbres_bput (uaecptr addr, uae_u32 b)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal bput at %08lx\n", addr);

    if ((addr & 0xFFFF) == 3)
	mbres_val = b;
}

static int REGPARAM2 mbres_check (uaecptr addr, uae_u32 size)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal check at %08lx\n", addr);

    return 0;
}

/* Chip memory */

uae_u8 *chipmemory;

static uae_u32 chipmem_lget (uaecptr) REGPARAM;
static uae_u32 chipmem_wget (uaecptr) REGPARAM;
static uae_u32 chipmem_bget (uaecptr) REGPARAM;
static void chipmem_lput (uaecptr, uae_u32) REGPARAM;
static void chipmem_wput (uaecptr, uae_u32) REGPARAM;
static void chipmem_bput (uaecptr, uae_u32) REGPARAM;
static int chipmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *chipmem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 chipmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_bget (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return chipmemory[addr];
}

static void REGPARAM2 chipmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 chipmem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);
    do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_bput (uaecptr addr, uae_u32 b)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    chipmemory[addr] = b;
}

static int REGPARAM2 chipmem_check (uaecptr addr, uae_u32 size)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return (addr + size) <= allocated_chipmem;
}

static uae_u8 REGPARAM2 *chipmem_xlate (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return chipmemory + addr;
}

/* Slow memory */

static uae_u8 *bogomemory;

static uae_u32 bogomem_lget (uaecptr) REGPARAM;
static uae_u32 bogomem_wget (uaecptr) REGPARAM;
static uae_u32 bogomem_bget (uaecptr) REGPARAM;
static void bogomem_lput (uaecptr, uae_u32) REGPARAM;
static void bogomem_wput (uaecptr, uae_u32) REGPARAM;
static void bogomem_bput (uaecptr, uae_u32) REGPARAM;
static int bogomem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *bogomem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 bogomem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    m = (uae_u32 *)(bogomemory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 bogomem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    m = (uae_u16 *)(bogomemory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 bogomem_bget (uaecptr addr)
{
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    return bogomemory[addr];
}

static void REGPARAM2 bogomem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    m = (uae_u32 *)(bogomemory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 bogomem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    m = (uae_u16 *)(bogomemory + addr);
    do_put_mem_word (m, w);
}

static void REGPARAM2 bogomem_bput (uaecptr addr, uae_u32 b)
{
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    bogomemory[addr] = b;
}

static int REGPARAM2 bogomem_check (uaecptr addr, uae_u32 size)
{
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    return (addr + size) <= allocated_bogomem;
}

static uae_u8 REGPARAM2 *bogomem_xlate (uaecptr addr)
{
    addr -= bogomem_start & bogomem_mask;
    addr &= bogomem_mask;
    return bogomemory + addr;
}

/* A3000 motherboard fast memory */

static uae_u8 *a3000memory;

static uae_u32 a3000mem_lget (uaecptr) REGPARAM;
static uae_u32 a3000mem_wget (uaecptr) REGPARAM;
static uae_u32 a3000mem_bget (uaecptr) REGPARAM;
static void a3000mem_lput (uaecptr, uae_u32) REGPARAM;
static void a3000mem_wput (uaecptr, uae_u32) REGPARAM;
static void a3000mem_bput (uaecptr, uae_u32) REGPARAM;
static int a3000mem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *a3000mem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 a3000mem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u32 *)(a3000memory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 a3000mem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u16 *)(a3000memory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 a3000mem_bget (uaecptr addr)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return a3000memory[addr];
}

static void REGPARAM2 a3000mem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u32 *)(a3000memory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 a3000mem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u16 *)(a3000memory + addr);
    do_put_mem_word (m, w);
}

static void REGPARAM2 a3000mem_bput (uaecptr addr, uae_u32 b)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    a3000memory[addr] = b;
}

static int REGPARAM2 a3000mem_check (uaecptr addr, uae_u32 size)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return (addr + size) <= allocated_a3000mem;
}

static uae_u8 REGPARAM2 *a3000mem_xlate(uaecptr addr)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return a3000memory + addr;
}

/* Kick memory */

static uae_u8 *kickmemory;

static uae_u32 kickmem_lget (uaecptr) REGPARAM;
static uae_u32 kickmem_wget (uaecptr) REGPARAM;
static uae_u32 kickmem_bget (uaecptr) REGPARAM;
static void  kickmem_lput (uaecptr, uae_u32) REGPARAM;
static void  kickmem_wput (uaecptr, uae_u32) REGPARAM;
static void  kickmem_bput (uaecptr, uae_u32) REGPARAM;
static int  kickmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *kickmem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 kickmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= kickmem_start & kickmem_mask;
    addr &= kickmem_mask;
    m = (uae_u32 *)(kickmemory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 kickmem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= kickmem_start & kickmem_mask;
    addr &= kickmem_mask;
    m = (uae_u16 *)(kickmemory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 kickmem_bget (uaecptr addr)
{
    addr -= kickmem_start & kickmem_mask;
    addr &= kickmem_mask;
    return kickmemory[addr];
}

static void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 b)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal kickmem lput at %08lx\n", addr);
}

static void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 b)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal kickmem wput at %08lx\n", addr);
}

static void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
    if (currprefs.illegal_mem)
	write_log ("Illegal kickmem lput at %08lx\n", addr);
}

static int REGPARAM2 kickmem_check (uaecptr addr, uae_u32 size)
{
    addr -= kickmem_start & kickmem_mask;
    addr &= kickmem_mask;
    return (addr + size) <= kickmem_size;
}

static uae_u8 REGPARAM2 *kickmem_xlate (uaecptr addr)
{
    addr -= kickmem_start & kickmem_mask;
    addr &= kickmem_mask;
    return kickmemory + addr;
}

/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
    return 0;
}

uae_u8 REGPARAM2 *default_xlate (uaecptr a)
{
    uade_song_end("the amiga player did something terribly stupid", 1);
    return kickmem_xlate (get_long (0xF80000)); /* So we don't crash. */
}

/* Address banks */

static addrbank dummy_bank = {
    dummy_lget, dummy_wget, dummy_bget,
    dummy_lput, dummy_wput, dummy_bput,
    default_xlate, dummy_check
};

static addrbank mbres_bank = {
    mbres_lget, mbres_wget, mbres_bget,
    mbres_lput, mbres_wput, mbres_bput,
    default_xlate, mbres_check
};

addrbank chipmem_bank = {
    chipmem_lget, chipmem_wget, chipmem_bget,
    chipmem_lput, chipmem_wput, chipmem_bput,
    chipmem_xlate, chipmem_check
};

static addrbank bogomem_bank = {
    bogomem_lget, bogomem_wget, bogomem_bget,
    bogomem_lput, bogomem_wput, bogomem_bput,
    bogomem_xlate, bogomem_check
};

static addrbank a3000mem_bank = {
    a3000mem_lget, a3000mem_wget, a3000mem_bget,
    a3000mem_lput, a3000mem_wput, a3000mem_bput,
    a3000mem_xlate, a3000mem_check
};

addrbank kickmem_bank = {
    kickmem_lget, kickmem_wget, kickmem_bget,
    kickmem_lput, kickmem_wput, kickmem_bput,
    kickmem_xlate, kickmem_check
};

char *address_space, *good_address_map;
static int good_address_fd;

static void init_mem_banks (void)
{
    int i;
    for (i = 0; i < 65536; i++)
	put_mem_bank (i<<16, &dummy_bank);
}

#define MAKE_USER_PROGRAMS_BEHAVE 1
void memory_init (void)
{
    char buffer[4096];
    char *nam;
    int i, fd;
    int custom_start;

    allocated_chipmem = currprefs.chipmem_size;
    allocated_bogomem = currprefs.bogomem_size;
    allocated_a3000mem = currprefs.a3000mem_size;

#ifdef USE_MAPPED_MEMORY
    fd = open ("/dev/zero", O_RDWR);
    good_address_map = mmap (NULL, 1 << 24, PROT_READ, MAP_PRIVATE, fd, 0);
    /* Don't believe USER_PROGRAMS_BEHAVE. Otherwise, we'd segfault as soon
     * as a decrunch routine tries to do color register hacks. */
    address_space = mmap (NULL, 1 << 24, PROT_READ | (USER_PROGRAMS_BEHAVE || MAKE_USER_PROGRAMS_BEHAVE? PROT_WRITE : 0), MAP_PRIVATE, fd, 0);
    if ((int)address_space < 0 || (int)good_address_map < 0) {
	write_log ("Your system does not have enough virtual memory - increase swap.\n");
	abort ();
    }
#ifdef MAKE_USER_PROGRAMS_BEHAVE
    memset (address_space + 0xDFF180, 0xFF, 32*2);
#else
    /* Likewise. This is mostly for mouse button checks. */
    if (USER_PROGRAMS_BEHAVE)
	memset (address_space + 0xA00000, 0xFF, 0xF00000 - 0xA00000);
#endif
    chipmemory = mmap (address_space, 0x200000, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fd, 0);
    kickmemory = mmap (address_space + 0xF80000, 0x80000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, fd, 0);

    close(fd);

    good_address_fd = open (nam = tmpnam (NULL), O_CREAT|O_RDWR, 0600);
    memset (buffer, 1, sizeof(buffer));
    write (good_address_fd, buffer, sizeof buffer);
    unlink (nam);

    for (i = 0; i < allocated_chipmem; i += 4096)
	mmap (good_address_map + i, 4096, PROT_READ, MAP_FIXED | MAP_PRIVATE,
	      good_address_fd, 0);
    for (i = 0; i < kickmem_size; i += 4096)
	mmap (good_address_map + i + 0x1000000 - kickmem_size, 4096, PROT_READ,
	      MAP_FIXED | MAP_PRIVATE, good_address_fd, 0);
#else
    kickmemory = (uae_u8 *)xmalloc (kickmem_size);
    chipmemory = (uae_u8 *) calloc (1, allocated_chipmem);

    while (! chipmemory && allocated_chipmem > 512*1024) {
	allocated_chipmem >>= 1;
	chipmemory = (uae_u8 *) calloc (1, allocated_chipmem);
	if (chipmemory)
	    fprintf (stderr, "Reducing chipmem size to %dkb\n", allocated_chipmem >> 10);
    }
    if (! chipmemory) {
	write_log ("virtual memory exhausted (chipmemory)!\n");
	abort ();
    }
#endif

    do_put_mem_long ((uae_u32 *)(chipmemory + 4), 0);
    init_mem_banks ();

    /* Map the chipmem into all of the lower 16MB */
    map_banks (&chipmem_bank, 0x00, 256);
    custom_start = 0xC0;

    map_banks (&custom_bank, custom_start, 0xE0-custom_start);
    map_banks (&cia_bank, 0xA0, 32);
    map_banks (&clock_bank, 0xDC, 1);

    /* @@@ Does anyone have a clue what should be in the 0x200000 - 0xA00000
     * range on an Amiga without expansion memory?  */
    custom_start = allocated_chipmem >> 16;
    if (custom_start < 0x20)
	custom_start = 0x20;
    map_banks (&dummy_bank, custom_start, 0xA0 - custom_start);
    /*map_banks (&mbres_bank, 0xDE, 1);*/

    if (allocated_bogomem > 0)
	bogomemory = (uae_u8 *)xmalloc (allocated_bogomem);
    if (bogomemory != NULL)
	map_banks (&bogomem_bank, 0xC0, allocated_bogomem >> 16);
    else
	allocated_bogomem = 0;

    if (allocated_a3000mem > 0)
	a3000memory = (uae_u8 *)xmalloc (allocated_a3000mem);
    if (a3000memory != NULL)
	map_banks (&a3000mem_bank, a3000mem_start >> 16, allocated_a3000mem >> 16);
    else
	allocated_a3000mem = 0;

    map_banks (&kickmem_bank, 0xF8, 8);
    //    map_banks (&expamem_bank, 0xE8, 1);

    if (cloanto_rom)
	map_banks (&kickmem_bank, 0xE0, 8);

    chipmem_mask = allocated_chipmem - 1;
    kickmem_mask = kickmem_size - 1;
    bogomem_mask = allocated_bogomem - 1;
    a3000mem_mask = allocated_a3000mem - 1;

}

void map_banks (addrbank *bank, int start, int size)
{
    int bnr;
    unsigned long int hioffs = 0, endhioffs = 0x100;

    if (start >= 0x100) {
	for (bnr = start; bnr < start + size; bnr++)
	    put_mem_bank (bnr << 16, bank);
	return;
    }
    /* Some '020 Kickstarts apparently require a 24 bit address space... */
    if (currprefs.address_space_24)
	endhioffs = 0x10000;
    for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100)
	for (bnr = start; bnr < start+size; bnr++)
	    put_mem_bank ((bnr + hioffs) << 16, bank);
}
