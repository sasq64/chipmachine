/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Define YM types for multi-platform compilation.
	Change that file depending of your platform. Please respect the right size
	for each type.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

	This file is part of ST-Sound

	ST-Sound is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	ST-Sound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with ST-Sound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/

#ifndef __YMTYPES__
#define __YMTYPES__

#ifdef _MSC_VER

typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else
#include <stdint.h>
#endif


#define YM_INTEGER_ONLY

//-----------------------------------------------------------
// Platform specific stuff
//-----------------------------------------------------------

#ifdef YM_INTEGER_ONLY
typedef	int64_t		yms64;
#else
typedef	float		ymfloat;
#endif

typedef	int8_t		yms8;			//  8 bits signed integer
typedef	int16_t		yms16;			// 16 bits signed integer
typedef	int32_t		yms32;			// 32 bits signed integer

typedef	uint8_t		ymu8;			//  8 bits unsigned integer
typedef	uint16_t	ymu16;			// 16 bits unsigned integer
typedef	uint32_t	ymu32;			// 32 bits unsigned integer

typedef	yms32		ymint;			// Native "int" for speed purpose. StSound suppose int is signed and at least 32bits. If not, change it to match to yms32
typedef	char		ymchar;			// 8 bits char character (used for null terminated strings)


#ifndef NULL
#define NULL	(0L)
#endif

//-----------------------------------------------------------
// Multi-platform
//-----------------------------------------------------------
typedef		int					ymbool;			// boolean ( theorically nothing is assumed for its size in StSound,so keep using int)
typedef		yms16				ymsample;		// StSound emulator render mono 16bits signed PCM samples

#define		YMFALSE				(0)
#define		YMTRUE				(!YMFALSE)

#endif

