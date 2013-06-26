/*
 *                     emu68 - 68000 IO manager
 *
 *             Copyright (C) 2001-2011 Benjamin Gerard
 *
 *           <benjihan -4t- users.sourceforge -d0t- net>
 *
 *              Time-stamp: <2011-08-23 02:49:53 ben>
 *
 * This  program is  free  software: you  can  redistribute it  and/or
 * modify  it under the  terms of  the GNU  General Public  License as
 * published by the Free Software  Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 *
 * You should have  received a copy of the  GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ioplug68.h"
#include "mem68.h"

static void do_io_unplug(emu68_t * const emu68, io68_t * const io)
{
  /* Remove memory acces handler */
  emu68_mem_reset_area(emu68, (io->addr_lo>>8)&255);
}

/* Unplug all IO */
static void _ioplug_unplug_all(emu68_t * const emu68, const int destroy)
{
  io68_t *next = emu68->iohead;
  while (next) {
    io68_t *io = next;
    next = io->next;
    do_io_unplug(emu68, io);
    if (destroy && io->destroy) {
      io->destroy(io);
    }
  }
  emu68->iohead = 0;
  emu68->nio    = 0;
}


/* Unplug all IO */
void emu68_ioplug_unplug_all(emu68_t * const emu68)
{
  if (emu68)
    _ioplug_unplug_all(emu68, 0);
}

/* Unplug and destroy all IO */
void emu68_ioplug_destroy_all(emu68_t * const emu68)
{
  if (emu68)
    _ioplug_unplug_all(emu68, 1);
}

/*  Unplug an IO :
 *  - remove from IO-list
 *  - remove memory access handler
 *
 *  return 0 if IO successfully unplugged
 */
int emu68_ioplug_unplug(emu68_t * const emu68, io68_t *this_io)
{
  io68_t *io,**pio;

  if (emu68) {
    if (!this_io) {
      return 0;
    }
    for (io = emu68->iohead, pio = &emu68->iohead;
         io;
         pio=&io->next, io=io->next) {
      /* Find it ??? */
      if (io==this_io) {
        *pio = io->next;
        --emu68->nio;
        do_io_unplug(emu68, io);
        return 0;
      }
    }
  }
  return -1;
}

/*  Plug new IO :
 *  - add to io list
 *  - add new memory access handler
 */
void emu68_ioplug(emu68_t * const emu68, io68_t * const io)
{
  if (emu68 && io) {
    int i;
    io->next = emu68->iohead;
    emu68->iohead = io;
    io->emu68 = emu68;
    ++emu68->nio;
    for (i=(u8)(io->addr_lo>>8); i<=(u8)(io->addr_hi>>8); ++i) {
      emu68->mapped_io[i] = io;
    }
  }
}
