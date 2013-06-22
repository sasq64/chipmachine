*==========================================================================*
*   MaxTrax Music Player - audio device handler include file               *
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

* various features enabled in the driver

HAS_MODULATION		equ		0
HAS_MICROTONAL		equ		0
HAS_ALLMIDI			equ		0
HAS_FULLCHANVOL		equ		0
IN_MUSICX			equ		0
FASTSOUND			equ		0

* various limits on this version of the driver (all power of two)

NUM_SAMPLES			equ		64
NUM_PATCHES			equ		64
NUM_VOICES			equ		4
NUM_CHANNELS		equ		16
NUM_SCORES			equ		8
NUM_REPEATS			equ		4

NUM_PATCHES_MASK	equ		(64-1)

PERIOD_THRESH		equ		288					; sample period threshold
PERIOD_ENTRIES		equ		((288*3)+11)		; size of period table

NTSC_CLOCKS			equ		3579546				; Color clock values
PAL_CLOCKS			equ		3546895

K_VALUE				equ		$09fd77
PREF_PERIOD			equ		$08fd77
PERIOD_LIMIT		equ		$06f73d

MEV_ERR_NONE		equ		0					; just was it says			
MEV_ERR_NO_MEMORY	equ		1					; ran out of memory		
MEV_ERR_AUDIODEV	equ		2					; couldn't open audio dev	

		STRUCTURE	StopEvent,0
			BYTE	sev_Command
			BYTE	sev_Data
			LONG	sev_StopTime
			LABEL	sev_sizeof

		STRUCTURE	CookedEvent,0
			BYTE	cev_Command
			BYTE	cev_Data
			WORD	cev_StartTime
			WORD	cev_StopTime
			LABEL	cev_sizeof

		STRUCTURE	EnvelopeData,0
			WORD	env_Duration				; duration in milliseconds	
			WORD	env_Volume					; volume of envelope		
			LABEL	env_sizeof

		STRUCTURE	SampleData,0
			APTR	samp_NextSample
			APTR	samp_Waveform
			LONG	samp_AttackSize
			LONG	samp_SustainSize
			LABEL	samp_sizeof

		STRUCTURE	PatchData,0
			APTR	patch_Sample				; Amiga sample data		
			APTR	patch_Attack				; array of env. segments	
			APTR	patch_Release				; array of env. segments	
			WORD	patch_AttackCount			; number of attack env.	
			WORD	patch_ReleaseCount			; number of release env.	
			WORD	patch_Volume				; sample volume 			
			WORD	patch_Tune					; sample tuning			
			BYTE	patch_Number				; self-identifing			
			BYTE	patch_pad
			LABEL	patch_sizeof

		STRUCTURE	VoiceData,0
			APTR	voice_Channel
			APTR	voice_Patch					; patch playing on channel	
			APTR	voice_Envelope				; in patch data 			
			LONG	voice_UniqueID				; sampled sound playing	
			LONG	voice_LastTicks				; last total tick value	
			LONG	voice_TicksLeft				; left in envelope	segment	
			LONG	voice_PortaTicks			; portamento timing		
			LONG	voice_IncrVolume			; incremental volume		
			LONG	voice_PeriodOffset
		ifne FASTSOUND
			APTR	voice_CurFastIOB			; current fast iob playing
			APTR	voice_NextFastIOB			; next fast iob to play
			APTR	voice_FastBuffer			; pointer to buffer area
		endc
			WORD	voice_EnvelopeLeft			; segments left in env.	
			WORD	voice_NoteVolume			; note attack volume		
			WORD	voice_BaseVolume			; base volume of segment	
			WORD	voice_LastPeriod			; last calculated period	
			BYTE	voice_BaseNote				; note we are playing		
			BYTE	voice_EndNote				; portamento note			
			BYTE	voice_Number				; self-identifying			
			BYTE	voice_Link					; stereo sampled sound		
			BYTE	voice_Priority				; priority of this voice	
			BYTE	voice_Status				; envelope status			
			BYTE	voice_Flags
			BYTE	voice_LastVolume
			LABEL	voice_sizeof

voice_FastSizeLeft	equ		voice_PeriodOffset
voice_FastSample	equ		voice_Envelope
voice_FastVolume	equ		voice_BaseVolume
voice_FastPeriod	equ		voice_LastPeriod
voice_FastNextBuf	equ		voice_EndNote

VOICE_STOLEN		equ		(1<<0)				; voice was stolen			
VOICE_PORTAMENTO	equ		(1<<1)				; portamento playing		
VOICE_DAMPER		equ		(1<<2)				; note still on via damper 
VOICE_BLOCKED		equ		(1<<3)				; sample player using this	
VOICE_RECALC		equ		(1<<4)

VOICEB_STOLEN		equ		0
VOICEB_PORTAMENTO	equ		1
VOICEB_DAMPER		equ		2
VOICEB_BLOCKED		equ		3
VOICEB_RECALC		equ		4

ENV_FREE			equ		0			; voice not in use					
ENV_HALT			equ		1			; don't just kill the sample		
ENV_DECAY			equ		2			; in release envelope section		
ENV_RELEASE			equ		3			; enter release envelope section	
ENV_SUSTAIN			equ		4			; finished attack envelopes		
ENV_ATTACK			equ		5			; in attack envelope section		
ENV_START			equ		6			; hasn't had envelope processing	

		STRUCTURE	ChannelData,0
			APTR	chan_Patch					; patch on this channel	
			WORD	chan_RPN					; registered parameter #	
	ifne HAS_MODULATION
			WORD	chan_Modulation				; modulation level			
			WORD	chan_ModulationTime			; modulation time (msec)	
	endc
	ifne HAS_MICROTONAL
			WORD	chan_Microtonal				; -1 or next utonal adjust 
	endc
			WORD	chan_PortamentoTime			; portamento time (msec)	
			WORD	chan_PitchBend				; current pitch bend value	
			WORD	chan_RealBend				; real pitch bend value	
			BYTE	chan_PitchBendRange			; pitch bend sensitivity	
			BYTE	chan_Volume					; volume					
			BYTE	chan_VoicesActive			; count of voices active	
			BYTE	chan_Number					; self identifing			
			BYTE	chan_Flags					; pan, etc.
			BYTE	chan_LastNote				; last note released on channel
			BYTE	chan_Program				; used by Music-X
			BYTE	chan_pad
			LABEL	chan_sizeof

CHAN_PAN			equ		(1<<0)				; 0 = left, 1 = right		
CHAN_MONO			equ		(1<<1)				; MONO mode, not POLY		
CHAN_PORTAMENTO		equ		(1<<2)				; portamento active		
CHAN_DAMPER			equ		(1<<3)				; "damper pedal" down		
CHAN_MICROTONAL		equ		(1<<4)				; use microtonal table
CHAN_MODTYPE		equ		(1<<5)				; period or volume modulation
CHAN_ALTERED		equ		(1<<7)				; channel params altered	

CHANB_PAN			equ		0
CHANB_MONO			equ		1
CHANB_PORTAMENTO	equ		2
CHANB_DAMPER		equ		3
CHANB_MICROTONAL	equ		4
CHANB_MODTYPE		equ		5
CHANB_ALTERED		equ		7

NO_BEND				equ		(64<<7)				; centered bend wheel val	
MAX_BEND_RANGE		equ		24					; 24 semitones				

		STRUCTURE	GlobalData,0
			LONG	glob_UniqueID				; for sampled sounds		
			LONG	glob_ColorClocks			; system color clock value	
			LONG	glob_Ticks					; current tick value		
			LONG	glob_TickUnit				; tick unit				
			LONG	glob_FrameUnit				; in fixed-point ms		
			LONG	glob_TempoTime				; time of tempo change		
			LONG	glob_TempoTicks				; tempo change counter		
	ifne HAS_MODULATION
			LONG	glob_SineValue				; for modulation affects	
	endc
			APTR	glob_SyncTask				; video sync values		
			LONG	glob_SyncSig
			LONG	glob_CurrentTime			; current event time		
			APTR	glob_CurrentScore
			APTR	glob_Current				; current event			
			STRUCT	glob_RepeatPoint,4*NUM_REPEATS	; repeat points		
			STRUCT	glob_NoteOff,sev_sizeof*NUM_VOICES	; stop event bufs
			WORD	glob_Frequency				; driver frequency			
			WORD	glob_TotalScores			; total number of scores	
			WORD	glob_Volume					; programmatic volume		
			WORD	glob_Filter					; global start filter		
			WORD	glob_Tempo					; global start tempo		
			WORD	glob_CurrentTempo			; current tempo			
			WORD	glob_StartTempo				; start tempo if continuous
			WORD	glob_DeltaTempo				; delta tempo if continuous
			STRUCT	glob_RepeatCount,NUM_REPEATS	; times to repeat
			BYTE	glob_RepeatTotal			; total repeats currently	
			BYTE	glob_VoicesActive			; number of voice used		
			BYTE	glob_LastVoice				; last voice affected		
			BYTE	glob_Flags
			BYTE	glob_SaveFilter
	ifne HAS_ALLMIDI
			BYTE	glob_RunningStatus			; running status (if used)	
	else
			BYTE	glob_pad
	endc
			LABEL	glob_sizeof

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

MUSIC_PRI_SCORE		equ		0
MUSIC_PRI_NOTE		equ		1
MUSIC_PRI_SOUND		equ		2

		STRUCTURE	MaxTraxInfo,0
			WORD	mxtx_TotalScores			; total number of scores
			BYTE	mxtx_Volume					; programmatic volume
			BYTE	mxtx_SyncValue				; last sync value
			BYTE	mxtx_Flags					; communication flags
			BYTE	mxtx_Changed				; set to 1 for change
			APTR	mxtx_OpenFunc
			APTR	mxtx_ReadFunc
			APTR	mxtx_CloseFunc
			STRUCT	mxtx_Priority,16			; current priorities
			LABEL	mxtx_sizeof

* communication flags are PLAYING, SILENT, VELOCITY, LOOP

* Values between 0 and 127 are NOTE events. { NOTE #, VOL | CHAN, STOP }
COMMAND_TEMPO		equ		$80			; { COMMAND, TEMPO, N/A }			
COMMAND_SPECIAL		equ		$a0			; { COMMAND, CHAN, SPEC # | VAL }	
COMMAND_CONTROL		equ		$b0			; { COMMAND, CHAN, CTRL # | VAL }	
COMMAND_PROGRAM		equ		$c0			; { COMMAND, CHANNEL, PROG # }		
COMMAND_BEND		equ		$e0			; { COMMAND, CHANNEL, BEND VALUE }	
COMMAND_SYSEX		equ		$f0			; { COMMAND, TYPE, SIZE }
COMMAND_REALTIME	equ		$f8			; { COMMAND, REALTIME, N/A }		
COMMAND_END			equ		$ff			; { COMMAND, N/A, N/A }			

SPECIAL_MARK		equ		$00			; song marker (no data)		
SPECIAL_SYNC		equ		$01			; send sync signal	(data)		
SPECIAL_BEGINREP	equ		$02			; begin repeat section (times) 
SPECIAL_ENDREP		equ		$03			; end repeat section (no data)	

SYSEX_MICROTONAL	equ		0

		STRUCTURE	ScoreData,0
			APTR	score_Data
			LONG	score_NumEvents
			LABEL	score_sizeof

PERF_ALL			equ		0
PERF_SCORE			equ		1
PERF_SAMPLES		equ		2
PERF_PARTSAMPLES	equ		3

		STRUCTURE	SoundBlock,0
			APTR	sblk_Data
			LONG	sblk_Length
			WORD	sblk_Volume
			WORD	sblk_Period
			WORD	sblk_Pan
			LABEL	sblk_sizeof

		STRUCTURE	NoteBlock,0
			WORD	nblk_Note
			WORD	nblk_Patch
			WORD	nblk_Duration
			WORD	nblk_Volume
			WORD	nblk_Pan
			LABEL	nblk_sizeof

EXTRA_NONE			equ		0
EXTRA_NOTE			equ		1
EXTRA_PLAYSOUND		equ		2
EXTRA_STOPSOUND		equ		3
EXTRA_CHECKSOUND	equ		4
EXTRA_ADVANCE		equ		5
EXTRA_CHECKNOTE		equ		6

SOUND_LEFT_SIDE		equ		$01
SOUND_RIGHT_SIDE	equ		$02
SOUND_STEREO		equ		$03
SOUND_LOOP			equ		$04
MUST_HAVE_SIDE		equ		$08

SOUNDB_LEFT_SIDE	equ		0
SOUNDB_RIGHT_SIDE	equ		1
SOUNDB_LOOP			equ		2
MUSTB_HAVE_SIDE		equ		3

		STRUCTURE	DiskSample,0
			WORD	dsamp_Number
			WORD	dsamp_Tune
			WORD	dsamp_Volume
			WORD	dsamp_Octaves
			LONG	dsamp_AttackLength
			LONG	dsamp_SustainLength
			WORD	dsamp_AttackCount
			WORD	dsamp_ReleaseCount
			LABEL	dsamp_sizeof

DSAMPF_FILTER		equ		(1<<0)
DSAMPF_VELOCITY		equ		(1<<1)
DSAMPF_MICROTONAL	equ		(1<<15)

MAXTRAX_VBLANKFREQ	equ		(64+1)
