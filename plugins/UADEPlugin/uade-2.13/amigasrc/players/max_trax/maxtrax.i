*==========================================================================*
*   MaxTrax Music Player - assembler include file for players              *
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
*	02111-1307 USA.                                                        *
*                                                                          *
*   Contact information:                                                   *
*   The Wyrmkeep Entertainment Co.                                         *
*   Attn: Joe Pearce                                                       *
*   P. O. Box 1585                                                         *
*   Costa Mesa, CA 92628-1585                                              *
*   www.wyrmkeep.com                                                       *
*==========================================================================*

*===========================================================================*
*					MeV Music Player - include file							*
*===========================================================================*

MEV_ERR_NONE		equ		0					; just was it says			
MEV_ERR_NO_MEMORY	equ		1					; ran out of memory		
MEV_ERR_AUDIODEV	equ		2					; couldn't open audio dev	

PERF_ALL			equ		0
PERF_SCORE			equ		1
PERF_SAMPLES		equ		2
PERF_PARTSAMPLES	equ		3

		STRUCTURE	MaxTraxInfo,0
			WORD	mxtx_TotalScores			/* total number of scores	*/
			BYTE	mxtx_Volume					/* programmatic volume		*/
			BYTE	mxtx_SyncValue				/* last sync value			*/
			BYTE	mxtx_Flags					/* communication flags		*/
			BYTE	mxtx_Changed				/* set to 1 for change		*/
			APTR	mxtx_OpenFunc
			APTR	mxtx_ReadFunc
			APTR	mxtx_CloseFunc
			STRUCT	mxtx_Priority,16			; current priorities
			LABEL	mxtx_sizeof

MUSIC_PLAYING		equ		(1<<0)				; events being handled		
MUSIC_ADDED_NOTE	equ		(1<<1)				; NOTE ON did add a note	
MUSIC_SILENT		equ		(1<<2)				; no voices playing		
MUSIC_VELOCITY		equ		(1<<3)				; handle attack velocity	
MUSIC_LOOP			equ		(1<<4)				; loop the song			
MUSIC_PLAYNOTE		equ		(1<<5)				; active PlayNote

MUSICB_PLAYING		equ		0
MUSICB_ADDED_NOTE	equ		1
MUSICB_SILENT		equ		2
MUSICB_VELOCITY		equ		3
MUSICB_LOOP			equ		4
MUSICB_PLAYNOTE		equ		5

SOUNDB_LEFT_SIDE	equ		0
SOUNDB_RIGHT_SIDE	equ		1
SOUNDB_LOOP			equ		2
MUSTB_HAVE_SIDE		equ		3

SOUND_LEFT_SIDE		equ		$01
SOUND_RIGHT_SIDE	equ		$02
SOUND_LOOP			equ		$04
MUST_HAVE_SIDE		equ		$08

MAXTRAX_VBLANKFREQ	equ		(64+1)

