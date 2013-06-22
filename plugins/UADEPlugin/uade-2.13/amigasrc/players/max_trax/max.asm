*==========================================================================*
*   MaxTrax Music Player - audio device handler 			               *
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

*	To do: Use volume of envelope to break ties when picking voices.

			include 'exec/types.i'
			include 'exec/execbase.i'
			include 'exec/ports.i'
			include 'exec/lists.i'
			include 'exec/memory.i'
                        include 'LVO3.0/dos_lib.i'
                        include 'LVO3.0/exec_lib.i'
			include 'devices/audio.i'
			include 'graphics/gfxbase.i'
			include 'hardware/intbits.i'
			include 'hardware/custom.i'
			include 'dos/dosextens.i'

			include 'driver.i'

SEE_EXTERNAL	equ		1
DEBUG			equ		0
C_ENTRY			equ		0
DO_DMACHECK		equ		0

JSRLIB		macro
;			xref	_LVO\1
			jsr		_LVO\1(a6)
			endm

JSRDEV		macro
;			xref	_do_audio
;			jsr		_do_audio
			jsr		DEV_\1(a6)
			endm

DEFX		macro
		ifne SEE_EXTERNAL
			xdef	\1
		endc
			endm

DMACHECK	macro
		ifne DO_DMACHECK
			bsr		dma_check
		endc
			endm

KDEBUG		macro
			pea		\1
			bsr		knum
			addq.w	#4,sp
			endm

COLOR0		macro
;			move.w	#\1,$00dff180
			endm

		; multiplies word values, result is a word

SCALE		macro

		ifeq	(\1&(\1-1))

		ifeq	\1-2
			add.w	\2,\2
		endc

		ifeq	\1-4
			add.w	\2,\2
			add.w	\2,\2
		endc

		ifeq	\1-8
			lsl.w	#3,\2
		endc

		ifeq	\1-16
			lsl.w	#4,\2
		endc

		ifeq	\1-32
			lsl.w	#5,\2
		endc

		else
			mulu.w	#\1,\2
		endc

			endm

*==========================================================================*
*
*	external references
*
*==========================================================================*

			xref	_SysBase
			xref	_GfxBase
			xref	_DOSBase

*==========================================================================*
*
*	global data area
*
*==========================================================================*
	
			section	__MERGED,DATA

			xdef	_maxtrax
	ifne	1
			DEFX	_globaldata
			DEFX	_patch
			DEFX	_channel
			DEFX	_voice
	else
			xdef	_globaldata
			xdef	_patch
			xdef	_channel
			xdef	_voice
	endc
			DEFX	_xchannel
			DEFX	_extra_op
			DEFX	_extra_data

_patch		ds.b	NUM_PATCHES*patch_sizeof
_voice		ds.b	NUM_VOICES*voice_sizeof
_channel	ds.b	NUM_CHANNELS*chan_sizeof
_xchannel	ds.b	chan_sizeof
_maxtrax	ds.b	mxtx_sizeof
_globaldata	ds.b	glob_sizeof
_extra_op	dc.w	0
_extra_data	dc.l	0

_scoreptr	dc.l	0
_scoremax	dc.w	0

		ifne HAS_MICROTONAL
			DEFX	_microtonal
_microtonal	ds.w	128
		endc

vblank_name	dc.b	'MaxTrax_VBlank',0,0
music_name	dc.b	'MT_Music',0,0
extra_name	dc.b	'MT_Extra',0,0

			DEFX	_vblank_server
			DEFX	_music_server
			DEFX	_extra_server
_vblank_server
			dc.l	0,0
			dc.b	5
			dc.b	NT_INTERRUPT
			dc.l	vblank_name
			dc.l	0
			dc.l	MusicVBlank
_music_server
			dc.l	0,0
			dc.b	-32
			dc.b	NT_INTERRUPT
			dc.l	music_name
			dc.l	0
			dc.l	MusicServer
_extra_server
			dc.l	0,0
			dc.b	-32
			dc.b	NT_INTERRUPT
			dc.l	extra_name
			dc.l	0
			dc.l	ExtraServer

			dc.w	0

			DEFX	_audio_play
			DEFX	_audio_ctrl
			DEFX	_audio_stop
			DEFX	_audio_env
			DEFX	_play_port
			DEFX	_temp_port
			DEFX	_AudioDevice
_audio_play
			dc.l	0
_audio_ctrl
			dc.l	0
_audio_stop
			dc.l	0
_audio_env
			dc.l	0
_play_port
			dc.l	0
_temp_port
			dc.l	0
_AudioDevice
			dc.l	0

			DEFX	_asample
_asample
			ds.l	NUM_SAMPLES

* 30 - serial number for LGPL release

			dc.b	'>'
			dc.b	0,0,3,0
			dc.b	'<'

			section code,code

* the following assembly langauge file is shared between MaxTrax & Music-X

			include 'shared.asm'

*==========================================================================*
*
*	InitMusic - initialize the music player, does not open audio device
*
*	WORD InitMusic(void)
*	WORD NewInitMusic(LONG scores)
*	WORD InitMusicTagList(LONG scores,struct TagItem *ti)
*
*==========================================================================*

AUDIO_MEM_SIZE		equ		((3*NUM_VOICES+3)*ioa_SIZEOF+2*MP_SIZE)

			xdef	_InitMusic
_InitMusic
			xdef	InitMusic
InitMusic
			move.l	#NUM_SCORES,d0
			suba.l	a0,a0
			bra.s	InitMusicTagList

			xdef	_NewInitMusic
_NewInitMusic
			move.l	4(sp),d0
			xdef	NewInitMusic
NewInitMusic
			suba.l	a0,a0
			bra.s	InitMusicTagList

			xdef	_InitMusicTagList
_InitMusicTagList
			move.l	4(sp),d0
			move.l	8(sp),a0
			xdef	InitMusicTagList
InitMusicTagList
			movem.l	a2/a6,-(sp)
			move.l	a0,a2								; save taglist (if any)

			tst.l	_globaldata+glob_FrameUnit			; already inited?
			bne		.l99									; tell them OK

			move.w	d0,_scoremax						; maximum scores
			mulu.w	#score_sizeof,d0					; allocate score buffers
			add.l	#AUDIO_MEM_SIZE,d0					; and audio blocks

			move.l	#MEMF_PUBLIC|MEMF_CLEAR,d1
			move.l	_SysBase,a6
			JSRLIB	AllocMem
			lea		_audio_play,a0
			move.l	d0,(a0)								; audio_play buffers
			beq		.l98									; if zero, error

			move.l	d0,a1

			add.l	#AUDIO_MEM_SIZE,d0
			move.l	d0,_scoreptr

			lea		3*NUM_VOICES*ioa_SIZEOF(a1),a1		; init audio_ctrl ptr
			move.l	a1,4(a0)
			lea		ioa_SIZEOF(a1),a1					; init audio_stop ptr
			move.l	a1,8(a0)
			lea		ioa_SIZEOF(a1),a1					; init audio_env ptr
			move.l	a1,12(a0)
			lea		ioa_SIZEOF(a1),a1					; init play_port ptr
			move.b	#NT_MSGPORT,LN_TYPE(a1)
			move.b	#PA_IGNORE,MP_FLAGS(a1)
			move.l	a1,16(a0)
			lea		MP_SIZE(a1),a1						; init temp_port ptr
			move.b	#NT_MSGPORT,LN_TYPE(a1)
			move.b	#PA_IGNORE,MP_FLAGS(a1)
			move.l	a1,20(a0)

* init various global variables

			moveq	#0,d1
			move.b	VBlankFrequency(a6),d1				; get VBlankFreq

			move.l	a2,d0								; tag list?
			beq.s	.l10
			move.l	a2,a0
			moveq	#MAXTRAX_VBLANKFREQ,d0
			bsr		FindTag
			tst.l	d0
			beq.s	.l10
			move.l	d0,a0
			move.w	6(a0),d1							; get user VBlankFreq

.l10			lea		_globaldata,a0
			move.w	d1,glob_Frequency(a0)				; save VBlankFreq
			move.l	#(1000<<8),d0						; calc FrameUnit
			move.l	a0,-(sp)
			jsr		divu
			move.l	(sp)+,a0
			move.l	d0,glob_FrameUnit(a0)

			move.l	_scoreptr,glob_CurrentScore(a0)		; init current score

			moveq	#64,d0
			move.w	d0,glob_Volume(a0)					; Volume = 64
			move.b	d0,_maxtrax+mxtx_Volume
			moveq	#0,d0
			move.l	d0,glob_TempoTime(a0)				; TempoTime = 0
			move.l	d0,glob_UniqueID(a0)				; UniqueID = 0
			move.b	d0,glob_Flags(a0)					; Flags = 0

* set ColorClocks

			cmp.w	#36,LIB_VERSION(a6)					; which version of OS?
			bpl.s	.l1

			move.l	#NTSC_CLOCKS,glob_ColorClocks(a0)	; KS1.3, check GfxBase
			move.l	_GfxBase,a1
			btst.b	#PALn,gb_DisplayFlags+1(a1)
			beq.s	.l2
			move.l	#PAL_CLOCKS,glob_ColorClocks(a0)
			beq.s	.l2

.l1			move.l	ex_EClockFrequency(a6),d0			; KS2.0, check ExecBase
			move.l	d0,d1
			add.l	d1,d1
			add.l	d1,d1
			add.l	d1,d0
			move.l	d0,glob_ColorClocks(a0)

.l2			lea		_xchannel,a0
			move.b	#16,chan_Number(a0)
			move.b	#0,chan_Flags(a0)
			move.b	#0,chan_VoicesActive(a0)

			bsr		ResetChannel						; reset xchannel

			moveq	#INTB_VERTB,d0						; add vblank server
			lea		_vblank_server,a1
			move.l	_SysBase,a6
			JSRLIB	AddIntServer

			lea		StdOpenFunc,a1
			move.l	a1,_maxtrax+mxtx_OpenFunc
			lea		StdReadFunc,a1
			move.l	a1,_maxtrax+mxtx_ReadFunc
			lea		StdCloseFunc,a1
			move.l	a1,_maxtrax+mxtx_CloseFunc

.l99			moveq	#MEV_ERR_NONE,d0					; no problem
			movem.l	(sp)+,a2/a6
			rts

.l98			jsr		FreeMusic
			moveq	#MEV_ERR_NO_MEMORY,d0				; problem!
			movem.l	(sp)+,a2/a6
			rts

*==========================================================================*
*
*	FreeMusic - close down music player, free all resources
*
*	void FreeMusic(void)
*
*==========================================================================*

			xdef	_FreeMusic
_FreeMusic
			xdef	FreeMusic
FreeMusic
			move.l	a6,-(sp)

			jsr		CloseMusic

			move.l	#PERF_ALL,d0
			jsr		UnloadPerf

			move.l	_SysBase,a6

* remove vblank server

			tst.l	_globaldata+glob_FrameUnit			; interrupt active?
			beq.s	.l1									; no, skip

			moveq	#INTB_VERTB,d0						; add vblank server
			lea		_vblank_server,a1
			JSRLIB	RemIntServer

			clr.l	_globaldata+glob_FrameUnit

.l1			move.l	_audio_play,d0						; audio blocks allocated?
			beq.s	.l2									; no, skip

			move.l	d0,a1								; free score buffers
			moveq	#0,d0								; & audio blocks
			move.w	_scoremax,d0
			mulu.w	#score_sizeof,d0
			add.l	#AUDIO_MEM_SIZE,d0

			JSRLIB	FreeMem

			clr.l	_audio_play

.l2			move.l	(sp)+,a6
			rts

*==========================================================================*
*
*	OpenMusic - allocate audio resources
*
*	WORD OpenMusic(void)
*
*==========================================================================*

audio_name	dc.b	'audio.device',0,0

			xdef	_OpenMusic
_OpenMusic
OpenMusic
			movem.l	d2/a2/a3/a6,-(sp)

			tst.l	_AudioDevice						; if music already open
			bne		.l99									;	done...

			jsr		GetAudioFilter						; save filter state
			move.b	d0,_globaldata+glob_SaveFilter
			bclr.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music off
			bclr.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music off
			clr.b	_globaldata+glob_VoicesActive

* init channels

			moveq	#NUM_CHANNELS-1,d2					; init all channels
			move.l	#_channel+(NUM_CHANNELS-1)*chan_sizeof,a2
			move.l	#_patch+(NUM_CHANNELS-1)*patch_sizeof,a3
.l1			move.l	a3,chan_Patch(a2)					; set patch number
			lea		-patch_sizeof(a3),a3
			move.b	d2,chan_Number(a2)					; set channel number
			clr.w	chan_RPN(a2)						; set RPN to 0
			move.l	a2,a0
			bsr		ResetChannel						; reset channel
			lea		-chan_sizeof(a2),a2
			dbra	d2,.l1								; loop

* init patches

			moveq	#NUM_PATCHES-1,d2					; init all patches
			move.l	#_patch+(NUM_PATCHES-1)*patch_sizeof,a3
.l4			move.b	d2,patch_Number(a3)					; set patch number
			lea		-patch_sizeof(a3),a3
			dbra	d2,.l4								; loop

* init voices

			moveq	#NUM_VOICES-1,d2
			move.l	#_voice+(NUM_VOICES-1)*voice_sizeof,a2
.l3			clr.l	voice_Channel(a2)					; no channel
			clr.b	voice_Status(a2)					; ENV_FREE
			move.b	d2,voice_Number(a2)					; voice number
			lea		-voice_sizeof(a2),a2
			dbra	d2,.l3								; loop

* initialize message ports

			move.l	_play_port,a1
			lea		MP_MSGLIST(a1),a1
			NEWLIST a1
			move.l	_temp_port,a1
			lea		MP_MSGLIST(a1),a1
			NEWLIST a1

* init audio blocks

			move.l	_audio_ctrl,a1						; set-up open device
			move.b	#127,LN_PRI(a1)						; don't allow stealing
			move.l	_play_port,MN_REPLYPORT(a1)			; the reply port
			move.w	#$0f0f,-(sp)						; allocation key is 0f
			move.l	sp,ioa_Data(a1)						; data on stack
			moveq	#1,d0								; only one byte
			move.l	d0,ioa_Length(a1)

			lea		audio_name,a0						; call OpenDevice
			moveq	#0,d0
			moveq	#0,d1
			move.l	_SysBase,a6
			JSRLIB	OpenDevice
			addq.w	#2,sp								; clean-up stack
			
			tst.l	d0									; if not zero, an error
			bne		.l98

			move.l	_audio_ctrl,a1
			move.l	IO_DEVICE(a1),d0
			move.l	d0,_AudioDevice						; get AudioDevice ptr

* intitialize audio stopping io block

			move.l	_audio_stop,a2						; set-up open device
			move.l	_play_port,MN_REPLYPORT(a2)			; the reply port
			move.l	d0,IO_DEVICE(a2)					; AudioDevice
			move.w	#CMD_FLUSH,IO_COMMAND(a2)			; CMD_FLUSH
			move.b	#IOF_QUICK,IO_FLAGS(a2)				; IOF_QUICK
			move.w	ioa_AllocKey(a1),ioa_AllocKey(a2)	; copy AllocKey

* init audio blocks and put on play_port

			moveq	#3*NUM_VOICES-1,d2
			move.l	_audio_play,a3
.l2			move.l	a2,a0								; copy data to play iob
			move.l	a3,a1
			moveq	#ioa_SIZEOF,d0
			JSRLIB	CopyMem
			move.w	#CMD_WRITE,IO_COMMAND(a3)			; make it CMD_WRITE
			move.l	a3,a1
			JSRLIB	ReplyMsg							; put on port
			lea		ioa_SIZEOF(a3),a3
			dbra	d2,.l2								; loop

			move.l	a2,a0								; copy data to env iob
			move.l	_audio_env,a3
			move.l	a3,a1
			moveq	#ioa_SIZEOF,d0
			JSRLIB	CopyMem
			move.w	#ADCMD_PERVOL,IO_COMMAND(a3)		; make it ADCMD_PERVOL

			move.l	_temp_port,d0
			move.l	d0,MN_REPLYPORT(a2)					; reply stop to temp port
			move.l	d0,MN_REPLYPORT(a3)					; reply env to temp port

;			xref	_init_ahandler
;			jsr		_init_ahandler

.l99			moveq	#MEV_ERR_NONE,d0
			movem.l	(sp)+,d2/a2/a3/a6
			rts

.l98			moveq	#MEV_ERR_AUDIODEV,d0
			movem.l	(sp)+,d2/a2/a3/a6
			rts

*==========================================================================*
*
*	CloseMusic - de-allocate audio resources
*
*	void CloseMusic(void)
*
*==========================================================================*

			xdef	_CloseMusic
_CloseMusic
			xdef	CloseMusic
CloseMusic
			movem.l	d2/a2/a6,-(sp)

			tst.l	_AudioDevice					; music open?
			beq.s	.l99								; no, so don't close

			bclr.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music off
			bclr.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music off
			bclr.b	#MUSICB_PLAYNOTE,_maxtrax+mxtx_Flags

* kill all voices with SystemReset & stop_audio

			bsr		SystemReset

			move.w	#NUM_VOICES-1,d2
			lea		_voice,a2
.l2			btst.b	#VOICEB_BLOCKED,voice_Flags(a2)
			beq.s	.l1
			move.w	d2,d0
			bsr		stop_audio
.l1			clr.b	voice_Flags(a2)
			lea		voice_sizeof(a2),a2
			dbra	d2,.l2

			moveq	#0,d0								; restore audio filter
			move.b	_globaldata+glob_SaveFilter,d0
			jsr		SetAudioFilter

;			xref	_free_ahandler
;			jsr		_free_ahandler

* close audio device

			move.l	_audio_ctrl,a1
			move.l	_SysBase,a6
			JSRLIB	CloseDevice

			clr.l	_AudioDevice

.l99			movem.l	(sp)+,d2/a2/a6
			rts

*==========================================================================*
*
*	NoteOn - handles a NOTE ON midi command
*
*	void NoteOn(struct ChannelData *chan,UBYTE *midi,WORD pri)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_NoteOn
_NoteOn
			move.l	4(sp),a0							; channel
			move.l	8(sp),a1							; midi stream
			move.w	12(sp),d0							; priority
		endc
NoteOn
			movem.l	d2-d5/a2/a3/a5/a6,-(sp)
			move.l	a0,a2
			move.l	a1,a3
			move.w	d0,d2

		ifne HAS_MICROTONAL
			move.w	chan_Microtonal(a2),d1				; microtinal setting?
			bmi		.l1									; no, skip
			moveq	#0,d0
			move.b	(a3),d0								; note #
			add.w	d0,d0
			lea		_microtonal,a5
			move.w	d1,0(a5,d0.w)						; set microtonal table
.l1
		endc

			tst.b	1(a1)								; vol = 0 -> NOTE OFF
			beq		.l99									; just ignore

			move.l	chan_Patch(a2),a5					; get patch ptr
			tst.l	patch_Sample(a5)					; no sample, exit
			beq		.l99

			btst.b	#CHANB_MONO,chan_Flags(a2)			; in mono mode?
			beq.s	.l2									; no, skip mono stuff
			tst.b	chan_VoicesActive(a2)				; already voice on?
			beq.s	.l2									; no, skip mono stuff

			lea		_voice,a5
			moveq	#NUM_VOICES-1,d0
.l3			cmp.l	voice_Channel(a5),a2		; any voice REALLY on channel
			beq.s	.l4
			lea		voice_sizeof(a5),a5
			dbra	d0,.l3

.l4			tst.w	d0									; if none, error
			bmi		.l99

			cmp.b	#ENV_SUSTAIN,voice_Status(a5)		; sustain or earlier?
			bmi.s	.l5									; no, skip porta code
			btst.b	#CHANB_PORTAMENTO,chan_Flags(a2)	; portamento?
			beq.s	.l5									; no, skip porta code

			clr.l	voice_PortaTicks(a5)				; init portamento

			bset.b	#VOICEB_PORTAMENTO,voice_Flags(a5)	; set & test
			beq.s	.l24							; not already portamento, skip

			move.b	voice_EndNote(a5),voice_BaseNote(a5) ; leap to old end note
			
.l24			move.b	(a3),voice_EndNote(a5)
			move.b	(a3),chan_LastNote(a2)				; remember note

			move.w	#128,d1								; default note velocity
			btst.b	#MUSICB_VELOCITY,_globaldata+glob_Flags	; handling note vel?
			beq.s	.l22									; no, just use 128
			moveq	#0,d1
			move.b	1(a3),d1							; get note volume
			addq.b	#1,d1
.l22			move.w	d1,voice_NoteVolume(a5)				; set note volume

			bset.b	#MUSICB_ADDED_NOTE,_globaldata+glob_Flags
			move.b	voice_Number(a5),_globaldata+glob_LastVoice
			bra		.l99

.l2			moveq	#LEFT_0,d0
			btst.b	#CHANB_PAN,chan_Flags(a2)			; pan set?
			beq.s	.l6									; no, left voice
			moveq	#RIGHT_0,d0							; yes, right voice
.l6			move.w	d2,d1
			bsr		pick_voice							; pick a voice
			tst.l	d0
			beq		.l99									; no voice, exit
			move.l	d0,a5

.l5			tst.l	voice_Channel(a5)				; if voice in use, kill it
			beq.s	.l7

			move.l	a5,a0
			bsr		KillVoice
			move.b	#VOICE_STOLEN,voice_Flags(a5)		; mark as stolen
			bra.s	.l8
.l7			clr.b	voice_Flags(a5)

.l8			move.l	a2,voice_Channel(a5)				; setup voice data
			move.l	chan_Patch(a2),voice_Patch(a5)

			move.b	(a3),voice_BaseNote(a5)

			move.l	a5,a0
			bsr		CalcNote							; calc note period
			move.l	d0,d3								; save sample in d3

			move.b	d2,voice_Priority(a5)				; more voice setup
			move.b	#ENV_START,voice_Status(a5)
			move.w	#128,d1								; default note velocity
			btst.b	#MUSICB_VELOCITY,_globaldata+glob_Flags	; handling note vel?
			beq.s	.l23									; no, just use 128
			moveq	#0,d1
			move.b	1(a3),d1
			addq.b	#1,d1

.l23
		ifeq HAS_FULLCHANVOL
			moveq	#0,d0
			move.b	chan_Volume(a2),d0				; get channel volume
			bmi.s	.l25								; if vol >= 128, skip
			mulu.w	d0,d1							; multiply into volume
			lsr.w	#7,d1							; scale by 128
		endc

.l25			move.w	d1,voice_NoteVolume(a5)
			clr.w	voice_BaseVolume(a5)
			clr.l	voice_LastTicks(a5)

			move.w	voice_LastPeriod(a5),d5
			bne.s	.l9
			move.w	#1000,d5

.l9			move.l	d3,a0
			tst.l	samp_AttackSize(a0)					; any attack wave?
			beq.s	.l10									; no, skip

			move.l	_SysBase,a6							; get an audio block
			move.l	_play_port,a0
			JSRLIB	GetMsg
			tst.l	d0
			beq		.l98									; error, undo voice init

			move.l	d0,a1								; set-uo audio request
			move.w	#CMD_WRITE,IO_COMMAND(a1)
			move.b	#ADIOF_PERVOL|IOF_QUICK,IO_FLAGS(a1)
			clr.b	IO_ERROR(a1)
			move.w	#1,ioa_Cycles(a1)
			clr.w	ioa_Volume(a1)
			move.w	d5,ioa_Period(a1)
			moveq	#1,d1
			moveq	#0,d0
			move.b	voice_Number(a5),d0
			lsl.l	d0,d1
			move.l	d1,IO_UNIT(a1)
			move.l	d3,a0
			move.l	samp_Waveform(a0),ioa_Data(a1)
			move.l	samp_AttackSize(a0),ioa_Length(a1)

			move.l	a1,d4								; save ptr to audio block
			move.l	_AudioDevice,a6
			JSRDEV	BEGINIO

			move.l	d4,a1
			tst.b	IO_ERROR(a1)						; any error?
			beq.s	.l51									; no, skip error code

			move.l	_SysBase,a6
			JSRLIB	ReplyMsg							; put message back
			bra		.l98									; goto error code

.l51			DMACHECK

.l10			move.l	d3,a0
			tst.l	samp_SustainSize(a0)				; any sustain wave?
			beq		.l11									; no, skip

			move.l	_SysBase,a6							; get an audio block
			move.l	_play_port,a0
			JSRLIB	GetMsg
			tst.l	d0
			beq		.l98									; error, undo voice init

			move.l	d0,a1								; set-uo audio request
			move.w	#CMD_WRITE,IO_COMMAND(a1)
			move.b	#IOF_QUICK,IO_FLAGS(a1)
			clr.b	IO_ERROR(a1)
			clr.w	ioa_Cycles(a1)
			moveq	#1,d1
			moveq	#0,d0
			move.b	voice_Number(a5),d0
			lsl.l	d0,d1
			move.l	d1,IO_UNIT(a1)
			move.l	d3,a0
			move.l	samp_SustainSize(a0),ioa_Length(a1)
			move.l	samp_Waveform(a0),d0
			add.l	samp_AttackSize(a0),d0
			move.l	d0,ioa_Data(a1)

			tst.l	samp_AttackSize(a0)					; was there an attack?
			bne.s	.l12									; yes, so ready

			move.b	#ADIOF_PERVOL|IOF_QUICK,IO_FLAGS(a1)	; no, do full set-up
			clr.w	ioa_Volume(a1)
			move.w	d5,ioa_Period(a1)

.l12			move.l	a1,d4								; save ptr to audio block
			move.l	_AudioDevice,a6
			JSRDEV	BEGINIO

			move.l	d4,a1
			tst.b	IO_ERROR(a1)						; any error?
			beq.s	.l52									; no, skip error code

			move.l	_SysBase,a6
			JSRLIB	ReplyMsg							; put message back

			move.l	d3,a0
			tst.l	samp_AttackSize(a0)					; was there an attack?
			beq.s	.l98									; yes, so do error code
;			bra.s	.l11

.l52			DMACHECK

.l11			addq.b	#1,chan_VoicesActive(a2)			; up voices active
			addq.b	#1,_globaldata+glob_VoicesActive

			cmp.b	#16,chan_Number(a2)					; xchannel?
			beq.s	.l13									; yes, skip normal code

			btst.b	#CHANB_MONO,chan_Flags(a2)			; mono mode?
			beq.s	.l14									; no, skip porta stuff
			btst.b	#CHANB_PORTAMENTO,chan_Flags(a2)	; portamento on?
			beq.s	.l14									; no, skip porta stuff
			move.b	chan_LastNote(a2),d0
			bmi.s	.l14									; bit 7 set means no
			cmp.b	voice_BaseNote(a5),d0				; Last Note == BaseNote
			beq.s	.l14									; yes, no need for porta

			clr.l	voice_PortaTicks(a5)				; init portamento
			move.b	voice_BaseNote(a5),voice_EndNote(a5)	; EndNote = BaseNote
			move.b	d0,voice_BaseNote(a5)				; BaseNote = LastNote
			bset.b	#VOICEB_PORTAMENTO,voice_Flags(a5)

.l14			btst.b	#CHANB_PORTAMENTO,chan_Flags(a2)	; portamento on?
			beq.s	.l13									; no, skip
			move.b	(a3),chan_LastNote(a2)				; remember note

.l13			bset.b	#MUSICB_ADDED_NOTE,_globaldata+glob_Flags
			move.b	voice_Number(a5),_globaldata+glob_LastVoice

.l99			movem.l	(sp)+,d2-d5/a2/a3/a5/a6
			rts

.l98			clr.l	voice_Channel(a5)					; un-init voice
			clr.b	voice_Status(a5)
			clr.b	voice_Flags(a5)
			clr.b	voice_Priority(a5)
			clr.l	voice_UniqueID(a5)
			bra		.l99

* this is a temporary hack that will become a better, permanent hack later

		ifne	DO_DMACHECK
dma_check
			move.l	a1,d0
			move.l	_AudioDevice,a1						; use BeginIO
			cmp.w	#36,LIB_VERSION(a1)					; if KS v35 or less, skip
			bmi.s	.l3

			move.l	d0,a1

			move.w	$dff000+vhposr,d0					; wait 1 scanline
			and.w	#$ff00,d0
.l1			move.w	$dff000+vhposr,d1
			and.w	#$ff00,d1
			cmp.w	d0,d1
			beq.s	.l1

			move.w	#139,d0
.l2			move.w	$dff000+vhposr,d1
			dbra	d0,.l2

			move.w	IO_UNIT+2(a1),d0					; be sure note on!
			or.w	#$8000,d0
			move.w	d0,$dff000+dmacon

.l3			rts
		endc

*==========================================================================*
*
*	NoteOff - handles a NOTE OFF midi command
*
*	void NoteOff(struct ChannelData *chan,UBYTE *midi)
*
*	NOTE: Only has code for MaxTrax version, not MIDI
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_NoteOff
_NoteOff
			move.l	4(sp),a0						; channel
			move.l	8(sp),a1						; midi stream
		endc
NoteOff
			move.l	a2,-(sp)

			tst.b	chan_VoicesActive(a0)			; any voices on channel?
			beq.s	.l99								; no, exit

			lea		_voice,a2						; calc voice to release
			moveq	#0,d0
			move.b	_globaldata+glob_LastVoice,d0
			SCALE	voice_sizeof,d0
			add.w	d0,a2

			cmp.l	voice_Channel(a2),a0			; if channel wrong, exit
			bne.s	.l99

			cmp.b	#ENV_RELEASE,voice_Status(a2)	; if already released, exit
			ble.s	.l99

* confirm correct note based on portamento setting

			move.b	(a1),d0							; note to check

			btst.b	#VOICEB_PORTAMENTO,voice_Flags(a2)
			beq.s	.l1

			cmp.b	voice_EndNote(a2),d0			; does note match EndNote?
			bne.s	.l99								; no, exit
			bra.s	.l2								; yes, continue

.l1			cmp.b	voice_BaseNote(a2),d0			; does note match BaseNote?
			bne.s	.l99								; no, exit

.l2			btst.b	#CHANB_DAMPER,chan_Flags(a0)	; 'damper pedal' down?
			bne.s	.l3								; yes, do damper code

			move.b	#ENV_RELEASE,voice_Status(a2)	; release note
			bra.s	.l99

.l3			bset.b	#VOICEB_DAMPER,voice_Flags(a2)	; mark as dampered

.l99			move.l	(sp)+,a2
			rts

*==========================================================================*
*
*	AllNotesOff - handles a ALL NOTES OFF midi command
*
*	void AllNotesOff(struct ChannelData *chan)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_AllNotesOff
_AllNotesOff
			move.l	4(sp),a0
		endc
AllNotesOff
			tst.b	chan_VoicesActive(a0)			; no voices active, exit
			beq.s	.l99

			lea		_voice,a1						; go through all voices
			moveq	#NUM_VOICES-1,d0
.l1			cmp.l	voice_Channel(a1),a0			; on this channel?
			bne.s	.l2								; no, skip

			btst.b	#CHANB_DAMPER,chan_Flags(a0)	; 'damper pedal' down
			beq.s	.l3								; no, goto release
			bset.b	#VOICEB_DAMPER,voice_Flags(a1)	; mark voice as dampered
			bra.s	.l2

.l3			move.b	#ENV_RELEASE,voice_Status(a1)	; set status to RELEASE

.l2			lea		voice_sizeof(a1),a1				; next voice
			dbra	d0,.l1

.l99			rts

*==========================================================================*
*
*	AllSoundsOff - handles a ALL SOUNDS OFF midi command
*
*	void AllSoundsOff(struct ChannelData *chan)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_AllSoundsOff
_AllSoundsOff
			move.l	4(sp),a0
		endc
AllSoundsOff
			tst.b	chan_VoicesActive(a0)			; no voices active, exit
			beq.s	.l99

			movem.l	d2/a2/a3,-(sp)

			move.l	a0,a3							; save channel
			lea		_voice,a2						; go through all voices
			moveq	#NUM_VOICES-1,d2

.l1			cmp.l	voice_Channel(a2),a3			; on this channel?
			bne.s	.l2								; no, skip

			move.l	a2,a0
			bsr		KillVoice

.l2			lea		voice_sizeof(a2),a2				; next voice
			dbra	d2,.l1

			movem.l	(sp)+,d2/a2/a3

.l99			rts

*==========================================================================*
*
*	SystemReset - handles SYSTEM RESET, terminates sound on all voices
*
*	void SystemReset(void)
*
*==========================================================================*

			DEFX	_SystemReset
_SystemReset
SystemReset
			movem.l	d2/a2,-(sp)

* kill all active voices

			lea		_voice,a2
			moveq	#NUM_VOICES-1,d2
.l1			tst.l	voice_Channel(a2)
			beq.s	.l2
			move.l	a2,a0
			bsr		KillVoice
.l2			lea		voice_sizeof(a2),a2
			dbra	d2,.l1

* reset all channels

			lea		_channel,a2
			moveq	#NUM_CHANNELS-1,d2
.l3			move.l	a2,a0
			bsr		ResetChannel
			bclr.b	#CHANB_MONO,chan_Flags(a2)
			moveq	#-1,d0
			move.b	d0,chan_LastNote(a2)				; no last note
			lea		chan_sizeof(a2),a2
			dbra	d2,.l3

		ifne HAS_MICROTONAL

* reset microtonal values to defaults

			lea		_microtonal,a2
			moveq	#128-1,d2
			moveq	#0,d0
.l4			move.w	d0,(a2)+
			add.w	#$0100,d0
			dbra	d2,.l4
		endc

			movem.l	(sp)+,d2/a2
			rts

*==========================================================================*
*
*	ProgramCh - handles PROGRAM CHANGE midi command
*
*	void ProgramCh(struct ChannelData *chan,UBYTE *midi)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_ProgramCh
_ProgramCh
			move.l	4(sp),a0						; channel
			move.l	8(sp),a1						; midi stream
		endc
ProgramCh
			moveq	#0,d0
			move.b	(a1),d0							; get patch number
			and.w	#NUM_PATCHES_MASK,d0			; mask to allowable range
			SCALE	patch_sizeof,d0
			lea		_patch,a1						; get base of patches
			add.w	d0,a1							; adjust to right patch
			move.l	a1,chan_Patch(a0)				; save pointer in channel
			rts

*==========================================================================*
*
*	SetTempo - adjust timing to match tempo
*
*	void SetTempo(UWORD tempo)
*
*==========================================================================*

			xdef	_SetTempo
_SetTempo
			move.l	4(sp),d0

			xdef	SetTempo
SetTempo
			lea		_globaldata,a0
			move.w	d0,glob_CurrentTempo(a0)		; save new tempo

			lsr.w	#4,d0					; (tempo>>4)*(192<<8)/(60*Frequency)
			mulu.w	#(192<<8),d0
			move.w	glob_Frequency(a0),d1
			mulu.w	#60,d1
			move.l	a0,-(sp)
			jsr		divu
			move.l	(sp)+,a0
			move.l	d0,glob_TickUnit(a0)			; -> TickUnit
			rts

*==========================================================================*
*
*	GetTempo - return current tempo setting
*
*	LONG GetTempo(void)
*
*==========================================================================*

			xdef	_GetTempo
			xdef	GetTempo
_GetTempo
GetTempo
			lea		_globaldata,a0
			moveq	#0,d0
			move.w	glob_CurrentTempo(a0),d0		; get current tempo
			rts

*==========================================================================*
*
*	SyncSong - set-up or clear song synchronization
*
*	void SyncSong(struct Task *task,ULONG signal)
*
*==========================================================================*

			xdef	_SyncSong
_SyncSong
			move.l	4(sp),a0						; task
			move.l	8(sp),d0						; signal

			xdef	SyncSong
SyncSong
			lea		_globaldata,a1
			move.l	a0,glob_SyncTask(a1)			
			move.l	d0,glob_SyncSig(a1)			
			rts

*==========================================================================*
*
*	SetAudioFilter - turn on or off audio filter
*
*	void SetAudioFilter(BOOL)
*
*==========================================================================*

			xdef	_SetAudioFilter
_SetAudioFilter
			move.l	4(sp),d0

			xdef	SetAudioFilter
SetAudioFilter
			move.w	d0,d1
			bsr.s	GetAudioFilter
			tst.w	d1								; test single argument
			bne.s	.l1
			bset	#1,$bfe001
			rts
.l1			bclr	#1,$bfe001
			rts

*==========================================================================*
*
*	GetAudioFilter - read state of audio filter
*
*	int GetAudioFilter(void)
*
*==========================================================================*

			xdef	_GetAudioFilter
_GetAudioFilter
GetAudioFilter
			move.b	$bfe001,d0						; check hardware
			btst	#1,d0
			bne.s	.l1								; if bit set, filter is OFF
			moveq	#1,d0							; else it's ON
			rts
.l1			moveq	#0,d0							; it's OFF
			rts

*==========================================================================*
*
*	MusicServer - handles score and timing
*
*	void MusicServer(void)
*
*==========================================================================*

; Changes made by --DJ
; Changed all references to _maxtrax and _globaldata to be relative addressing

			DEFX	_MusicServer
_MusicServer
MusicServer
			COLOR0	$0ff0
			movem.l	d2/d3/d4/d5/a2-a6,-(sp)
			subq.w	#2,sp							; space for MIDI stream

			lea		_maxtrax,a5						; preload address of _maxtrax
			lea		_globaldata,a4					; preload address of _globaldata

			tst.b	mxtx_Changed(a5)
			beq.s	.l30

			moveq	#0,d0
			move.b	mxtx_Volume(a5),d0				; copy over volume
			move.w	d0,glob_Volume(a4)
			move.b	mxtx_Flags(a5),d0
			and.b	#MUSIC_VELOCITY,d0				; copy over velocity flag
			and.b	#~MUSIC_VELOCITY,glob_Flags(a4)
			or.b	d0,glob_Flags(a4)

			lea		_voice+voice_Channel,a0			; mark all used channels ALTERED
			moveq	#NUM_VOICES-1,d2
.l31			move.l	(a0),d0							; d0 <-- channel data pointer
			beq.s	.l32								; if no channel data, skip
			move.l	d0,a1							; a1 <-- channel data pointer
			bset.b	#CHANB_ALTERED,chan_Flags(a1)	; set channel as altered
.l32			lea		voice_sizeof(a0),a0				; go to next voice
			dbra	d2,.l31							; and loop

			clr.b	mxtx_Changed(a5)

.l30			move.l	glob_TickUnit(a4),d0			; add TickUnit to Ticks
			add.l	d0,glob_Ticks(a4)

			move.l	glob_Ticks(a4),d4				; shifted clock value
			lsr.l	#8,d4

			moveq	#0,d5
			moveq	#NUM_VOICES-1,d2				; check stop events
			lea		glob_NoteOff(a4),a2				; first NoteOff
			lea		_voice,a3						; first voice

; Q: Why isn't this integrated into the actual Voice table?

; look at the table of note-offs (one for each audio channel)

.l1			move.b	d5,glob_LastVoice(a4)			; set last voice
			tst.l	voice_Channel(a3)				; voice playing something?
			beq.s	.l2								; no, skip
			move.b	sev_Command(a2),d0				; get note # (error ??)
			bmi.s	.l2								; high bit set, no note

			moveq	#0,d1
			move.b	sev_Data(a2),d1					; channel #
			cmp.b	#16,d1							; is it the extra channel?
			beq.s	.l3								; yes, do that instead

			move.l	glob_TickUnit(a4),d3			; get TickUnit
			sub.l	d3,sev_StopTime(a2)				; subtract from stoptime
			bgt.s	.l2								; still time left, skip

; why is this needed?

			move.b	d0,(sp)							; set-up NoteOff
			clr.b	1(sp)
			lea		_channel,a0						; figure out channel addr
			SCALE	chan_sizeof,d1
			add.w	d1,a0
			move.l	sp,a1
			bsr		NoteOff							; do a NoteOff
			move.b	#$ff,sev_Command(a2)			; set stop event to not used
			bra.s	.l2

;.l4			sub.l	d3,sev_StopTime(a2)				; subtract TickUnit
;			bra.s	.l2

.l3			move.l	glob_FrameUnit(a4),d3			; get FrameUnit
			sub.l	d3,sev_StopTime(a2)				; subtract from stoptime
			bgt.s	.l2								; still time left, skip

			move.b	d0,(sp)							; set-up NoteOff
			clr.b	1(sp)
			lea		_xchannel,a0					; get xchannel addr
			move.l	sp,a1
			bsr		NoteOff							; do a NoteOff
			move.b	#$ff,sev_Command(a2)			; set stop event to not used

.l2			addq.w	#1,d5
			lea		sev_sizeof(a2),a2
			lea		voice_sizeof(a3),a3
			dbra	d2,.l1

			btst.b	#MUSICB_PLAYING,glob_Flags(a4)	; music playing?
			beq		.l20								; no, skip sequencer code
													; also skipping tempo code

			;COLOR0	$00f0

			move.l	glob_Current(a4),a2				; get current event ptr

*	Here is a real serious kludge to keep from having to much time spent
*	in this interupt...

.l5			move.w	$00dff000+vhposr,d0				; get beam position
			bmi		.l60								; if >= 128, prematurely end

			moveq	#0,d0
			move.w	cev_StartTime(a2),d0
			add.l	glob_CurrentTime(a4),d0			; calc new CurrrentTime
			cmp.l	d0,d4							; compare event time w/clocks
			bmi		.l60								; if greater, stop processing

			move.l	d0,glob_CurrentTime(a4)			; store new CurrentTime
			move.b	cev_Command(a2),d1				; get command
			bmi.s	.l7								; not a note, skip ahead

													; reset note added flag
			bclr.b	#MUSICB_ADDED_NOTE,glob_Flags(a4)

			move.b	d1,(sp)							; note #
			move.b	cev_Data(a2),d1					; volume & channel
			moveq	#0,d2
			move.b	d1,d2							; keep copy
			lsr.b	#1,d1
			and.b	#$78,d1							; extract volume
			move.b	d1,1(sp)						; second MIDI value

			and.b	#$0f,d2							; extract channel #
			SCALE	chan_sizeof,d2
			lea		_channel,a0
			add.w	d2,a0							; calc channel addr
			move.l	sp,a1							; MIDI stream
			moveq	#MUSIC_PRI_SCORE,d0				; priority 0 normally

			COLOR0	$00ff

			bsr		NoteOn

			COLOR0	$0f0f
													; was a note added?
			btst.b	#MUSICB_ADDED_NOTE,glob_Flags(a4)
			beq		.l70								; nope, skip

			moveq	#0,d0
			move.b	glob_LastVoice(a4),d0
			SCALE	sev_sizeof,d0
			lea		glob_NoteOff(a4),a0
			add.w	d0,a0

			move.b	(sp),sev_Command(a0)			; set-up stop event
			move.b	cev_Data(a2),d0
			and.b	#$0f,d0
			move.b	d0,sev_Data(a0)
			moveq	#0,d0
			move.w	cev_StopTime(a2),d0				; get stop time
			add.l	glob_CurrentTime(a4),d0			; add low word CurrentTime
			sub.l	d4,d0							; sub low word Ticks
			lsl.l	#8,d0							; put into TickUnit units
			move.l	d0,sev_StopTime(a0)				; store it!
			bra		.l70

.l7			cmp.b	#COMMAND_TEMPO,d1					; tempo event?
			bne.s	.l8									; no, skip

			move.l	glob_TickUnit(a4),d0
			lsr.l	#8,d0
			cmp.w	cev_StopTime(a2),d0					; is length < TickUnit?
			bmi.s	.l9									; no, do continuous

			moveq	#0,d0
			move.b	cev_Data(a2),d0
			lsl.w	#4,d0
			bsr		SetTempo							; set tempo immediately
			clr.l	glob_TempoTime(a4)					; clear tempo time
			bra		.l70

.l9			move.w	glob_CurrentTempo(a4),d0
			move.w	d0,glob_StartTempo(a4)				; StartTempo = CurrentTempo
			moveq	#0,d1
			move.b	cev_Data(a2),d1
			lsl.w	#4,d1
			sub.w	d0,d1								; (tempo<<4) - StartTempo
			move.w	d1,glob_DeltaTempo(a4)				; store as DeltaTempo

			moveq	#0,d0
			move.w	cev_StopTime(a2),d0
			lsl.l	#8,d0
			move.l	d0,glob_TempoTime(a4)				; length of tempo change
			clr.l	glob_TempoTicks(a4)					; reset tick count
			bra		.l70

.l8			cmp.b	#COMMAND_END,d1						; end event?
			bne.s	.l10									; no, skip to next

			btst.b	#MUSICB_LOOP,glob_Flags(a4)			; looping score?
			beq.s	.l11									; no, cut off

			move.l	glob_CurrentScore(a4),a0			; get start of score
			move.l	score_Data(a0),a2					; make it current event
			clr.l	glob_Ticks(a4)						; reset clocks
			clr.l	glob_CurrentTime(a4)
			bra		.l60									; process no more events

.l11			bclr.b	#MUSICB_PLAYING,glob_Flags(a4)		; stop music
			bra		.l60									; process no more events

.l10			cmp.b	#COMMAND_BEND,d1					; pitch bend command?
			bne.s	.l12									; no, skip to next

			move.b	cev_StopTime+1(a2),d0				; LSB of bend
			and.b	#$7f,d0
			move.b	d0,(sp)
			move.b	cev_StopTime(a2),d0					; MSB of bend
			and.b	#$7f,d0
			move.b	d0,1(sp)

			move.b	cev_Data(a2),d0						; get channel
			and.w	#$000f,d0
			SCALE	chan_sizeof,d0
			lea		_channel,a0
			add.w	d0,a0
			move.l	sp,a1								; MIDI stream
			bsr		PitchBend
			bra		.l70

.l12			cmp.b	#COMMAND_CONTROL,d1					; control change command?
			bne.s	.l13									; no, skip to next

			move.b	cev_StopTime(a2),(sp)				; control #
			move.b	cev_StopTime+1(a2),1(sp)			; value

			move.b	cev_Data(a2),d0						; get channel
			and.w	#$000f,d0
			SCALE	chan_sizeof,d0
			lea		_channel,a0
			add.w	d0,a0
			move.l	sp,a1								; MIDI stream
			bsr		ControlCh
			bra		.l70

.l13			cmp.b	#COMMAND_PROGRAM,d1					; program change command?
			bne.s	.l14									; no, skip to next

			move.b	cev_StopTime+1(a2),(sp)				; program #

			move.b	cev_Data(a2),d0						; get channel
			and.w	#$000f,d0
			SCALE	chan_sizeof,d0
			lea		_channel,a0
			add.w	d0,a0
			move.l	sp,a1								; MIDI stream
			bsr		ProgramCh
			bra		.l70

.l14			cmp.b	#COMMAND_SPECIAL,d1					; program change command?
			bne		.l70									; no, that's all

			moveq	#0,d1
			move.b	cev_StopTime+1(a2),d1
			move.b	cev_StopTime(a2),d0

			cmp.b	#SPECIAL_SYNC,d0					; sync event?
			bne.s	.l15									; no, skip to next

			move.l	glob_SyncTask(a4),d0				; is there a sync task?
			beq		.l70

			move.l	d0,a1								; put in right register
			move.l	glob_SyncSig(a4),d0					; get signal
			move.b	d1,mxtx_SyncValue(a5)				; set sync value

			move.l	_SysBase,a6
			JSRLIB	Signal								; signal task
			bra.s	.l70

.l15			cmp.b	#SPECIAL_BEGINREP,d0				; begin repeat event?
			bne.s	.l16									; no, skip to next

			moveq	#0,d0
			move.b	glob_RepeatTotal(a4),d0				; get repeat total
			cmp.b	#NUM_REPEATS,d0						; test against max
			beq.s	.l70									; oops, too many

			addq.b	#1,glob_RepeatTotal(a4)				; increment total

			lea		glob_RepeatCount(a4),a0				; value is repeat count
			move.b	d1,0(a0,d0.w)

			lea		glob_RepeatPoint(a4),a0
			add.w	d0,d0
			add.w	d0,d0
			add.w	d0,a0
			lea		cev_sizeof(a2),a1					; get event + 1
			move.l	a1,(a0)								; store in RepeatPoint
			bra.s	.l70

.l16			cmp.b	#SPECIAL_ENDREP,d0					; begin repeat event?
			bne.s	.l70									; no, that's all

			moveq	#0,d0
			move.b	glob_RepeatTotal(a4),d0				; get repeat total
			beq.s	.l70									; ain't any?? oh, well

			subq.w	#1,d0								; find right entry
			lea		glob_RepeatCount(a4),a0
			tst.b	0(a0,d0.w)							; test repeat count
			beq.s	.l17									; last loop, skip ahead

			subq.b	#1,0(a0,d0.w)						; reduce count
			lea		glob_RepeatPoint(a4),a0				; get repeat point
			add.w	d0,d0
			add.w	d0,d0
			add.w	d0,a0
			move.l	(a0),a2								; set event ptr

			clr.l	glob_Ticks(a4)						; reset clocks
			clr.l	glob_CurrentTime(a4)
			bra.s	.l60

.l17			move.b	d0,glob_RepeatTotal(a4)				; reduce total
			bra		.l70									; continue

.l70			lea		cev_sizeof(a2),a2					; go to next event
			bra		.l5									; and loop

.l60			move.l	a2,glob_Current(a4)					; store last ev checked

			;COLOR0	$0f00

.l80			move.l	glob_TempoTime(a4),d0				; cont. tempo on?
			beq.s	.l20									; nope...

			move.l	glob_TempoTicks(a4),d1				; adjust TempoTicks
			add.l	glob_TickUnit(a4),d1
			move.l	d1,glob_TempoTicks(a4)

			cmp.l	d0,d1
			bmi.s	.l21

			move.w	glob_StartTempo(a4),d0
			add.w	glob_DeltaTempo(a4),d0
			bsr		SetTempo
			clr.l	glob_TempoTime(a4)					; clear tempo time
			bra.s	.l20

.l21			moveq	#0,d0
			move.w	glob_DeltaTempo(a4),d0
			jsr		mulu								; DeltaTempo * TempoTicks
			move.l	glob_TempoTime(a4),d1
			jsr		divs								; / TempoTime
			add.w	glob_StartTempo(a4),d0
			bsr		SetTempo

.l20			move.l	glob_FrameUnit(a4),d0				; do envelopes
;			COLOR0	$0fff
			bsr		EnvelopeManager
;			COLOR0	$0777

* set silent flag correctly and do check_sound calls as needed

			bset.b	#MUSICB_SILENT,glob_Flags(a4)

			lea		_voice,a2
			moveq	#NUM_VOICES-1,d2
.l22			tst.l	voice_Channel(a2)
			beq.s	.l23
			bclr.b	#MUSICB_SILENT,glob_Flags(a4)
			bra.s	.l24

.l23			btst.b	#VOICEB_BLOCKED,voice_Flags(a2)		; voice blocked?
			beq.s	.l24									; no, so don't check
			move.l	a2,a0
			bsr		check_sound
.l24			lea		voice_sizeof(a2),a2
			dbra	d2,.l22

			bclr.b	#MUSICB_PLAYNOTE,_maxtrax+mxtx_Flags
			tst.b	_xchannel+chan_VoicesActive
			beq.s	.l25
			bset.b	#MUSICB_PLAYNOTE,_maxtrax+mxtx_Flags

.l25			move.b	glob_Flags(a4),d0
			and.b	#(MUSIC_PLAYING|MUSIC_SILENT|MUSIC_LOOP),d0
			and.b	#~(MUSIC_PLAYING|MUSIC_SILENT|MUSIC_LOOP),mxtx_Flags(a5)
			or.b	d0,mxtx_Flags(a5)

			addq.w	#2,sp							; cleanup stack
			movem.l	(sp)+,d2/d3/d4/d5/a2-a6

			COLOR0	$000f

			moveq	#0,d0
			rts

*==========================================================================*
*
*	AdvanceSong - advance a song to next mark
*
*
*	BOOL AdvanceSong(WORD which)
*
*	void advance_song(WORD which)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_advance_song
_advance_song
			move.w	4(sp),d0							; amount to advance
		endc
advance_song
			move.l	_globaldata+glob_Current,a0			; current location
			move.l	a0,a1								; scan location
			bra.s	.l1

.l2			cmp.b	#COMMAND_END,cev_Command(a1)		; is it an END event?
			bne.s	.l3									; no, skip
			
			move.l	_globaldata+glob_CurrentScore,a1	; else, reset to start
			move.l	score_Data(a1),a0
			move.l	a0,a1
			bra.s	.l1									; and goto outer loop

.l3			cmp.b	#COMMAND_SPECIAL,cev_Command(a1)	; is it MARK event?
			bne.s	.l4									; no, skip
			cmp.b	#SPECIAL_MARK,cev_StopTime(a1)
			bne.s	.l4									; no, skip

			lea		cev_sizeof(a1),a1					; advance scan
			move.l	a1,a0								; set start to scan
			bra.s	.l1									; and goto outer loop

.l4			lea		cev_sizeof(a1),a1					; advance scan
			bra		.l2									; continue inner loop

.l1			dbra	d0,.l2								; loop

			move.l	a0,_globaldata+glob_Current			; location = start
			rts

			xdef	_AdvanceSong
_AdvanceSong
			move.l	4(sp),d0							; amount to advance

			xdef	AdvanceSong
AdvanceSong
			move.w	#EXTRA_ADVANCE,_extra_op			; the op
			ext.l	d0
			move.l	d0,_extra_data						; the data

			move.l	a6,-(sp)							; cause a softint
			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause
			move.l	(sp)+,a6

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint

			move.l	_extra_data,d0						; get result
			rts

*==========================================================================*
*
*	PlaySong - start a song playing
*
*	BOOL PlaySong(WORD which)
*
*==========================================================================*

			xdef	_PlaySong
_PlaySong
			move.l	4(sp),d0

			xdef	PlaySong
PlaySong
			move.w	d2,-(sp)
			move.w	d0,d2

			jsr		OpenMusic							; open music
			tst.l	d0
			bne		.l98									; if not 0, error

			bclr.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music off
			bclr.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music off
			bclr.b	#MUSICB_LOOP,_globaldata+glob_Flags	; no looping

			jsr		SystemReset

			move.l	_globaldata+glob_CurrentScore,a0	; reset score pointer
			move.l	score_Data(a0),_globaldata+glob_Current

			move.w	d2,d0								; advance to mark
			jsr		advance_song

			moveq	#-1,d0								; reset note off's
			move.b	d0,_globaldata+glob_NoteOff+sev_Command
			move.b	d0,_globaldata+glob_NoteOff+sev_Command+sev_sizeof
			move.b	d0,_globaldata+glob_NoteOff+sev_Command+2*sev_sizeof
			move.b	d0,_globaldata+glob_NoteOff+sev_Command+3*sev_sizeof

			move.w	_globaldata+glob_Tempo,d0			; set tempo
			lsl.w	#4,d0
			bsr		SetTempo

			move.w	_globaldata+glob_Filter,d0			; set filter
			bsr		SetAudioFilter

			clr.b	_globaldata+glob_RepeatTotal		; clear things
		ifne HAS_ALLMIDI
			clr.b	_globaldata+glob_RunningStatus
		endc
			clr.l	_globaldata+glob_CurrentTime
			clr.l	_globaldata+glob_TempoTime
			clr.l	_globaldata+glob_Ticks

			bset.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music on!
			bset.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags		; tell user

			move.w	(sp)+,d2
			moveq	#1,d0
			rts

.l98			move.w	(sp)+,d2
			moveq	#0,d0
			rts

*==========================================================================*
*
*	LoopSong - start a song playing w/looping
*
* BOOL LoopSong(WORD which)
*
*==========================================================================*

			xdef	_LoopSong
_LoopSong
			move.l	4(sp),d0

			xdef	LoopSong
LoopSong
			jsr		PlaySong							; condition codes set
			beq.s	.l98
			bset.b	#MUSICB_LOOP,_globaldata+glob_Flags	; looping
.l98			rts

*==========================================================================*
*
*	StopSong - stop a playing song
*
*	void StopSong(void)
*
*==========================================================================*

			xdef	_StopSong
_StopSong
			xdef	StopSong
StopSong
			movem.l	d2/a2,-(sp)

			bclr.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music off
			bclr.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music off

			lea		_voice,a2								; kill voices
			moveq	#NUM_VOICES-1,d2
.l1			tst.l	voice_Channel(a2)
			beq.s	.l2
			move.l	a2,a0
			bsr		KillVoice
.l2			lea		voice_sizeof(a2),a2
			dbra	d2,.l1

			movem.l	(sp)+,d2/a2
			rts

*==========================================================================*
*
*	ContinueSong - continue playing a song
*
*	void ContinueSong(void)
*
*==========================================================================*

			xdef	_ContinueSong
_ContinueSong
			xdef	ContinueSong
ContinueSong
			bset.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music on!
			bset.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music on!
			rts

*==========================================================================*
*
*	SelectScore - select which score to use
*
*	BOOL SelectScore(WORD which)
*
*==========================================================================*

			xdef	_SelectScore
_SelectScore
			move.l	4(sp),d0

			xdef	SelectScore
SelectScore
			cmp.w	_globaldata+glob_TotalScores,d0
			bpl.s	.l98

			bclr.b	#MUSICB_PLAYING,_globaldata+glob_Flags	; turn music off
			bclr.b	#MUSICB_PLAYING,_maxtrax+mxtx_Flags	; turn music on!
			move.l	_scoreptr,a0
			SCALE	score_sizeof,d0
			add.w	d0,a0
			move.l	a0,_globaldata+glob_CurrentScore

			bsr		SystemReset

			moveq	#1,d0
			rts

.l98			moveq	#0,d0
			rts

*==========================================================================*
*
*	PlayNote - play a single note externally to sequencer
*
*	LONG PlayNote(UWORD note,UWORD patch,UWORD duration,UWORD volume,UWORD pan)
*
*==========================================================================*

			xdef	_PlayNote
_PlayNote
			movem.l	d2/d3/d4/a6,-(sp)
			move.l	4+16(sp),d0
			move.l	8+16(sp),d1
			move.l	12+16(sp),d2
			move.l	16+16(sp),d3
			move.l	20+16(sp),d4
			bra.s	pn_merge

			xdef	PlayNote
PlayNote
			movem.l	d2/d3/d4/a6,-(sp)
pn_merge
			sub.w	#nblk_sizeof,sp
			move.w	d0,nblk_Note(sp)
			move.w	d1,nblk_Patch(sp)
			move.w	d2,nblk_Duration(sp)
			move.w	d3,nblk_Volume(sp)
			move.w	d4,nblk_Pan(sp)

			move.w	#EXTRA_NOTE,_extra_op
			move.l	sp,_extra_data

			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint

			move.l	_extra_data,d0						; get result
			add.w	#nblk_sizeof,sp
			movem.l	(sp)+,d2/d3/d4/a6
			rts

*==========================================================================*
*
*	PlaySound - play a sampled sound by stealing an audio channel
*
*
*	LONG PlaySound(void *sample,LONG length,WORD volume,WORD period,WORD side)
*
*==========================================================================*

			xdef	_PlaySound
_PlaySound
			movem.l	d2/d3/a6,-(sp)
			move.l	4+12(sp),a0
			move.l	8+12(sp),d0
			move.l	12+12(sp),d1
			move.l	16+12(sp),d2
			move.l	20+12(sp),d3
			bra.s	ps_merge

			xdef	PlaySound
PlaySound
			movem.l	d2/d3/a6,-(sp)
ps_merge
			sub.w	#sblk_sizeof,sp
			move.l	a0,sblk_Data(sp)
			move.l	d0,sblk_Length(sp)
			move.w	d1,sblk_Volume(sp)
			move.w	d2,sblk_Period(sp)
			move.w	d3,sblk_Pan(sp)

			move.w	#EXTRA_PLAYSOUND,_extra_op
			move.l	sp,_extra_data

			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint

			move.l	_extra_data,d0						; get result
			add.w	#sblk_sizeof,sp
			movem.l	(sp)+,d2/d3/a6
			rts

*==========================================================================*
*
*	StopSound - stop a playing sampled sound
*
*	void StopSound(LONG cookie)
*
*==========================================================================*

			xdef	_StopSound
_StopSound
			move.l	4(sp),d0

			xdef	StopSound
StopSound
			move.w	#EXTRA_STOPSOUND,_extra_op
			move.l	d0,_extra_data

			move.l	a6,-(sp)
			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause
			move.l	(sp)+,a6

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint
			rts

*==========================================================================*
*
*	CheckSound - see if sampled sound still playing
*
*	BOOL check_sound(struct VoiceData *v)
*
*	LONG CheckSound(LONG cookie)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_check_sound
_check_sound
			move.l	4(sp),a0
		endc
check_sound
			movem.l	d2/a2/a3/a6,-(sp)
			moveq	#0,d2								; result = FALSE
			move.l	a0,a2

			btst.b	#VOICEB_BLOCKED,voice_Flags(a2)		; voice blocked?
			beq.s	.l1									; no, so don't check

			move.l	_SysBase,a6							; get an audio block
			move.l	_play_port,a0
			JSRLIB	GetMsg
			tst.l	d0
			beq.s	.l1									; failed!

			move.l	d0,a3								; set up audio read call
			move.w	#CMD_READ,IO_COMMAND(a3)
			clr.b	IO_FLAGS(a3)
			clr.b	IO_ERROR(a3)
			moveq	#1,d0
			moveq	#0,d1
			move.b	voice_Number(a2),d1					; set unit number
			lsl.l	d1,d0
			move.l	d0,IO_UNIT(a3)

			move.l	a3,a1
			move.l	_AudioDevice,a6						; do it!
			JSRDEV	BEGINIO

			tst.b	IO_ERROR(a3)		; check conditions matching sound done 
			bne.s	.l2
			tst.l	ioa_Data(a3)
			beq.s	.l2

			moveq	#1,d2								; voice still going
			bra.s	.l1

.l2			bclr.b	#VOICEB_BLOCKED,voice_Flags(a2)		; unblock voice
			clr.b	voice_Priority(a2)					; clear priority
			clr.l	voice_UniqueID(a2)					; clear id
			moveq	#0,d0
			move.b	voice_Link(a2),d0
			cmp.b	voice_Number(a2),d0					; is this stereo?
			beq.s	.l1									; no, skip

			SCALE	voice_sizeof,d0					; check sound on link
			lea		_voice,a0
			add.w	d0,a0
			jsr		check_sound

.l1			move.l	d2,d0
			movem.l	(sp)+,d2/a2/a3/a6
			rts

xcheck_sound
			movem.l	d2/a2/a3/a6,-(sp)
			moveq	#0,d2								; result = FALSE
			move.l	a0,a2

			move.l	_SysBase,a6							; get an audio block
			move.l	_play_port,a0
			JSRLIB	GetMsg
			tst.l	d0
			beq.s	.l1									; failed!

			move.l	d0,a3								; set up audio read call
			move.w	#CMD_READ,IO_COMMAND(a3)
			clr.b	IO_FLAGS(a3)
			clr.b	IO_ERROR(a3)
			moveq	#1,d0
			moveq	#0,d1
			move.b	voice_Number(a2),d1					; set unit number
			lsl.l	d1,d0
			move.l	d0,IO_UNIT(a3)

			move.l	a3,a1
			move.l	_AudioDevice,a6						; do it!
			JSRDEV	BEGINIO

			tst.b	IO_ERROR(a3)		; check conditions matching sound done 
			bne.s	.l2
			tst.l	ioa_Data(a3)
			beq.s	.l2

			moveq	#1,d2								; voice still going
			bra.s	.l1

.l2			move.l	voice_Channel(a2),a0				; get channel

			subq.b	#1,chan_VoicesActive(a0)			; -1 active voice
		ifeq IN_MUSICX
			lea		_globaldata,a0
			subq.b	#1,glob_VoicesActive(a0)
		endc

			clr.l	voice_Channel(a2)					; clear voice
			clr.b	voice_Status(a2)					; ENV_FREE
			clr.b	voice_Flags(a2)
			clr.b	voice_Priority(a2)
			clr.l	voice_UniqueID(a2)

.l1			move.l	d2,d0
			movem.l	(sp)+,d2/a2/a3/a6
			rts

			xdef	_CheckSound
_CheckSound
			move.l	4(sp),d0

			xdef	CheckSound
CheckSound
			move.w	#EXTRA_CHECKSOUND,_extra_op
			move.l	d0,_extra_data

			move.l	a6,-(sp)
			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause
			move.l	(sp)+,a6

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint

			move.l	_extra_data,d0						; get result
			rts

			xdef	_CheckNote
_CheckNote
			move.l	4(sp),d0

			xdef	CheckNote
CheckNote
			move.w	#EXTRA_CHECKNOTE,_extra_op
			move.l	d0,_extra_data

			move.l	a6,-(sp)
			move.l	_SysBase,a6
			lea		_extra_server,a1
			JSRLIB	Cause
			move.l	(sp)+,a6

.l1			tst.w	_extra_op							; busy loop until OS
			bne		.l1									;	notices softint

			move.l	_extra_data,d0						; get result
			rts

*==========================================================================*
*
*	ExtraServer - handle non-score features of driver
*
*	void ExtraServer(void)
*
*==========================================================================*

			DEFX	_ExtraServer
_ExtraServer
ExtraServer
			movem.l	d2-d4/a2/a3/a5/a6,-(sp)
			move.w	_extra_op,d2						; get op into register
			clr.w	_extra_op							; clear indicates handled

*	PlayNote

			cmp.w	#EXTRA_NOTE,d2						; PlayNote operation?
			bne		.l1									; no

			move.l	_extra_data,a2						; get extra data
			clr.l	_extra_data							; init result to false

			bsr		OpenMusic							; be sure driver active
			tst.l	d0
			bne		.l99									; if not zero, error

			lea		_xchannel,a0
			clr.b	chan_Flags(a0)						; clear xchannel flags
			tst.w	nblk_Pan(a2)						; pan?
			beq.s	.l2									; no, skip
			move.b	#CHAN_PAN,chan_Flags(a0)			; set to CHAN_PAN

.l2			move.w	nblk_Patch(a2),d0					; which patch?
			lea		_patch,a1
			SCALE	patch_sizeof,d0
			add.w	d0,a1								; get address of patch
			move.l	a1,chan_Patch(a0)					; put in xchannel

			subq.w	#2,sp								; space for midi stream
			move.w	nblk_Note(a2),d0
			bclr.l	#7,d0
			move.b	d0,(sp)								; set note #, midi[0]
			move.b	nblk_Volume+1(a2),1(sp)				; set note volume

			bclr.b	#MUSICB_ADDED_NOTE,_globaldata+glob_Flags	; reset added
			move.l	sp,a1								; midi stream
			moveq	#MUSIC_PRI_NOTE,d0					; priority 1 normally
			bsr		NoteOn								; note on (channel in a0)

			btst.b	#MUSICB_ADDED_NOTE,_globaldata+glob_Flags	; note added?
			beq.s	.l3									; nope...

			moveq	#0,d0								; add note off on voice
			move.b	_globaldata+glob_LastVoice,d0
			lea		_globaldata+glob_NoteOff,a0
			SCALE	sev_sizeof,d0
			add.w	d0,a0								; get stop event addr

			move.b	(sp),sev_Command(a0)				; set command
			move.b	#16,sev_Data(a0)					; set data (channel 16)
			moveq	#0,d0
			move.w	nblk_Duration(a2),d0
			lsl.l	#8,d0
			move.l	d0,sev_StopTime(a0)					; set stop time

			moveq	#0,d0
			move.b	_globaldata+glob_LastVoice,d0
			addq.b	#1,d0
			move.l	d0,_extra_data						; indicate ok (voice+1)

.l3			addq.w	#2,sp								; clean up stack
			bra		.l99									; done

*	PlaySound

.l1			cmp.w	#EXTRA_PLAYSOUND,d2					; PlaySound operation?
			bne		.l4									; no

*	a2		= sound block
*	a3/d3	= audio blocks
*	a5/d2	= voice data

			move.l	_extra_data,a2						; get extra data
			clr.l	_extra_data							; init result to false

			bsr		OpenMusic							; be sure driver active
			tst.l	d0
			bne		.l99									; if not zero, error

			move.w	sblk_Pan(a2),d1						; get pan value
			moveq	#1,d4								; default loop value = 1
			btst.l	#SOUNDB_LOOP,d1						; loop?
			beq.s	.l20

			move.w	d1,d4
			lsr.w	#8,d4								; get loop value

.l20			moveq	#LEFT_0,d0
			btst.l	#SOUNDB_RIGHT_SIDE,d1				; pan?
			beq.s	.l5									; no, use left
			moveq	#RIGHT_0,d0							; use right
.l5			and.w	#MUST_HAVE_SIDE,d1					; mask all but MUST bit
			or.w	#MUSIC_PRI_SOUND,d1					; priority 2 normally
			bsr		pick_voice
			tst.l	d0									; picked a voice?
			beq		.l99									; nope, done
			move.l	d0,a5

			moveq	#0,d3								; init iob2 = NULL
			moveq	#0,d2								; init voice2 = NULL

			move.l	_SysBase,a6
			move.l	_play_port,a0
			JSRLIB	GetMsg								; get an audio block
			tst.l	d0
			beq		.l99									; oops, none available??

			move.l	d0,a3								; set-up iob1

		ifne FASTSOUND
			move.l	sblk_Data(a2),a1
			JSRLIB	TypeOfMem							; is fastmem?
			btst.l	#MEMB_FAST,d0
			bne		.l40									; yep, play fastsound
		endc

			move.w	#CMD_WRITE,IO_COMMAND(a3)			; it's a write
			move.b	#ADIOF_PERVOL|IOF_QUICK,IO_FLAGS(a3)
			clr.b	IO_ERROR(a3)
			move.w	d4,ioa_Cycles(a3)
			move.w	sblk_Volume(a2),ioa_Volume(a3)
			move.w	sblk_Period(a2),ioa_Period(a3)
			moveq	#1,d1
			moveq	#0,d0
			move.b	voice_Number(a5),d0
			lsl.l	d0,d1
			move.l	d1,IO_UNIT(a3)
			move.l	sblk_Data(a2),ioa_Data(a3)
			move.l	sblk_Length(a2),ioa_Length(a3)

			exg.l	d3,a3								; makes things simpler
			move.l	a5,a0
			exg.l	d2,a5

			move.w	sblk_Pan(a2),d0						; check for stereo
			and.b	#SOUND_STEREO,d0
			cmp.b	#SOUND_STEREO,d0
			bne		.l6									; no, didn't want it

			move.b	voice_Number(a0),d0
			subq.w	#1,d0						; (v# - 1) & 2
			btst.l	#1,d0						; did we get LEFT side?
			bne		.l6							; wanted RIGHT, can't have stereo

			moveq	#LEFT_0,d0
			move.w	sblk_Pan(a2),d1
			and.w	#MUST_HAVE_SIDE,d1
			or.w	#MUSIC_PRI_SOUND,d1			; priority 2 normally
			bsr		pick_voice
			tst.l	d0
			beq.s	.l6

			move.l	d0,a5
			move.b	voice_Number(a5),d0
			subq.w	#1,d0						; (v# - 1) & 2
			btst.l	#1,d0						; did we get LEFT side?
			beq.s	.l6							; if 0, can't have stereo
			
			move.l	_SysBase,a6
			move.l	_play_port,a0
			JSRLIB	GetMsg								; get another audio block
			tst.l	d0
			beq.s	.l6									; oops, none available??

			move.l	d0,a3								; use iob2
			move.w	#CMD_WRITE,IO_COMMAND(a3)			; it's a write
			move.b	#ADIOF_PERVOL|IOF_QUICK,IO_FLAGS(a3)
			clr.b	IO_ERROR(a3)
			move.w	d4,ioa_Cycles(a3)
			move.w	sblk_Volume(a2),ioa_Volume(a3)
			move.w	sblk_Period(a2),ioa_Period(a3)
			moveq	#1,d1
			moveq	#0,d0
			move.b	voice_Number(a5),d0
			lsl.l	d0,d1
			move.l	d1,IO_UNIT(a3)
			move.l	sblk_Data(a2),ioa_Data(a3)
			move.l	sblk_Length(a2),ioa_Length(a3)

			tst.l	voice_Channel(a5)					; if voice in use, kill
			beq.s	.l7
			move.l	a5,a0
			bsr		KillVoice
.l7			bset.b	#VOICEB_BLOCKED,voice_Flags(a5)		; block voice

.l6			exg.l	a5,d2								; switch to voice 1
			exg.l	a3,d3								; switch to iob 1

			tst.l	voice_Channel(a5)					; if voice in use, kill
			beq.s	.l8
			move.l	a5,a0
			bsr		KillVoice
.l8			bset.b	#VOICEB_BLOCKED,voice_Flags(a5)		; block voice

			move.l	a3,a1								; send off iob 1
			move.l	_AudioDevice,a6
			JSRDEV	BEGINIO

			tst.l	d3									; stereo?
			beq.s	.l9									; no, skip
			move.l	d3,a1								; else, send off iob 2
			JSRDEV	BEGINIO

.l9			tst.b	IO_ERROR(a3)						; error on iob 1?
			bne.s	.l30									; no, skip

			move.l	a3,a1
			DMACHECK
			bra.s	.l10

.l30			move.l	a3,a1								; put iob on play_port
			move.l	_SysBase,a6
			JSRLIB	ReplyMsg

			sub.l	a3,a3								; indicate error
			bclr.b	#VOICEB_BLOCKED,voice_Flags(a5)		; unblock voice

.l10			tst.l	d3
			beq.s	.l11
			move.l	d3,a1
			tst.b	IO_ERROR(a1)						; error on iob 1?
			bne.s	.l32									; no, skip & do link

			DMACHECK
			bra.s	.l12

.l32			move.l	_SysBase,a6							; put iob on play_port
			JSRLIB	ReplyMsg

			moveq	#0,d3								; indicate error
			move.l	d2,a1
			bclr.b	#VOICEB_BLOCKED,voice_Flags(a1)		; unblock voice
			bra.s	.l11

.l12			move.l	d2,a1
			move.b	#MUSIC_PRI_SOUND,voice_Priority(a1) ; priority 2 normally
			move.b	voice_Number(a1),voice_Link(a1)

.l11			move.l	a3,d0								; or iob1 with iob2
			or.l	d3,d0
			beq		.l99									; zero, no sound made

			move.b	#MUSIC_PRI_SOUND,voice_Priority(a5) ; priority 2 normally
			move.b	voice_Number(a5),voice_Link(a5)		; link voices if stereo
			tst.l	d3
			beq.s	.l13
			move.l	d2,a0
			move.b	voice_Number(a0),voice_Link(a5)

.l13			move.l	_globaldata+glob_UniqueID,d0		; set-up UniqueID
			move.l	d0,d1
			or.b	voice_Number(a5),d0
			bset.l	#31,d0
			move.l	d0,voice_UniqueID(a5)
			move.l	d0,_extra_data

			addq.l	#4,d1								; increment UniqueID
			and.b	#$fc,d1								; safety net
			move.l	d1,_globaldata+glob_UniqueID
			bra		.l99

		ifne FASTSOUND
.l40
		endc

*	StopSound

.l4			cmp.w	#EXTRA_STOPSOUND,d2					; StopSound operation?
			bne		.l14									; no

			move.l	_extra_data,d0						; get voice # from ID
			move.l	d0,d1
			and.w	#3,d0
			SCALE	voice_sizeof,d0
			lea		_voice,a2
			add.w	d0,a2

			cmp.l	voice_UniqueID(a2),d1				; ID matches with voice
			bne.s	.l15									; nope, skip all

			btst.b	#VOICEB_BLOCKED,voice_Flags(a2)		; voice actually on?
			beq.s	.l16									; no, check for stereo

			moveq	#0,d0
			move.b	voice_Number(a2),d0
			bsr		stop_audio							; kill voice
			clr.b	voice_Status(a2)
			clr.b	voice_Flags(a2)
			clr.b	voice_Priority(a2)
			clr.l	voice_UniqueID(a2)

.l16			moveq	#0,d0
			move.b	voice_Link(a2),d0					; is link = number?
			cmp.b	voice_Number(a2),d0
			beq.s	.l15									; yes, so not stereo

			SCALE	voice_sizeof,d0
			lea		_voice,a2
			add.w	d0,a2

			btst.b	#VOICEB_BLOCKED,voice_Flags(a2)		; voice actually on?
			beq.s	.l15									; no, check for stereo

			moveq	#0,d0
			move.b	voice_Number(a2),d0
			bsr		stop_audio							; kill voice
			clr.b	voice_Status(a2)
			clr.b	voice_Flags(a2)
			clr.b	voice_Priority(a2)
			clr.l	voice_UniqueID(a2)

.l15			clr.l	_extra_data
			bra.w	.l99

*	CheckSound

.l14			cmp.w	#EXTRA_CHECKSOUND,d2				; CheckSound operation?
			bne.s	.l18									; no

			move.l	_extra_data,d0						; get voice # from ID
			move.l	d0,d1
			and.w	#3,d0
			SCALE	voice_sizeof,d0
			lea		_voice,a2
			add.w	d0,a2

			cmp.l	voice_UniqueID(a2),d1				; ID matches with voice
			bne.s	.l17									; nope, skip all

			move.l	a2,a0
			bsr		check_sound							; check sound on voice
			tst.l	d0
			beq.s	.l17									; no, return FALSE
			moveq	#1,d0
			move.l	d0,_extra_data						; yes, return TRUE
			bra.s	.l99

.l17			clr.l	_extra_data
			bra.s	.l99

*	AdvanceSong

.l18			cmp.w	#EXTRA_ADVANCE,d2					; CheckSound operation?
			bne.s	.l19									; no

			move.l	_extra_data,d0						; get advance #
			clr.l	_extra_data							; assume no music playing
			btst.b	#MUSICB_PLAYING,_globaldata+glob_Flags
			beq.s	.l99									; no music playing, skip

			bsr		advance_song
			moveq	#1,d0
			move.l	d0,_extra_data
			bra.s	.l99

*	CheckNote

.l19			cmp.w	#EXTRA_CHECKNOTE,d2				; CheckNote operation?
			bne.s	.l99									; no

			move.l	_extra_data,d0						; get voice # + 1
			subq.b	#1,d0
			and.w	#3,d0
			SCALE	voice_sizeof,d0
			lea		_voice,a2
			add.w	d0,a2

			move.l	voice_Channel(a2),a3
			cmp.b	#16,chan_Number(a3)
			bne.s	.l40

			move.l	a2,a0
			bsr		xcheck_sound						; check note on voice
			tst.l	d0
			beq.s	.l40									; no, return FALSE
			moveq	#1,d0
			move.l	d0,_extra_data						; yes, return TRUE
			bra.s	.l99

.l40			clr.l	_extra_data

.l99			movem.l	(sp)+,d2-d4/a2/a3/a5/a6
			moveq	#0,d0
			rts

*==========================================================================*
*
*	AllocSample - allocates & initialized a SampleData structure
*
*	struct SampleData *AllocSample(
*		struct SampleData	*prev_sample,
*		LONG				attacksize,
*		LONG				sustainsize)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_AllocSample
_AllocSample
			move.l	4(sp),a0							; previous sample
			move.l	8(sp),d0							; attack size
			move.l	12(sp),d1							; sustain size
		endc
AllocSample
			movem.l	d2/d3/a2/a3/a6,-(sp)
			move.l	a0,a2
			move.l	d0,d2
			move.l	d1,d3

			moveq	#samp_sizeof,d0						; allocate SampleData
			move.l	#MEMF_CLEAR,d1
			move.l	_SysBase,a6
			JSRLIB	AllocMem
			tst.l	d0
			beq.s	.l99
			move.l	d0,a3

			move.l	d2,d0								; allocate Waveform
			add.l	d3,d0
			move.l	#MEMF_CHIP,d1
			JSRLIB	AllocMem

			move.l	d0,samp_Waveform(a3)				; Waveform -> SampleData
			beq.s	.l97									; whoops... no memory
			
			move.l	d2,samp_AttackSize(a3)				; store sizes
			move.l	d3,samp_SustainSize(a3)
			move.l	a3,d0								; return SampleData

			move.l	a2,d1								; previous sample?
			beq.s	.l99									; no, done

			move.l	a3,samp_NextSample(a2)				; link to previous

.l99			movem.l (sp)+,d2/d3/a2/a3/a6
			rts

.l97			move.l	#samp_sizeof,d0						; free memory
			move.l	a3,a1
			JSRLIB	FreeMem
			moveq	#0,d0								; return error
			bra		.l99

*==========================================================================*
*
*	FreeSample - frees memory associated with a sample
*
*	void FreeSample(WORD number)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_FreeSample
_FreeSample
			move.w	4(sp),d0							; sample number
		endc
FreeSample
			movem.l	a2/a6,-(sp)	
			lea		_asample,a0							; calc ptr to sample
			add.w	d0,d0
			add.w	d0,d0
			add.w	d0,a0
			move.l	(a0),a2								; get sample ptr
			clr.l	(a0)								; clear saved ptr

			move.l	_SysBase,a6

.l2			move.l	a2,d0								; still have sample?
			beq.s	.l1									; no, done

			move.l	samp_Waveform(a2),a1
			move.l	samp_AttackSize(a2),d0
			add.l	samp_SustainSize(a2),d0
			JSRLIB	FreeMem

			move.l	samp_NextSample(a2),a1				; get next sample
			exg		a1,a2								; set-up next FreeMem
			moveq	#samp_sizeof,d0
			JSRLIB	FreeMem

			bra		.l2

.l1			movem.l	(sp)+,a2/a6
			rts

*==========================================================================*
*
*	FreePatch - frees memory associated with a patch
*
*	void FreePatch(WORD number)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_FreePatch
_FreePatch
			move.w	4(sp),d0							; patch number
		endc
FreePatch
			movem.l	a2/a6,-(sp)

			lea		_patch,a2							; calc addr of patch
			SCALE	patch_sizeof,d0
			add.w	d0,a2

			move.l	_SysBase,a6

			move.l	patch_Attack(a2),d0					; free attack envelopes
			beq.s	.l1
			move.l	d0,a1
			move.w	patch_AttackCount(a2),d0
			mulu.w	#env_sizeof,d0
			JSRLIB	FreeMem
			clr.l	patch_Attack(a2)

.l1			move.l	patch_Release(a2),d0				; free release envelopes
			beq.s	.l2
			move.l	d0,a1
			move.w	patch_ReleaseCount(a2),d0
			mulu.w	#env_sizeof,d0
			JSRLIB	FreeMem
			clr.l	patch_Release(a2)

.l2			clr.l	patch_Sample(a2)					; clear sample ptr

			movem.l	(sp)+,a2/a6
			rts

*==========================================================================*
*
*	UnloadPerf - unloads a MaxTrax performance
*
*	void UnloadPerf(WORD mode)
*
*==========================================================================*

			xdef	_UnloadPerf
_UnloadPerf
			move.l	4(sp),d0							; ALL/SCORE/SAMPLES

			xdef	UnloadPerf
UnloadPerf
			movem.l	d2/d3/a2/a6,-(sp)
			move.w	d0,d2

;			cmp.w	#PERF_PARTSAMPLES,d2				; partial samples only?
;			beq.s	.l9									; yes, don't unload

			cmp.w	#PERF_SCORE,d2
			beq.s	.l1									; if only scores, skip

			moveq	#NUM_SAMPLES-1,d3					; free all samples
.l2			move.w	d3,d0
			bsr		FreeSample
			dbra	d3,.l2

			moveq	#NUM_PATCHES-1,d3					; free all patches
.l3			move.w	d3,d0
			bsr		FreePatch
			dbra	d3,.l3

.l1			cmp.w	#PERF_SAMPLES,d2
			beq.s	.l9									; if only samples, skip

			move.l	_SysBase,a6
			move.l	_scoreptr,a2
			move.l	a2,_globaldata+glob_CurrentScore	; reset current score
			move.w	_globaldata+glob_TotalScores,d3		; get total scores
			bra.s	.l4

.l5			move.l	score_NumEvents(a2),d0				; any events?
			beq.s	.l6									; no, skip

			moveq	#cev_sizeof,d1						; calc size of events
			jsr		mulu
			move.l	score_Data(a2),a1					; free score
			JSRLIB	FreeMem
			clr.l	score_Data(a2)						; clear score structure
			clr.l	score_NumEvents(a2)

.l6			addq.w	#score_sizeof,a2					; next score
.l4			dbra	d3,.l5								; loop

			clr.w	_globaldata+glob_TotalScores
			clr.w	_maxtrax+mxtx_TotalScores
			move.w	#120,_globaldata+glob_Tempo
			move.w	#1,_globaldata+glob_Filter

.l9			movem.l	(sp)+,d2/d3/a2/a6
			rts

*==========================================================================*
*
*	LoadPerf - load a Music-X score
*
*	BOOL LoadPerf(char *name,WORD mode [,BPTR handle])
*
*==========================================================================*

CheckRead		macro

				pea		\2
				move.l	_maxtrax+mxtx_ReadFunc,-(sp)
				rts

\2				cmp.l	d0,d3
;			ifeq NARG-2
;				bne.s	\1
;			else
				bne		\1
;			endc
				endm
				
				xdef	_LoadPerf
_LoadPerf
				move.l	4(sp),a0							; filename
				move.l	8(sp),d0							; load mode
				move.l	12(sp),d1							; possible fh

				xdef	LoadPerf
LoadPerf
				movem.l	d2-d7/a2/a3/a5/a6,-(sp)
				sub.w	#dsamp_sizeof+8,sp					; local variables
															; +8 to fix CDTV
				move.l	a0,a2								; a2 = filename
				move.w	d0,d4								; d4 = mode
				move.l	d1,d7								; d7 = posible fh

				bsr		CloseMusic							; close down
				move.w	d4,d0
				bsr		UnloadPerf							; unload performance

				move.l	_DOSBase,a6

				move.l	a2,d1								; filename
				beq.s	.l30									; none! must have fh

				moveq	#0,d7								; no fh
				move.l	#MODE_OLDFILE,d2

				pea		.l32
				move.l	_maxtrax+mxtx_OpenFunc,-(sp)
				rts

.l32				tst.l	d0
				beq		.l99									; error! 

				move.l	d0,a2								; a2 = file handle
				bra.s	.l31

.l30				move.l	d7,a2								; a2 = file handle

.l31				move.l	a2,d1								; get file ID
				move.l	sp,d2
				moveq	#4,d3
				CheckRead .l98,.l70

				cmp.l	#'MXTX',(sp)						; a MaxTrax file?
				bne		.l98									; nope, error

				move.l	a2,d1								; get tempo & filter
				move.l	sp,d2
				moveq	#4,d3
				CheckRead .l98,.l71
				
				cmp.w	#PERF_SAMPLES,d4					; samples only?
				beq.s	.l1									; yes, skip
;				cmp.w	#PERF_PARTSAMPLES,d4				; samples only?
;				beq.s	.l1									; yes, skip

				move.w	(sp),_globaldata+glob_Tempo			; set tempo & filter
				move.w	2(sp),d0
				move.w	d0,d1
				and.w	#1,d0								; low bit is filter
				move.w	d0,_globaldata+glob_Filter

				bclr.b	#MUSICB_VELOCITY,_globaldata+glob_Flags
				bclr.b	#MUSICB_VELOCITY,_maxtrax+mxtx_Flags
				btst.l	#1,d1								; attack volume?
				beq.s	.l20
				bset.b	#MUSICB_VELOCITY,_globaldata+glob_Flags
				bset.b	#MUSICB_VELOCITY,_maxtrax+mxtx_Flags

.l20
			ifne HAS_MICROTONAL
				btst.l	#15,d1								; has microtonal?
				beq.s	.l12

				move.l	a2,d1								; read in table
				move.l	#_microtonal,d2
				moveq	#2+128,d3
				CheckRead .l98,.l72

				bra		.l12
			endc

.l1				btst.l	#15,d1								; has microtonal?
				beq.s	.l12

				move.l	a2,d1								; seek past data
				move.l	#2*128,d2
				move.l	#OFFSET_CURRENT,d3
				JSRLIB	Seek

.l12				move.l	a2,d1								; get # of scores
				move.l	sp,d2
				moveq	#2,d3
				CheckRead .l98,.l73

				move.w	(sp),d5								; use d5 as counter
				move.l	_scoreptr,a3
				bra.w	.l2

.l3				move.l	a2,d1								; get # of events
				move.l	sp,d2
				moveq	#4,d3
				CheckRead .l98,.l74
				move.l	(sp),d0
				moveq	#cev_sizeof,d1						; x CookedEvent size
				jsr		mulu
				move.l	d0,4(sp)							; save locally

				cmp.w	#PERF_SAMPLES,d4					; samples only?
				beq.s	.l14									; no, do load
;				cmp.w	#PERF_PARTSAMPLES,d4				; samples only?
;				beq.s	.l14									; no, do load

				move.w	_scoremax,d1
				cmp.w	_globaldata+glob_TotalScores,d1 	; too many scores?
				bne.s	.l4									; no, do load

.l14				move.l	a2,d1								; seek past data
				move.l	d0,d2
				move.l	#OFFSET_CURRENT,d3
				JSRLIB	Seek
				bra.s	.l2

.l4				move.l	#MEMF_CLEAR,d1						; get d0 size bytes
				move.l	_SysBase,a6
				JSRLIB	AllocMem
				tst.l	d0
				beq		.l98

				move.l	d0,score_Data(a3)					; event data
				move.l	(sp),score_NumEvents(a3)			; number (on stack)
				addq.w	#1,_globaldata+glob_TotalScores
				addq.w	#1,_maxtrax+mxtx_TotalScores

				move.l	a2,d1								; load events
				move.l	d0,d2
				move.l	4(sp),d3
				move.l	_DOSBase,a6
				CheckRead .l98,.l75

				addq.w	#score_sizeof,a3					; next score
.l2				dbra	d5,.l3

				cmp.w	#PERF_SCORE,d4						; score only?
				beq		.l80									; yes, no samples

				move.l	a2,d1								; get # of samples
				move.l	sp,d2
				moveq	#2,d3
				CheckRead .l98,.l76
				
				move.w	(sp),d5								; number of samples
				bra		.l5

.l6				move.l	a2,d1								; get sample header
				move.l	sp,d2
				moveq	#dsamp_sizeof,d3
				CheckRead .l98,.l78

				lea		_asample,a3							; ptr to sample
				move.w	dsamp_Number(sp),d0
				move.w	d0,d1								; save # in d1
				add.w	d0,d0								; entries are ptrs
				add.w	d0,d0
				add.w	d0,a3
				
				lea		_patch,a5							; ptr to patch
				SCALE	patch_sizeof,d1					; patch size * number
				add.w	d1,a5

				move.w	dsamp_Tune(sp),patch_Tune(a5)		; copy values
				move.w	dsamp_Volume(sp),patch_Volume(a5)

				move.w	dsamp_AttackCount(sp),d6			; # attack env segs
				mulu.w	#env_sizeof,d6						; memory needed
				move.l	d6,d0
				moveq	#0,d1
				move.l	_SysBase,a6
				JSRLIB	AllocMem
				tst.l	d0
				beq		.l98

				move.l	d0,patch_Attack(a5)					; save data
				move.w	dsamp_AttackCount(sp),patch_AttackCount(a5)

				move.l	a2,d1								; read sample
				move.l	d0,d2
				move.l	d6,d3
				move.l	_DOSBase,a6
				CheckRead .l98,.l79

				move.w	dsamp_ReleaseCount(sp),d6			; # release env segs
				mulu.w	#env_sizeof,d6						; memory needed
				move.l	d6,d0
				moveq	#0,d1
				move.l	_SysBase,a6
				JSRLIB	AllocMem
				tst.l	d0
				beq		.l98

				move.l	d0,patch_Release(a5)				; save data
				move.w	dsamp_ReleaseCount(sp),patch_ReleaseCount(a5)

				move.l	a2,d1								; read sample
				move.l	d0,d2
				move.l	d6,d3
				move.l	_DOSBase,a6
				CheckRead .l98,.l60

				moveq	#0,d6								; for each octave...
.l9				tst.w	dsamp_Octaves(sp)
				beq.s	.l5

				move.l	d6,a0								; alloc sample space
				move.l	dsamp_AttackLength(sp),d0
				move.l	dsamp_SustainLength(sp),d1
				bsr		AllocSample
				move.l	d0,d6
				beq		.l98									; opps, error

				tst.l	(a3)								; first sample?
				bne.s	.l8									; no, skip ahead

				move.l	d6,(a3)								; set sample ptr
				move.l	d6,patch_Sample(a5)					; set sample in patch

.l8				move.l	a2,d1								; load sample
				move.l	d6,a0
				move.l	samp_Waveform(a0),d2
				move.l	dsamp_AttackLength(sp),d3
				add.l	dsamp_SustainLength(sp),d3
				move.l	_DOSBase,a6
				CheckRead .l98,.l61

				move.l	dsamp_AttackLength(sp),d2
				add.l	d2,dsamp_AttackLength(sp)
				move.l	dsamp_SustainLength(sp),d2
				add.l	d2,dsamp_SustainLength(sp)

				subq.w	#1,dsamp_Octaves(sp)		; decrement octave count
				bra.s	.l9									; loop

.l5				dbra	d5,.l6								; get more samples

				tst.l	d7									; fh?
				bne.s	.l33									; don't close it

				move.l	a2,d1
				move.l	_DOSBase,a6
				pea		.l33
				move.l	_maxtrax+mxtx_CloseFunc,-(sp)
				rts

.l33				moveq	#NUM_PATCHES-1,d2
				lea		_patch,a2
.l10				tst.l	patch_Sample(a2)
				bne.s	.l11
				lea		patch_sizeof(a2),a2
				dbra	d2,.l10

.l11				moveq	#1,d0
				tst.w	d2
				bpl.s	.l99
				moveq	#0,d0

.l99				add.w	#dsamp_sizeof+8,sp					; local variables
				movem.l	(sp)+,d2-d7/a2/a3/a5/a6
				rts

.l80				tst.l	d7									; fh?
				bne.s	.l81									; don't close it

				move.l	a2,d1								; scores only cleanup
				move.l	_DOSBase,a6
				pea		.l81
				move.l	_maxtrax+mxtx_CloseFunc,-(sp)
				rts

.l81				moveq	#1,d0
				bra.s	.l99

.l98				tst.l	d7									; fh?
				bne.s	.l97									; don't close it

				move.l	a2,d1								; error cleanup
				move.l	_DOSBase,a6
				pea		.l97
				move.l	_maxtrax+mxtx_CloseFunc,-(sp)
				rts

.l97				moveq	#0,d0
				bra.s	.l99

StdOpenFunc
				JSRLIB	Open
				rts

StdReadFunc
				JSRLIB	Read
				rts

StdCloseFunc
				JSRLIB	Close
				rts

*==========================================================================*
*
*	quick tag finder -- taglist:a0 id:d0, result tagitem:d0
*
*==========================================================================*

FindTag			move.l	(a0),d1
				beq.s	.l99
				cmp.l	d0,d1
				beq.s	.l1
				addq.w	#8,a0
				bra.s	FindTag
.l1				move.l	a0,d0
				rts
.l99				moveq	#0,d0
				rts

*==========================================================================*
*
*	Glue for the three interrupts.
*
*==========================================================================*

* made this as fast as possible if music system off...

MusicVBlank		tst.l	_AudioDevice		; if audio system off, exit
				bne.s	.l1
				moveq	#0,d0				; continue chain
				rts

.l1				move.l	_SysBase,a6
				lea		_music_server,a1
				jsr		_LVOCause(a6)		; Cause a softint at IMusicServer...
				moveq	#0,d0				; continue chain
				rts

IMusicServer	bsr		MusicServer			; call music server
				moveq	#0,d0				; continue chain
				rts

IExtraServer	bsr		ExtraServer			; call extra server
				moveq	#0,d0				; continue chain
				rts

*==========================================================================*
*	Debugging code
*==========================================================================*

			ifne	DEBUG
knum			movem.l	d0/d1/a0/a1,-(sp)
				move.l	20(sp),d0
				move.l	d0,-(sp)
				pea		knumtext
				xref	_kprintf
				jsr		_kprintf
				addq.w	#8,sp
				movem.l	(sp)+,d0/d1/a0/a1
				rts

knumtext		dc.b	'NUM %ld',10,0
				ds.w	0

knum3			movem.l	d0/d1/a0/a1,-(sp)
				move.l	20(sp),d0
				move.l	24(sp),d1
				move.l	28(sp),a0
				movem.l	d0/d1/a0,-(sp)
				pea		knum3text
				xref	_kprintf
				jsr		_kprintf
				lea		16(sp),sp
				movem.l	(sp)+,d0/d1/a0/a1
				rts

knum3text		dc.b	'DATA: %ld,%ld,%ld',10,0
				ds.w	0

knum3s			movem.l	d0/d1/a0/a1,-(sp)
				move.l	20(sp),d0
				move.l	24(sp),d1
				move.l	28(sp),a0
				movem.l	d0/d1/a0,-(sp)
				pea		knum3stext
				xref	_kprintf
				jsr		_kprintf
				lea		16(sp),sp
				movem.l	(sp)+,d0/d1/a0/a1
				rts

knum3stext		dc.b	'STOLE: %ld,%ld,%ld',10,0
				ds.w	0

knum3e			movem.l	d0/d1/a0/a1,-(sp)
				move.l	20(sp),d0
				move.l	24(sp),d1
				move.l	28(sp),a0
				movem.l	d0/d1/a0,-(sp)
				pea		knum3etext
				xref	_kprintf
				jsr		_kprintf
				lea		16(sp),sp
				movem.l	(sp)+,d0/d1/a0/a1
				rts

knum3etext		dc.b	'ENV: %ld,%ld,%ld   ',0
				ds.w	0
			endc

			end
