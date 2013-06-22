/*==========================================================================*
 *   MaxTrax Music Player - C header file for players                       *
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

/*==========================================================================*
 *					MeV Music Player - include file							*
 *==========================================================================*/

enum {
	MEV_ERR_NONE=0,								/* just was it says			*/
	MEV_ERR_NO_MEMORY,							/* ran out of memory		*/
	MEV_ERR_AUDIODEV							/* couldn't open audio dev	*/
};

enum {
	PERF_ALL=0,
	PERF_SCORE,
	PERF_SAMPLES
};

struct MaxTraxInfo {
	UWORD				TotalScores;			/* total number of scores	*/
	UBYTE				Volume;					/* programmatic volume		*/
	UBYTE				SyncValue;				/* last sync value			*/
	UBYTE				Flags;					/* communication flags		*/
	UBYTE				Changed;				/* set to 1 for change		*/
	void				*OpenFunc;
	void				*ReadFunc;
	void				*CloseFunc;
	UBYTE				Priority[16];			/* current priority			*/
};

#define MUSIC_PLAYING		(1<<0)				/* events being handled		*/
#define MUSIC_ADDED_NOTE	(1<<1)				/* NOTE ON did add a note	*/
#define MUSIC_SILENT		(1<<2)				/* no voices playing		*/
#define MUSIC_VELOCITY		(1<<3)				/* handle attack velocity	*/
#define MUSIC_LOOP			(1<<4)				/* loop the song			*/
#define MUSIC_PLAYNOTE		(1<<5)				/* active PlayNote			*/

#define SOUNDB_LEFT_SIDE	0
#define SOUNDB_RIGHT_SIDE	1
#define SOUNDB_LOOP			2
#define MUSTB_HAVE_SIDE		3

#define LEFT_SIDE			(1<<0)
#define RIGHT_SIDE			(1<<1)
#define SOUND_LOOP			(1<<2)
#define MUST_HAVE_SIDE		(1<<3)

#define PS_LOOPCNT(a)		((a) << 8 | SOUND_LOOP)

#define MAXTRAX_VBLANKFREQ	(64+1)

LONG InitMusic(void);
LONG NewInitMusic(LONG scores);
LONG InitMusicTagList(LONG scores,struct TagItem *ti);
void FreeMusic(void);
LONG OpenMusic(void);
void CloseMusic(void);
void SetTempo(ULONG tempo);
void SetAudioFilter(ULONG state);
void SyncSong(struct Task *task,ULONG signal);
BOOL PlaySong(ULONG which);
BOOL LoopSong(ULONG which);
void StopSong(void);
BOOL SelectScore(ULONG which);
LONG PlaySound(void *sample,ULONG length,ULONG volume,ULONG period,ULONG side);
void StopSound(LONG cookie);
LONG CheckSound(LONG cookie);
LONG PlayNote(ULONG notenum,ULONG patchnum,ULONG duration,ULONG volume,ULONG pan);
LONG CheckNote(LONG cookie);
BOOL LoadPerf(char *name,LONG mode,...);
void UnloadPerf(LONG mode);
