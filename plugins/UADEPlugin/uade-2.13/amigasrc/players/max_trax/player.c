/*==========================================================================*
 *   MaxTrax Player Example                                                 *
 *   Copyright 1991 Talin (David Joiner) & Joe Pearce                       *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *    Library General Public License for more details.                      *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with this library; if not, write to the Free             *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA               *
 *   02111-1307 USA.                                                        *
 *                                                                          *
 *   Contact information:                                                   *
 *   The Wyrmkeep Entertainment Co.                                         *
 *   Attn: Joe Pearce                                                       *
 *   P. O. Box 1585                                                         *
 *   Costa Mesa, CA 92628-1585                                              *
 *   www.wyrmkeep.com                                                       *
 *==========================================================================*/

#ifndef EXEC_TYPES_H
#include <exec/libraries.h>
#include <libraries/dosextens.h>
#include <clib/all_protos.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "maxtrax.h"

#ifndef AZTEC_C
#define atexit	onexit
#endif

extern struct MaxTraxInfo	maxtrax;		/* global interface to driver	*/

struct Library				*GfxBase;		/* need by driver				*/

int closeall(void)
{
	FreeMusic();
	if (GfxBase) CloseLibrary(GfxBase);
}

doexit(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}

void main(int argc,char *argv[])
{	int		score = 0,
			song = 0;

	atexit(closeall);

	if ( !(GfxBase = OpenLibrary("graphics.library",33L)) ) exit(20);

	if (argc < 2) doexit("USAGE: player songfile <score> <mark>");

	if (argc > 2) score = atol(argv[2]);
	if (argc > 3) song = atol(argv[3]);

	if ( InitMusic() ) doexit("Can't init driver!");

	if ( !LoadPerf(argv[1],PERF_ALL) ) doexit("Can't load song file!");

	if (!SelectScore(score)) doexit("Score # to high!");
	if (!PlaySong(song)) doexit("Couldn't start song!");

	SetSignal(0,SIGBREAKF_CTRL_C);

	while ((maxtrax.Flags & MUSIC_PLAYING) || !(maxtrax.Flags & MUSIC_SILENT))
	{
		if (SetSignal(0,0) & SIGBREAKF_CTRL_C)
		{	WORD	i;

			for (i=63;i>=0;i--)
			{
				maxtrax.Volume = i;
				maxtrax.Changed = 1;
				WaitTOF();
			}

			SetSignal(0,SIGBREAKF_CTRL_C);
			break;
		}

		WaitTOF();
	}

	doexit("Done.");
}
