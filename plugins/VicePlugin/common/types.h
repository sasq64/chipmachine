/*
 * types.h - Type definitions for VICE.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_TYPES_H
#define VICE_TYPES_H

#include "vice.h"

#define BYTE unsigned char

typedef signed char SIGNED_CHAR;

#if SIZEOF_UNSIGNED_SHORT == 2
typedef unsigned short WORD;
typedef signed short SWORD;
#else
#error Cannot find a proper 16-bit type!
#endif

#if SIZEOF_UNSIGNED_INT == 4

//#ifndef WIN32
typedef unsigned int DWORD;
//#endif

typedef signed int SDWORD;
#elif SIZEOF_UNSIGNED_LONG == 4

//#ifndef WIN32
typedef unsigned long DWORD;
//#endif

typedef signed long SDWORD;
#else
#error Cannot find a proper 32-bit type!
#endif

typedef DWORD CLOCK;
/* Maximum value of a CLOCK.  */
#define CLOCK_MAX (~((CLOCK)0))


#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))

#endif
