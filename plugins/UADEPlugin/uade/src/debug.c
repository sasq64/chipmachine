 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Debugger
  *
  * (c) 1995 Bernd Schmidt
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "readcpu.h"
#include "newcpu.h"
#include "debug.h"
#include "cia.h"

static int debugger_active = 0;
static uaecptr skipaddr;
static int do_skip;
static int wait_interrupt;
int debugging = 0;
int debug_interrupt_happened;

void activate_debugger (void)
{
    do_skip = 0;
    if (debugger_active)
	return;
    debugger_active = 1;
    regs.spcflags |= SPCFLAG_BRK;
    debugging = 1;
}

int firsthist = 0;
int lasthist = 0;
#ifdef NEED_TO_DEBUG_BADLY
struct regstruct history[MAX_HIST];
union flagu historyf[MAX_HIST];
#else
uaecptr history[MAX_HIST];
#endif


static void ignore_ws (char **c)
{
    while (**c && isspace(**c)) (*c)++;
}


static uae_u32 readany (char **c)
{
  uae_u32 val = 0;
  char nc;
  int numbase = 10;    /* Default numeric base is 10 */

  ignore_ws (c);
  
  /* Check if the value is given in hexadecimal ('$' or "0x") */
  if (**c=='$' || strncmp("0x",*c,2)==0 ) {
    numbase = 16;
    (*c) += 1 + (strncmp("0x",*c,2)==0 ? 1 : 0);
  }

  while (isxdigit(nc = **c)) {
    (*c)++;
    val *= numbase;
    nc = toupper(nc);
    if (isdigit(nc)) {
      val += nc - '0';
    } else if (numbase == 16) {
      val += nc - 'A' + 10;
    }
  }
  return val;
}

static uae_u32 readhex (char **c)
{
  uae_u32 val = 0;
  char nc;
  
  ignore_ws (c);
  /* Check if there is '$' or "0x" prefix and skip them ... */
  if( **c=='$' || strncmp("0x",*c,2)==0 ) {
    (*c) += 1 + (strncmp("0x",*c,2)==0 ? 1 : 0);
  }

  while (isxdigit(nc = **c)) {
    (*c)++;
    val *= 16;
    nc = toupper(nc);
    if (isdigit(nc)) {
      val += nc - '0';
    } else {
      val += nc - 'A' + 10;
    }
  }
  return val;
}

static char next_char( char **c)
{
    ignore_ws (c);
    return *(*c)++;
}

static int more_params (char **c)
{
    ignore_ws (c);
    return (**c) != 0;
}

static void dumpmem (uaecptr addr, uaecptr *nxmem, int lines)
{
    char c;
    uaecptr tmpaddr;
    broken_in = 0;
    for (;lines-- && !broken_in;) {
	int i;
	printf ("%08lx ", addr);
	tmpaddr = addr;
	for (i = 0; i < 8; i++) {
	    printf ("%04x ", get_word(addr)); addr += 2;
	}
	printf ("|");
	for (; tmpaddr < addr; tmpaddr++) {
	    c = get_byte(tmpaddr);
	    if (c < 0x20 || c > 0x7e)
  	        c = '.';
	    printf ("%c", c);
	}
	printf ("|\n");
    }
    *nxmem = addr;
}

static uae_u32 uade_debug_search (char *name) {
  uae_u8 *p = get_real_address(0);
  uae_u32 baseptr = 0;
  uae_u32 ptr;
  uae_u32 infoptr;
  uae_u32 infoname;
  uae_s32 infoval;
  int infolen, i;

  for (ptr = 0; ptr < 0x10000; ptr += 2) {
    if (!strcmp((char *) chipmemory + ptr, "uade debug info")) {
      baseptr = ptr;
      break;
    }
  }
  if (!baseptr) {
    fprintf (stderr, "uade debug info not found\n");
    return 0;
  }

  infoptr = baseptr;

  i = 0;

  while (1) {
    if (!p[infoptr])
      break;
    infoname = infoptr;
    infolen = strlen((char *) p + infoptr) + 1;
    if (infolen %2 == 1)
      infolen++;
    infoptr += infolen;
    infoval = get_long(infoptr) + baseptr;
    infoptr += 4;
    if (i > 0) {
      if (!name) {
	printf("%.8x: %s\n", infoval, p + infoname);
      } else {
	if (strlen(name) > 0) {
	  if (!strncmp ((char *) p + infoname, name, strlen(name))) {
	    return infoval;
	  }
	}
      }
    }
    i++;
  }
  return 0;
}

static void foundmod (uae_u32 ptr, char *type)
{
    char name[21];
    uae_u8 *ptr2 = chipmemory + ptr;
    int i,length;

    printf ("Found possible %s module at 0x%lx.\n", type, ptr);
    memcpy (name, ptr2, 20);
    name[20] = '\0';

    /* Browse playlist */
    length = 0;
    for (i = 0x3b8; i < 0x438; i++)
	if (ptr2[i] > length)
	    length = ptr2[i];

    length = (length+1)*1024 + 0x43c;

    /* Add sample lengths */
    ptr2 += 0x2A;
    for (i = 0; i < 31; i++, ptr2 += 30)
	length += 2*((ptr2[0]<<8)+ptr2[1]);
    
    printf ("Name \"%s\", Length 0x%lx bytes.\n", name, length);
}

static void modulesearch (void)
{
    uae_u8 *p = get_real_address (0);
    uae_u32 ptr;

    for (ptr = 0; ptr < allocated_chipmem - 40; ptr += 2, p += 2) {
	/* Check for Mahoney & Kaktus */
	/* Anyone got the format of old 15 Sample (SoundTracker)modules? */
	if (ptr >= 0x438 && p[0] == 'M' && p[1] == '.' && p[2] == 'K' && p[3] == '.')
	    foundmod (ptr - 0x438, "ProTracker (31 samples)");

	if (ptr >= 0x438 && p[0] == 'F' && p[1] == 'L' && p[2] == 'T' && p[3] == '4')
	    foundmod (ptr - 0x438, "Startrekker");

	if (strncmp ((char *)p, "SMOD", 4) == 0) {
	    printf ("Found possible FutureComposer 1.3 module at 0x%lx, length unknown.\n", ptr);
	}
	if (strncmp ((char *)p, "FC14", 4) == 0) {
	    printf ("Found possible FutureComposer 1.4 module at 0x%lx, length unknown.\n", ptr);
	}
	if (p[0] == 0x48 && p[1] == 0xe7 && p[4] == 0x61 && p[5] == 0
	    && p[8] == 0x4c && p[9] == 0xdf && p[12] == 0x4e && p[13] == 0x75
	    && p[14] == 0x48 && p[15] == 0xe7 && p[18] == 0x61 && p[19] == 0
	    && p[22] == 0x4c && p[23] == 0xdf && p[26] == 0x4e && p[27] == 0x75) {
	    printf ("Found possible Whittaker module at 0x%lx, length unknown.\n", ptr);
	}
	if (p[4] == 0x41 && p[5] == 0xFA) {
	    int i;

	    for (i = 0; i < 0x240; i += 2)
		if (p[i] == 0xE7 && p[i + 1] == 0x42 && p[i + 2] == 0x41 && p[i + 3] == 0xFA)
		    break;
	    if (i < 0x240) {
		uae_u8 *p2 = p + i + 4;
		for (i = 0; i < 0x30; i += 2)
		    if (p2[i] == 0xD1 && p2[i + 1] == 0xFA) {
			printf ("Found possible MarkII module at %lx, length unknown.\n", ptr);
		    }
	    }
		
	}
    }
}

/* cheat-search by Holger Jakob */
static void cheatsearch (char **c)
{
    uae_u8 *p = get_real_address (0);
    static uae_u32 *vlist = NULL;
    uae_u32 ptr;
    uae_u32 val = 0;
    uae_u32 type = 0; /* not yet */
    uae_u32 count = 0;
    uae_u32 fcount = 0;
    uae_u32 full = 0;
    char nc;

    val = readany(c);

    if (vlist == NULL) {
	vlist = malloc (256*4);
	if (vlist != NULL) {
	    for (count = 0; count<255; count++)
		vlist[count] = 0;
	    count = 0;
	    for (ptr = 0; ptr < allocated_chipmem - 40; ptr += 2, p += 2) {
		if (ptr >= 0x438 && p[3] == (val & 0xff)
		    && p[2] == (val >> 8 & 0xff)
		    && p[1] == (val >> 16 & 0xff)
		    && p[0] == (val >> 24 & 0xff))
		{
		    if (count < 255) {
			vlist[count++]=ptr;
			printf ("%08x: %x%x%x%x\n",ptr,p[0],p[1],p[2],p[3]);
		    } else
			full = 1;
		}
	    }
	    printf ("Found %d possible addresses with %d\n",count,val);
	    printf ("Now continue with 'g' and use 'C' with a different value\n");
	}
    } else {
	for (count = 0; count<255; count++) {
	    if (p[vlist[count]+3] == (val & 0xff)
		&& p[vlist[count]+2] == (val>>8 & 0xff) 
		&& p[vlist[count]+1] == (val>>16 & 0xff)
		&& p[vlist[count]] == (val>>24 & 0xff))
	    {
		fcount++;
		printf ("%08x: %x%x%x%x\n", vlist[count], p[vlist[count]],
			p[vlist[count]+1], p[vlist[count]+2], p[vlist[count]+3]);
	    }
	}
	printf ("%d hits of %d found\n",fcount,val);
	free (vlist);
	vlist = NULL;
    }
}

static void writeintomem (char **c)
{
    uae_u8 *p = get_real_address (0);
    uae_u32 addr = 0;
    uae_u32 val = 0;
    char nc;
    int numbase=10;     /* Numeric base for value is 10 by default ! */
    int opsize=4;       /* write operation is 32 bits by default ! */

    if(strncasecmp(".B",*c,2)==0) {
      opsize=1;
      *c+=2;
    }
    if(strncasecmp(".W",*c,2)==0) {
      opsize=2;
      *c+=2;
    }
    if(strncasecmp(".L",*c,2)==0) {
      opsize=4;
      *c+=2;
    }

    addr = readhex(c);
    val = readany(c);

    /* Check for value overflow */
    if(opsize==1 && val>255) {
      printf("Value too big for byte operation !\n");
      return;
    }
    if(opsize==2 && val>65535) {
      printf("Value too big for word operation !\n");
      return;
    }

    if (addr < allocated_chipmem) {
      switch(opsize) {
      case 1:
	p[addr] = val & 0xff;
	break;
      case 2:
	p[addr] = val>>8 & 0xff;
	p[addr+1] = val & 0xff;
	break;
      default:
	p[addr] = val>>24 & 0xff;
	p[addr+1] = val>>16 & 0xff;
	p[addr+2] = val>>8 & 0xff;
	p[addr+3] = val & 0xff;
	break;
      }
      printf("Wrote (");
      if(opsize==1) printf("byte");
      else if (opsize==2) printf("word");
      else printf("long");
      printf(") %d == 0x%x at %08x\n",val,val,addr);
    } else
      printf("Invalid address %08x\n",addr);
}

static void set_break(uae_u32 addr) {
  skipaddr = addr;
  do_skip = 1;
  regs.spcflags |= SPCFLAG_BRK;
}

static void history_log(void) {
#ifdef NEED_TO_DEBUG_BADLY
  history[lasthist] = regs;
  historyf[lasthist] = regflags;
#else
  history[lasthist] = m68k_getpc();
#endif
  if (++lasthist == MAX_HIST) lasthist = 0;
  if (lasthist == firsthist) {
    if (++firsthist == MAX_HIST) firsthist = 0;
  }
}

static void history_flush(void) {
  lasthist = firsthist = 0;
}

void debug (void) {
  char input[80];
  uaecptr nextpc,nxdis,nxmem;
  
  bogusframe = 1;

  history_log();

  if (do_skip && (m68k_getpc() != skipaddr) &&
      (wait_interrupt == 0 || debug_interrupt_happened == 0)) {
    regs.spcflags |= SPCFLAG_BRK;
    return;
  }

  do_skip = 0;
  debug_interrupt_happened = 0;
  wait_interrupt = 0;

  m68k_dumpstate (&nextpc);
  nxdis = nextpc; nxmem = 0;
  
  for (;;) {
    char cmd, *inptr;
    
    printf (">");
    fflush (stdout);

    while (fgets (input, 80, stdin) == NULL) {
	if (feof(stdin) || ferror(stdin)) {
	    quit_program = 1;
	    return;
	}
    }

    inptr = input;
    cmd = next_char (&inptr);
    switch (cmd) {
    case 'c': dumpcia (); dumpcustom (); break;
    case 'r': m68k_dumpstate (&nextpc); break;
    case 'M': modulesearch (); break;
    case 'C': cheatsearch (&inptr); break; 
    case 'W': writeintomem (&inptr); break;
      
    case 'R':
      {
	uae_u8 *memp;
	uae_u32 dst, len = 0;
	char *name;
	FILE *fp;
	int havesize=0;
	struct stat *st;
	uae_u32 fsize=0;
	
	if (!more_params (&inptr))
	  goto R_argh;
	
	name = inptr;
	while (*inptr != '\0' && !isspace (*inptr))
	  inptr++;
	if (!isspace (*inptr))
	  goto R_argh;
	
	*inptr = '\0';
	inptr++;
	if (!more_params (&inptr))
	  goto R_argh;
	dst = readhex (&inptr);
	
	if (more_params (&inptr)) {
	  len = readany (&inptr);
	  havesize=1;
	}
	
	if ((st=malloc(sizeof(struct stat)))) if(!stat(name,st)) fsize=st->st_size;
	if (!fsize)
	  return;
	
	if (!havesize)
	  len = fsize;
	
	if (! valid_address (dst, len)) {
	  printf ("Invalid memory block (0x %.8x - %.8x\n",dst,dst+len-1);
	  return;
	}
	memp = get_real_address (dst);
	
	fp = fopen (name, "r");
	if (fp == NULL) {
	  printf ("Couldn't open file\n");
	  break;
	}
	
	if (fread (memp, 1, len, fp) == len) {
	  printf("Read %d == 0x%x bytes from file into memory (0x %.8x - %.8x)\n",len,len,dst,dst+len-1); 
	} else {
	  printf ("Error reading file !\n");
	}
      R_fclose:
	fclose (fp);
	break;
	
      R_argh:
	printf ("R command needs more arguments!\n");
	break;
      }
      
    case 'S':
      {
	uae_u8 *memp;
	uae_u32 src, len;
	char *name;
	FILE *fp;
	
	if (!more_params (&inptr))
	  goto S_argh;
	
	name = inptr;
	while (*inptr != '\0' && !isspace (*inptr))
	  inptr++;
	if (!isspace (*inptr))
	  goto S_argh;
	
	*inptr = '\0';
	inptr++;
	if (!more_params (&inptr))
	  goto S_argh;
	src = readhex (&inptr);
	if (!more_params (&inptr))
	  goto S_argh;
	len = readany (&inptr);
	if (! valid_address (src, len)) {
	  printf ("Invalid memory block\n");
	  break;
	}
	memp = get_real_address (src);
	fp = fopen (name, "w");
	if (fp == NULL) {
	  printf ("Couldn't open file\n");
	  break;
	}
	if (fwrite (memp, 1, len, fp) == len) {
	  printf("Wrote %d == 0x%x bytes into file from memory (0x %.8x - %.8x)\n",len,len,src,src+len-1); 
	} else {
	  printf ("Error writing file\n");
	}
	fclose (fp);
	break;
	
      S_argh:
	printf ("S command needs more arguments!\n");
	break;
      }

    case 'd':
      {
	uae_u32 daddr;
	int count;

	if (more_params(&inptr)) {
	  if (!strncmp(inptr, "pc", 2)) {
	    daddr = m68k_getpc();
	    inptr += 2;
	  } else if (!strncmp(inptr, "rd", 2)) {
	    daddr = m68k_dreg(regs, inptr[2] - '0');
	    inptr += 3;
	  } else if (!strncmp(inptr, "ra", 2)) {
	    daddr = m68k_areg(regs, inptr[2] - '0');
	    inptr += 3;
	  } else {
	    daddr = readhex(&inptr);
	  }
	} else
	  daddr = nxdis;
	if (more_params(&inptr))
	  count = readany(&inptr);
	else
	  count = 10;
	m68k_disasm(daddr, &nxdis, count);
      }
      break;

    case 't':
      regs.spcflags |= SPCFLAG_BRK;
      return;

    case 'z':
      set_break(nextpc);
      return;

    case 'f':
      set_break(readhex(&inptr));
      return;

      /* Run until next interrupt */
    case 'i':
      set_break(0x13371337);
      wait_interrupt = 1;
      return;

    case 'u':
      set_break(m68k_getpc());
      return;

    case 'l':
      {
	uae_u32 traceaddr;
	if (inptr[strlen(inptr) - 1] == '\n') {
	  inptr[strlen(inptr) - 1] = 0;
	}
	if (strlen(inptr) <= 1) {
	  uade_debug_search(NULL);
	  break;
	}
	traceaddr = uade_debug_search (inptr + 1);
	if (!traceaddr)
	  break;
	set_break(traceaddr);
	return;
      }
      break;
      
    case 'q':
      uae_quit();
      debugger_active = 0;
      debugging = 0;
      return;
      
    case 'g':
      if (more_params (&inptr))
	m68k_setpc (readhex (&inptr));
      fill_prefetch_0 ();
      debugger_active = 0;
      debugging = 0;

      history_flush();

      return;

    case 'F':
      history_flush();
      break;
      
    case 'H':
      {
	int count;
	int temp;
#ifdef NEED_TO_DEBUG_BADLY
	struct regstruct save_regs = regs;
	union flagu save_flags = regflags;
#endif
	
	if (more_params(&inptr))
	  count = readany(&inptr);
	else
	  count = 10;
	if (count < 0)
	  break;
	temp = lasthist;
	while (count-- > 0 && temp != firsthist) {
	  if (temp == 0) temp = MAX_HIST-1; else temp--;
	}
	while (temp != lasthist) {
#ifdef NEED_TO_DEBUG_BADLY
	  regs = history[temp];
	  regflags = historyf[temp];
	  m68k_dumpstate(NULL);
#else
	  m68k_disasm(history[temp], NULL, 1);
#endif
	  if (++temp == MAX_HIST) temp = 0;
	}
#ifdef NEED_TO_DEBUG_BADLY
	regs = save_regs;
	regflags = save_flags;
#endif
      }
      break;
    case 'm':
      {
	uae_u32 maddr; int lines;
	if (more_params(&inptr))
	  maddr = readhex(&inptr);
	else
	  maddr = nxmem;
	if (more_params(&inptr))
	  lines = readany(&inptr);
	else
	  lines = 16;
	dumpmem(maddr, &nxmem, lines);
      }
      break;
    case 'h':
    case '?':
      {
	printf ("          HELP for UAE Debugger\n");
	printf ("         -----------------------\n\n");
	printf (" Address values are in hexadecimal, other values are in decimal by default.\n");
	printf (" Values can be written in hexadecimal with prefixes '$' or '0x'. (eg: g $100)\n\n");
	printf ("  g: <address>          Start execution at the current address or <address>\n");
	printf ("  c:                    Dump state of the CIA and custom chips\n");
	printf ("  r:                    Dump state of the CPU\n");
	printf ("  m <address> <lines>:  Memory dump starting at <address>\n");
	printf ("  d <address> <lines>:  Disassembly starting at <address>\n");
	printf ("  t:                    Step one instruction\n");
	printf ("  z:                    Step through one instruction - useful for JSR, DBRA etc\n");
	printf ("  u:                    Run until here again (same as f current pc)\n");
	printf ("  f <address>:          Step forward until PC == <address>\n");
	printf ("  i:                    Run until next interrupt\n");
	printf ("  H <count>:            Show PC history <count> instructions\n");
	printf ("  M:                    Search for *Tracker sound modules\n");
	printf ("  C <value>:            Search for values like energy or lifes in games\n");
	printf ("  W[.bwl] <addr> <val>: Write into memory with op.size (b,w,l), default is LONG\n");
	printf ("  R <file> <addr> <n>:  Read a file into Amiga memory (<n> is optional)\n");
	printf ("  S <file> <addr> <n>:  Save a block of Amiga memory\n");
	printf ("  l [<name>]:           Trace to an uade debug point or list them\n"); 
	printf ("  h,?:                  Show this help page\n");
	printf ("  q:                    Quit the emulator. You don't want to use this command.\n\n");
      }
      break;
      
    }
  }
}
