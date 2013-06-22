*==========================================================================*
*   MaxTrax Music Player - audio device handler (shared module)            *
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

*==========================================================================*
*
*	CalcVolume - calculate current volume for a voice
*
*	UWORD CalcVolume(struct VoiceData *v)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_CalcVolume
_CalcVolume
			move.l	4(sp),a0						; voice
		endc
CalcVolume
		ifne IN_MUSICX
			moveq	#64,d0
		else
			lea		_globaldata,a1					; ptr to global data
			move.w	glob_Volume(a1),d0				; get global volume
		endc
			move.w	voice_NoteVolume(a0),d1			; get attack volume
			cmp.w	#128,d1							; 128 is max volume
			bpl.s	.l1								; if vol >= 128, skip
			mulu.w	d1,d0							; multiply into volume
			lsr.w	#7,d0							; scale by 128

* experiment: only consider the top byte of envelope vol 

.l1			moveq	#0,d1
			move.b	voice_BaseVolume(a0),d1			; get envelope volume
			cmp.w	#128,d1							; $80 is max volume
			bpl.s	.l2								; if vol > $80, skip
			mulu.w	d1,d0							; multiply into volume
			lsr.w	#7,d0							; scale by $80

.l2
		ifne HAS_FULLCHANVOL
			move.l	voice_Channel(a0),a1			; get channel
			moveq	#0,d1
			move.b	chan_Volume(a1),d1				; get channel volume
			bmi.s	.l3								; if vol >= 128, skip
			mulu.w	d1,d0							; multiply into volume
			lsr.w	#7,d0							; scale by 128
		endc

.l3			cmp.w	#65,d0							; if vol < 65, done
			bmi.s	.l4
			moveq	#64,d0							; else set to 64
.l4			move.b	d0,voice_LastVolume(a0)			; save in LastVolume
			rts

*==========================================================================*
*
*	voice picking routine
*
*	struct VoiceData *pick_voice(UWORD pick,WORD pri)
*
*==========================================================================*

LEFT_0			equ		0
RIGHT_0			equ		1
RIGHT_1			equ		2
LEFT_1			equ		3

SIBLING_SIDE	macro
			eor.w	#3,\1
				endm

OTHER_SIDE		macro
			eor.w	#1,\1
				endm

BEST			macro	
			cmp.b	\1,\2				; test if dd > ds

			blt.s	\@MIN1				; yes, ds has right value (unsigned)
			move.b	\1,\2				; move dd into ds
\@MIN1
				endm

right_round	dc.b	0								; bounce control
left_round	dc.b	0

		ifne C_ENTRY
			DEFX	_pick_voice
_pick_voice
			move.w	4(sp),d0						; get initial voice number
			move.w	6(sp),d1						; get pri
		endc
pick_voice
			movem.l	a2/d2-d5,-(sp)

		ifne IN_MUSICX
			lea		_voice_table,a0					; load addr of voice table
		else
			lea		_voice,a0						; load addr of voice table
		endc

			move.w	d1,d5							; save pri in d5

		ifeq IN_MUSICX
			bclr.l	#MUSTB_HAVE_SIDE,d5				; clear MUST bit
			bclr.l	#MUSTB_HAVE_SIDE,d1				; test and clear MUST bit
			bne.s	pv1								; if was set, skip
		endc

			move.w	d0,d1
			OTHER_SIDE	d1							; get other side vice num

			move.w	d0,d2							; find 'best' of siblings
			SCALE	voice_sizeof,d2
			move.b	voice_Status(a0,d2.w),d2
			move.w	d0,d3
			SIBLING_SIDE d3
			SCALE	voice_sizeof,d3
			move.b	voice_Status(a0,d3.w),d3
			BEST	d3,d2
			move.b	d2,d4							; save vnear -> d4

			move.w	d1,d2							; find 'best' of 'others'
			SCALE	voice_sizeof,d2
			move.b	voice_Status(a0,d2.w),d2
			move.w	d1,d3
			SIBLING_SIDE d3
			SCALE	voice_sizeof,d3
			move.b	voice_Status(a0,d3.w),d3
			BEST	d3,d2							; vfar -> d2

			cmp.b	#ENV_RELEASE,d4					; is vnear <= ENV_RELEASE
			ble.s	pv1								; yes, so no change

			cmp.b	#ENV_RELEASE,d2					; is vfar > ENV_RELEASE
			bgt.s	pv1								; yes, so no change

			move.w	d1,d0							; other side better pick

* now we know which side it's going to be on, left or right
* now choose between 0 and 1

pv1			jsr		pick_voice1						; pick from siblings

			move.l	d0,d2							; get pointer to that voice
			SCALE	voice_sizeof,d2
			lea		0(a0,d2.w),a1

		ifeq IN_MUSICX
			btst.b	#VOICEB_BLOCKED,voice_Flags(a1)	; if blocked, pick another
			bne.s	pv3

			cmp.b	voice_Priority(a1),d5			; compare to voice priority
			bpl.s	pv99							; if v->Pri <= pri, OK
			
pv3			SIBLING_SIDE d0							; prob, so try other sibling
			move.l	d0,d2							; get pointer to that voice
			SCALE	voice_sizeof,d2
			lea		0(a0,d2.w),a1

			btst.b	#VOICEB_BLOCKED,voice_Flags(a1)	; if blocked, pick another
			bne.s	pv4

			cmp.b	voice_Priority(a1),d5			; compare to voice priority
			bpl.s	pv99							; if v->Pri <= pri, OK

* ick, so go to OTHER_SIDE

pv4			OTHER_SIDE d0
			jsr		pick_voice1						; pick from siblings

			move.l	d0,d2							; get pointer to that voice
			SCALE	voice_sizeof,d2
			lea		0(a0,d2.w),a1

			btst.b	#VOICEB_BLOCKED,voice_Flags(a1)	; if blocked, pick another
			bne.s	pv5

			cmp.b	voice_Priority(a1),d5			; compare to voice priority
			bpl.s	pv99							; if v->Pri <= pri, OK
			
pv5			SIBLING_SIDE d0							; prob, so try LAST VOICE
			move.l	d0,d2							; get pointer to that voice
			SCALE	voice_sizeof,d2
			lea		0(a0,d2.w),a1

			btst.b	#VOICEB_BLOCKED,voice_Flags(a1)	; if blocked, pick another
			bne.s	pv6

			cmp.b	voice_Priority(a1),d5			; compare to voice priority
			bpl.s	pv99							; if v->Pri <= pri, OK

* total failure!!

pv6			moveq	#0,d0							; no voice found
			bra.s	pv98

* success...

pv99
		endc
			and.b	#2,d0							; a2 has round_ptr
			move.b	d0,(a2)							; set-up next bounce val
			move.l	a1,d0							; return voice found

pv98		movem.l	(sp)+,a2/d2-d5					; done!
			rts

* this subroutine chooses between two voices on same side

pick_voice1
			lea		right_round,a2					; assume use right_round
			move.w	d0,d1							; decide which round
			subq.w	#1,d1
			btst.l	#1,d1
			beq.s	.l2
			addq.w	#1,a2							; use left_round

.l2			move.l	d0,d2
			SCALE	voice_sizeof,d2
			move.b	voice_Status(a0,d2.w),d2
;			move.b	voice_LastVolume(a0,d2.w),d4	; "pick" voice volume

			move.w	d0,d1
			SIBLING_SIDE d1
			move.w	d1,d3
			SCALE	voice_sizeof,d3
			sub.b	voice_Status(a0,d3.w),d2
			bmi.s	.l3								; if other higher, use pick
			bne.s	.l4								; if pick high, use other

;			cmp.b	voice_LastVolume(a0,d3.w),d4	; compare w/"other" volume
;			bgt		.l4						; if pick vol > other vol, use other 

			move.b	d0,d3							; voice #
			and.b	#2,d3							; check on same side
			cmp.b	(a2),d3							; break tie, get round data
			bne.s	.l3								; if not same, use pick
.l4			move.w	d1,d0							; switch to other

.l3			rts

*==========================================================================*
*
*	CalcNote - calculate note # and period for voice
*
*	UWORD CalcNote(struct VoiceData *v)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_CalcNote
_CalcNote
			move.l	4(sp),a0						; get voice pointer
		endc
CalcNote
			movem.l	a2/a3/d2/d3,-(sp)				; save regs

			move.l	voice_Channel(a0),a2			; get channel pointer
			move.w	chan_RealBend(a2),d2			; get adj bend value
			moveq	#0,d3

			clr.w	voice_LastPeriod(a0)			; set to error value

			btst.b	#VOICEB_PORTAMENTO,voice_Flags(a0)	; is there a portamento
			beq.s	.l1

		ifne HAS_MICROTONAL
			btst.b	#CHANB_MICROTONAL,chan_Flags(a2)	; microtonal active?
			beq		.l2
			lea		_microtonal,a1					; addr of microtonal table
			moveq	#0,d0
			move.b	voice_EndNote(a0),d0			; note we're going to
			add.w	d0,d0							; access as word
			move.w	0(a1,d0.w),d1					; real microtonal value
			move.b	voice_BaseNote(a0),d0			; note we're going from
			add.w	d0,d0							; access as word
			sub.w	0(a1,d0.w),d1					; subract microtonal value
			ext.l	d1
			move.l	voice_PortaTicks(a0),d0			; current PortaTicks
			move.l	a0,-(sp)
			jsr		mulu							; delta m'tonal * p'ticks
			move.l	(sp)+,a0
			tst.l	d0
			bpl		.l22
			neg.l	d0
			lsr.l	#8,d0							; divide neg # by 256
			neg.l	d0
			bra		.l23
.l22			lsr.l	#8,d0							; divide by 256
.l23			divs.w	chan_PortamentoTime(a2),d0		; divide by PortamentoTime
			move.w	d0,d3							; save extra bend value
			bra		.l1
.l2
		endc

			move.b	voice_EndNote(a0),d1			; note we're going to
			sub.b	voice_BaseNote(a0),d1			; note we're going from
			ext.w	d1
			ext.l	d1
			move.l	voice_PortaTicks(a0),d0			; current PortaTicks
			move.l	a0,-(sp)
			jsr		mulu							; delta note * PortaTicks
			move.l	(sp)+,a0
			divs.w	chan_PortamentoTime(a2),d0		; divide by PortamentoTime
			move.w	d0,d3							; save extra bend value

.l1
		ifne HAS_MODULATION
			tst.w	chan_Modulation(a2)				; modulation?
			beq.s	.l80								; no...

			btst.b	#CHANB_MODTYPE,chan_Flags(a2)	; period modulation?
			bne.s	.l80								; no...

			move.l	_globaldata+glob_SineValue,d0	; calc table index
			lsr.l	#8,d0							; / 256

			moveq	#0,d1
			move.w	chan_ModulationTime(a2),d1		; ModTime

			move.l	a0,-(sp)
			jsr		modu							; (SineValue >> 8) % ModTime
			move.l	(sp)+,a0

			lsl.l	#8,d0							; * 256
			divu.w	chan_ModulationTime(a2),d0		; / ModTime

			lea		_sine_table,a1
			move.b	0(a1,d0.w),d1
			ext.w	d1
			add.w	d1,d1
			muls.w	chan_Modulation(a2),d1
			divs.w	#127,d1
			add.w	d1,d3
.l80
		endc

			ext.l	d3
			ext.l	d2
			add.l	d3,d2							; tone = bend + mod

			moveq	#0,d0
			move.b	voice_BaseNote(a0),d0			; get base note
		ifne HAS_MICROTONAL
			btst.b	#CHANB_MICROTONAL,chan_Flags(a2)	; microtonal active?
			beq		.l6

			lea		_microtonal,a1					; addr of microtonal table
			add.w	d0,d0							; access as word
			moveq	#0,d1
			move.w	0(a1,d0.w),d1					; add microtonal to tone
			add.l	d1,d2
			bra		.l7
.l6
		endc

			lsl.w	#8,d0
			add.l	d0,d2							; tone += note << 8

* really should be pre-calculate this

.l7			move.l	voice_Patch(a0),a1				; get patch pointer
			move.w	patch_Tune(a1),d0				; add tuning to period
			ext.l	d0
			lsl.l	#8,d0
			divs.w	#24,d0							; (tune * 256) / 24
			ext.l	d0
			add.l	d0,d2							; add to tone

			sub.l	#(45<<8),d2						; based on midi note #45

			add.l	d2,d2							; try to do math w/o .divs#
			add.l	d2,d2
			divs.w	#3,d2
			ext.l	d2
			lsl.l	#4,d2							; ((tone << 2) / 3) << 4

			neg.l	d2
			add.l	#K_VALUE,d2						; logperiod = K - 'tone'

			move.l	patch_Sample(a1),a3				; get sample pointer

			btst.b	#VOICEB_RECALC,voice_Flags(a0)	; a recalc?
			bne.s	.l30								; yep, skip ahead

			clr.l	voice_PeriodOffset(a0)			; clear period offset

.l13			cmp.l	#PREF_PERIOD,d2					; is <= PREF_PERIOD
			ble.s	.l12								; yes, so don't shift

			move.l	samp_NextSample(a3),d1			; is this last sample?
			beq.s	.l12								; yes, can't shift any more

			move.l	d1,a3
			add.l	#$10000,voice_PeriodOffset(a0)	; remember logperiod shift
			sub.l	#$10000,d2						; adjust logperiod
			bra		.l13

.l30			sub.l	voice_PeriodOffset(a0),d2		; directly change period

.l12			cmp.l	#PERIOD_LIMIT,d2				; logperiod < PERIOD_LIMIT?
			blt.s	.l99								; oops, bad period, error

.l14			move.l	a0,a2							; preserve across call
			move.l	d2,d0
			bsr		IntAlg
			move.w	d0,voice_LastPeriod(a2)

.l99			move.l	a3,d0							; return sample to play
			movem.l	(sp)+,a2/a3/d2/d3				; restore regs
			rts										; done!

*==========================================================================*
*
*	EnvelopeManager - handles envelopes of playing notes
*
*	void EnvelopeManager(ULONG delta)
*
*	LONG calc_incrvol(LONG delta_volume,UWORD time)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX _calc_incrvol
_calc_incrvol
			move.l	4(sp),d0						; delta
			move.w	6(sp),d1						; time
		endc
calc_incrvol
			movem.l	d2/d3,-(sp)
			tst.w	d1								; is time zero?
			beq.s	.l3								; yes, just return delta

			move.l	d0,d2							; save original delta
			bpl.s	.l2								; if positive, ok
			neg.l	d0								; else negate

.l2			move.w	d0,d3							; save it
			mulu.w	#1000,d0						; 1000 * delta
		ifne IN_MUSICX
			mulu.w	#60,d1
		else
			mulu.w	_globaldata+glob_Frequency,d1	; time * frequency
		endc
			jsr		divu							; (1000 * delta)/(time * freq)
			cmp.w	d0,d3							; compare incrvol & delta
			bmi.s	.l1								; time too short, go special

			tst.l	d2								; was delta negative?
			bpl.s	.l3								; no, skip			
			neg.l	d0								; else negate incrvol

.l3			movem.l	(sp)+,d2/d3
			rts

.l1			move.l	d2,d0							; get saved delta
			bra		.l3

DoOneVoice
			COLOR0	$0fff
		ifne IN_MUSICX
			tst.b	voice_Status(a2)
			beq		.l19
		endc
			move.l	voice_Channel(a2),d0			; any note playing?
			beq		.l19								; no, skip
			move.l	d0,a3

			add.l	d2,voice_LastTicks(a2)			; add delta to LastTicks

			cmp.b	#ENV_SUSTAIN,voice_Status(a2)	; in sustain?
			bne.s	.l2								; no, skip ahead

			btst.b	#CHANB_ALTERED,chan_Flags(a3)	; channel altered?
			bne		.l18								; yes, must do calc's
			btst.b	#VOICEB_PORTAMENTO,voice_Flags(a2)	; portamento on?
			bne		.l18								; yes, must do calc's
		ifne HAS_MODULATION
			tst.w	chan_Modulation(a3)				; modulation on?
			bne		.l18								; yes, must do calc's
		endc
			bra		.l19								; otherwise, skip

.l2			cmp.b	#ENV_HALT,voice_Status(a2)		; halt voice?
			bne.s	.l3								; no, skip ahead

			move.l	a2,a0
			bsr		KillVoice						; kill voice
			bra		.l19								; go to next voice

.l3			move.l	voice_Patch(a2),a5				; get patch

			cmp.b	#ENV_START,voice_Status(a2)		; starting a voice?
			bne.s	.l4								; no, check another status

			move.l	patch_Attack(a5),a0				; envelope
			move.l	a0,voice_Envelope(a2)			; put attack env in voice
			beq.s	.l5								; but there is none...

													; initialize voice
			move.w	patch_AttackCount(a5),voice_EnvelopeLeft(a2)
			moveq	#0,d0
			move.w	env_Duration(a0),d0				; get duration (a0 = env)
			lsl.l	#8,d0							; shift left 8
			move.l	d0,voice_TicksLeft(a2)			; save as TicksLeft
			move.b	#ENV_ATTACK,voice_Status(a2)	; in attack envelope
			move.l	d2,voice_LastTicks(a2)			; reset LastTicks to delta
			moveq	#0,d0
			move.w	env_Volume(a0),d0				; get volume (a0 = env)
			move.w	env_Duration(a0),d1				; get duration (a0 = env)
			bsr		calc_incrvol					; calc volume delta
			move.l	d0,voice_IncrVolume(a2)			; and save in voice
			bra.s	.l9								; do envelope management

.l5			move.b	#ENV_SUSTAIN,voice_Status(a2)	; in sustain
			move.w	patch_Volume(a5),voice_BaseVolume(a2)	; set base volume
			move.l	d2,voice_LastTicks(a2)			; reset LastTicks to delta
			bra		.l18								; but do calc's			

.l4			cmp.b	#ENV_RELEASE,voice_Status(a2)	; releasing a voice?
			bne.s	.l9								; no, so do env management

			move.l	patch_Release(a5),a0			; envelope
			move.l	a0,voice_Envelope(a2)			; put release env in voice
			bne.s	.l6								; and there is one...

			move.b	#ENV_HALT,voice_Status(a2)		; set to halt
			moveq	#0,d4							; set new volume to zero
			bra		.l17								; send audio request

.l6			move.w	patch_ReleaseCount(a5),voice_EnvelopeLeft(a2)
			moveq	#0,d0
			move.w	env_Duration(a0),d0				; get duration (a0 = env)
			lsl.l	#8,d0							; shift left 8
			move.l	d0,voice_TicksLeft(a2)			; save as TicksLeft
			move.b	#ENV_DECAY,voice_Status(a2)		; in release envelope
			move.l	d2,voice_LastTicks(a2)				; reset LastTicks to delta
			moveq	#0,d0
			move.w	env_Volume(a0),d0				; get volume (a0 = env)
			moveq	#0,d1
			move.w	voice_BaseVolume(a2),d1			; subtract current volume
			sub.l	d1,d0
			move.w	env_Duration(a0),d1				; get duration (a0 = env)
			bsr		calc_incrvol					; calc volume delta
			move.l	d0,voice_IncrVolume(a2)			; and save in voice

.l9			cmp.b	#ENV_SUSTAIN,voice_Status(a2)	; need to do env management?
			beq		.l18								; none, skip

			cmp.l	voice_TicksLeft(a2),d2			; compare delta & time left
			bmi.s	.l10								; time left, just count down

			move.l	voice_Envelope(a2),a0			; copy final volume from env
			move.w	env_Volume(a0),voice_BaseVolume(a2)	; into voice

			subq.w	#1,voice_EnvelopeLeft(a2)		; decrement envelope count
			bne.s	.l11								; still > 0, keep status

													; no envelopes left
			cmp.b	#ENV_DECAY,voice_Status(a2)		; was doing release?
			bne.s	.l12								; no, was attack

			move.b	#ENV_HALT,voice_Status(a2)		; set to halt
			moveq	#0,d4							; set new volume to zero
			bra		.l17								; send audio request

.l12			move.b	#ENV_SUSTAIN,voice_Status(a2)	; start sustain
			move.l	d2,voice_LastTicks(a2)			; reset last ticks
			bra.s	.l18

.l11			addq.w	#4,a0							; next envelope segment
			move.l	a0,voice_Envelope(a2)
			moveq	#0,d0
			move.w	env_Duration(a0),d0				; get duration (a0 = env)
			lsl.l	#8,d0							; shift left 8
			move.l	d0,voice_TicksLeft(a2)			; save as TicksLeft
			moveq	#0,d0
			move.w	env_Volume(a0),d0				; get volume (a0 = env)
			moveq	#0,d1
			move.w	voice_BaseVolume(a2),d1			; subtract current volume
			sub.l	d1,d0
			move.w	env_Duration(a0),d1				; get duration (a0 = env)
			bsr		calc_incrvol					; calc volume delta
			move.l	d0,voice_IncrVolume(a2)			; and save in voice
			bra.s	.l18			

.l10			moveq	#0,d0							; calc new base volume
			move.w	voice_BaseVolume(a2),d0
			add.l	voice_IncrVolume(a2),d0
			bmi.s	.l13
			cmp.l	#$00008000,d0
			bmi.s	.l14
			move.w	#$8000,d0
			bra.s	.l14
.l13			moveq	#0,d0
.l14			move.w	d0,voice_BaseVolume(a2)			; save new base volume
			sub.l	d2,voice_TicksLeft(a2)			; subtract delta from time

.l18			move.l	a2,a0
			bsr		CalcVolume						; calc new volume
			move.w	d0,d4

			btst.b	#VOICEB_PORTAMENTO,voice_Flags(a2)	; portamento going?
			beq.s	.l16								; no, so skip

			add.l	d2,voice_PortaTicks(a2)			; add delta to porta time
			move.l	voice_PortaTicks(a2),d0
			lsr.l	#8,d0
			cmp.w	chan_PortamentoTime(a3),d0		; compare portamento times
			bmi.s	.l20								; portamento not over

			bclr.b	#VOICEB_PORTAMENTO,voice_Flags(a2)	; clear portamento flag
			move.b	voice_EndNote(a2),voice_BaseNote(a2)	; set base note
			bra.s	.l20

.l16			
		ifne HAS_MODULATION
			tst.w	chan_Modulation(a3)				; if modulation, calc note
			bne.s	.l20
		endc

			btst.b	#CHANB_ALTERED,chan_Flags(a3)	; channel altered?
			beq.s	.l17								; no, don't recalc note

.l20			bset.b	#VOICEB_RECALC,voice_Flags(a2)	; its a recalc
			move.l	a2,a0
			bsr		CalcNote

.l17			move.l	_audio_env,a1					; get envelope audio block
			move.b	#IOF_QUICK,IO_FLAGS(a1)			; a quick action
			clr.b	IO_ERROR(a1)					; clear error
			moveq	#0,d0
			move.b	voice_Number(a2),d0
			moveq	#1,d1
			lsl.l	d0,d1
			move.l	d1,IO_UNIT(a1)					; unit = 1 << voice#
			move.w	voice_LastPeriod(a2),d1			; test LastPeriod
			beq.s	.l21								; if zero, special meaning
			move.w	d1,ioa_Period(a1)
			move.w	d4,ioa_Volume(a1)
			bra.s	.l22
.l21			move.w	#1000,ioa_Period(a1)
			clr.w	ioa_Volume(a1)

.l22			move.l	_AudioDevice,a6
			JSRDEV	BEGINIO

.l19			COLOR0	$0777
			rts

		ifne C_ENTRY
			DEFX	_EnvelopeManager
_EnvelopeManager
			move.l	4(sp),d0						; delta
		endc
EnvelopeManager
			movem.l	d2/d3/d4/a2/a3/a5/a6,-(sp)
			move.l	d0,d2

		ifne IN_MUSICX
			lea		_voice_table,a2
		else
			lea		_voice,a2
		endc
			moveq	#NUM_VOICES-1,d3
.l1			bsr		DoOneVoice
			lea		voice_sizeof(a2),a2
			dbra	d3,.l1

		ifne IN_MUSICX
			lea		_channel_table,a0
		else
			lea		_channel,a0
		endc
			moveq	#NUM_CHANNELS-1,d0
.l30			bclr.b	#CHANB_ALTERED,chan_Flags(a0)
			lea		chan_sizeof(a0),a0
			dbra	d0,.l30

		ifeq IN_MUSICX
			bclr.b	#CHANB_ALTERED,_xchannel+chan_Flags
		endc

		ifne HAS_MODULATION
			lsr.l	#2,d2
			move.l	d2,_globaldata+glob_SineValue
		endc

			movem.l	(sp)+,d2/d3/d4/a2/a3/a5/a6
			rts

		ifne IN_MUSICX
			xdef	_EnvOneVoice
			xdef	EnvOneVoice
_EnvOneVoice
			move.l	4(sp),a0						; voice
			move.l	8(sp),d0						; delta
EnvOneVoice
			movem.l	d2/d3/d4/a2/a3/a5/a6,-(sp)

			move.l	d0,d2
			move.l	a0,a2

			bsr		DoOneVoice

			movem.l	(sp)+,d2/d3/d4/a2/a3/a5/a6
			rts
		endc

*==========================================================================*
*
*	KillVoice - immediately terminates sound on one voice
*
*	void KillVoice(struct VoiceData *v)
*	void stop_audio(UWORD num)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_stop_audio
_stop_audio
			move.w	4(sp),d0							; voice number
		endc
stop_audio
			movem.l	d2/a6,-(sp)
			moveq	#0,d2
			move.w	d0,d2

			move.l	_audio_stop,a1						; stop that voice
			moveq	#1,d1
			lsl.l	d2,d1
			move.l	d1,IO_UNIT(a1)
			move.b	#IOF_QUICK,IO_FLAGS(a1)

			move.l	_AudioDevice,a6						; use BeginIO
			JSRDEV	BEGINIO

			cmp.w	#37,LIB_VERSION(a6)					; if KS v37, skip
			bpl.s	.l1

			; the AudControl structure is 16 bytes in size, so...

			move.l	#$00dff000+aud,a0					; use audio flush trick
			lsl.w	#4,d2								; 16 * channel #
			move.l	#$00010000,ac_per(a0,d2.w)			; hit hardware

.l1			movem.l	(sp)+,d2/a6
			rts

		ifne C_ENTRY
			DEFX	_KillVoice
_KillVoice
			move.l	4(sp),a0
		endc
KillVoice
			move.l	voice_Channel(a0),a1				; get channel

			subq.b	#1,chan_VoicesActive(a1)			; -1 active voice
		ifeq IN_MUSICX
			lea		_globaldata,a1
			subq.b	#1,glob_VoicesActive(a1)
		endc

			clr.l	voice_Channel(a0)					; clear voice
			clr.b	voice_Status(a0)					; ENV_FREE
			clr.b	voice_Flags(a0)
			clr.b	voice_Priority(a0)
			clr.l	voice_UniqueID(a0)

			moveq	#0,d0							; let stop_audio finish up
			move.b	voice_Number(a0),d0
			jmp		stop_audio

*==========================================================================*
*
*	ResetChannel - handles RESET ALL CONTROLLERS midi command
*
*	void ResetChannel(struct ChannelData *chan)(a0)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_ResetChannel
_ResetChannel
			move.l	4(sp),a0
		endc
ResetChannel
		ifne HAS_MODULATION
			clr.w	chan_Modulation(a0)
			move.w	#1000,chan_ModulationTime(a0)			; 1 second
		endc
		ifne HAS_MICROTONAL
			move.w	#-1,chan_Microtonal(a0)
		endc
			move.w	#500,chan_PortamentoTime(a0)			; 1/2 second
			move.w	#NO_BEND,chan_PitchBend(a0)
			clr.w	chan_RealBend(a0)
			move.b	#MAX_BEND_RANGE,chan_PitchBendRange(a0)
			move.b	#128,chan_Volume(a0)
			btst.b	#0,chan_Number(a0)
			bne.s	.l1
			bclr.b	#CHANB_PAN,chan_Flags(a0)
			bra.s	.l2
.l1			bset.b	#CHANB_PAN,chan_Flags(a0)
.l2			bclr.b	#CHANB_PORTAMENTO,chan_Flags(a0)
			bclr.b	#CHANB_MICROTONAL,chan_Flags(a0)
			bset.b	#CHANB_ALTERED,chan_Flags(a0)
			rts

*==========================================================================*
*
*	ControlCh - handles CONTROL CHANGE midi command
*
*	void ControlCh(struct ChannelData *chan,UBYTE *midi)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_ControlCh
_ControlCh
			move.l	4(sp),a0
			move.l	8(sp),a1
		endc
ControlCh
			move.b	(a1),d0

		ifne HAS_MODULATION
			cmp.b	#1,d0							; modulation level MSB
			bne.s	.l1
			move.b	1(a1),d0
			lsl.w	#8,d0
			move.w	d0,chan_Modulation(a0)
			bra		.l99

.l1			cmp.b	#1+32,d0						; modulation level LSB
			bne.s	.l2
			move.b	1(a1),d0
			add.b	d0,d0
			move.b	d0,chan_Modulation+1(a0)
			bra		.l99

.l2
		endc

			cmp.b	#5,d0							; portamento time MSB
			bne.s	.l3
			moveq	#0,d0
			move.b	1(a1),d0
			lsl.w	#7,d0
			move.w	d0,chan_PortamentoTime(a0)
			bra		.l99

.l3			cmp.b	#5+32,d0						; portamento time LSB
			bne.s	.l4
			move.w	chan_PortamentoTime(a0),d0
			and.w	#$3f80,d0
			or.b	1(a1),d0
			move.w	d0,chan_PortamentoTime(a0)
			bra		.l99

.l4			cmp.b	#6,d0							; data entry MSB
			bne.s	.l5
			tst.w	chan_RPN(a0)					; only doing RPN #0
			bne		.l99

			moveq	#0,d0
			move.b	1(a1),d0
			cmp.b	#MAX_BEND_RANGE,d0				; can't be more than max
			ble.s	.l6
			move.b	#MAX_BEND_RANGE,d0
.l6			move.b	d0,chan_PitchBendRange(a0)
			move.w	chan_PitchBend(a0),d1
			sub.w	#NO_BEND,d1						; if no bend, no need to adj
			beq		.l99

			mulu.w	d0,d1			; RealBend = (PitchBend - NO_BEND) * Range
			lsl.l	#8,d1			;				* 256
			divu.w	#NO_BEND,d1		;				/ NO_BEND
			move.w	d1,chan_RealBend(a0)
			bset.b	#CHANB_ALTERED,chan_Flags(a0)	; mark as altered
			bra		.l99

.l5			cmp.b	#7,d0							; Main Volume MSB
			bne.s	.l7
			move.b	1(a1),d0
			beq.s	.l8
			addq.w	#1,d0
.l8			move.b	d0,chan_Volume(a0)
		ifne HAS_FULLCHANVOL
			bset.b	#CHANB_ALTERED,chan_Flags(a0)	; mark as altered
		endc
			bra		.l99

.l7			cmp.b	#10,d0							; Pan
			bne.s	.l9
			move.b	1(a1),d0
			cmp.b	#64,d0							; compare data to 64
			bmi.s	.l10								; less than 64, PAN left
			bne.s	.l11								; more than 64, PAN right
			btst.b	#0,chan_Number(a0)				; else, base on channel #
			beq.s	.l10								; is even, PAN left
.l11			bclr.b	#CHANB_PAN,chan_Flags(a0)
			bra		.l99
.l10			bset.b	#CHANB_PAN,chan_Flags(a0)
			bra		.l99

.l9
		ifne HAS_MODULATION
			cmp.b	#16,d0							; GPC as Modulation Time MSB
			bne.s	.l12
			moveq	#0,d0
			move.b	1(a1),d0
			lsl.w	#7,d0
			move.w	d0,chan_ModulationTime(a0)
			bra		.l99

.l12			cmp.b	#16+32,d0						; GPC as Modulation Time LSB
			bne.s	.l13
			move.w	chan_ModulationTime(a0),d0
			and.w	#$3f80,d0
			or.b	1(a1),d0
			move.w	d0,chan_ModulationTime(a0)
			bra		.l99

.l13
		endc
		ifne HAS_MICROTONAL
			cmp.b	#17,d0							; GPC as Microtonal Set MSB
			bne		.l14
			moveq	#0,d0
			move.b	1(a1),d0
			lsl.w	#8,d0
			move.w	d0,chan_Microtonal(a0)
			bra		.l99

.l14			cmp.b	#17+32,d0						; GPC as Microtonal Set LSB
			bne		.l15
			move.b	1(a1),d0
			add.b	d0,d0
			move.b	d0,chan_Modulation+1(a0)
			bra		.l99

.l15
		endc

			cmp.b	#64,d0							; Damper Pedal
			bne.s	.l16
			btst.b	#6,1(a1)						; test bit 6 (64)
			beq.s	.l17								; do releasing pedal
			bset.b	#CHANB_DAMPER,chan_Flags(a0)
			bra		.l99

.l17			bclr.b	#CHANB_DAMPER,chan_Flags(a0)	; release dampered voices
		ifne IN_MUSICX
			lea		_voice_table,a1
		else
			lea		_voice,a1
		endc
			moveq	#NUM_VOICES-1,d0
.l18			cmp.l	voice_Channel(a1),a0
			bne.s	.l19
			bclr.b	#VOICEB_DAMPER,voice_Flags(a1)
			beq.s	.l19
			move.b	#ENV_RELEASE,voice_Status(a1)
.l19			lea		voice_sizeof(a1),a1
			dbra	d0,.l18
			bra		.l99

.l16			cmp.b	#65,d0							; Portamento off/on 
			bne.s	.l20
			btst.b	#6,1(a1)						; test bit 6 (64)
			beq.s	.l21								; do off
			bset.b	#CHANB_PORTAMENTO,chan_Flags(a0)	; set to on
			bra		.l99
.l21			bclr.b	#CHANB_PORTAMENTO,chan_Flags(a0)	; set to off
			move.b	#-1,chan_LastNote(a2)			; no last note
			bra		.l99

.l20
		ifne HAS_MICROTONAL
			cmp.b	#80,d0							; Microtonal off/on 
			bne		.l22
			btst.b	#6,1(a1)						; test bit 6 (64)
			beq		.l23								; do off
			bset.b	#CHANB_MICROTONAL,chan_Flags(a0)	; set to on
			bra		.l99
.l23			bclr.b	#CHANB_MICROTONAL,chan_Flags(a0)	; set to off
			bra		.l99

.l22
		endc

			cmp.b	#81,d0							; Audio Filter off/on 
			bne.s	.l30
			moveq	#0,d0							; default <64, filter off
			cmp.b	#64,1(a1)
			beq.s	.l31
			bmi.s	.l32

			moveq	#1,d0							; >64, set filter on
			bra.s	.l32

.l31			move.w	_globaldata+glob_Filter,d0		; =64, set as global flag
.l32			bsr		SetAudioFilter
			bra.s	.l99

.l30			cmp.b	#100,d0							; RPN LSB
			bne.s	.l24
			move.b	1(a1),chan_RPN+1(a0)
			bra.s	.l99

.l24			cmp.b	#101,d0							; RPN MSb
			bne.s	.l25
			move.b	1(a1),chan_RPN(a0)
			bra.s	.l99

.l25			cmp.b	#121,d0							; Reset All Controllers
			bne.s	.l26
			bsr		ResetChannel					; a0 has channel already
			bra.s	.l99

.l26			cmp.b	#123,d0							; All Notes Off
			bne.s	.l27
			bsr		AllNotesOff						; a0 has channel already
			bra.s	.l99

.l27			cmp.b	#126,d0							; MONO mode
			bne.s	.l28
			bset.b	#CHANB_MONO,chan_Flags(a0)
			bsr		AllNotesOff						; a0 has channel already
			bra.s	.l99

.l28			cmp.b	#127,d0							; POLY mode
			bne.s	.l29
			bclr.b	#CHANB_MONO,chan_Flags(a0)
			bsr		AllNotesOff						; a0 has channel already
			bra.s	.l99

.l29			cmp.b	#120,d0							; All Sounds Off
			bne.s	.l99
			bsr		AllSoundsOff					; a0 has channel already
;			bra.s	.l99

.l99			rts

*==========================================================================*
*
*	PitchBend - handles PITCH BEND midi command
*
*	void PitchBend(struct ChannelData *chan,UBYTE *midi)(a0/a1)
*
*==========================================================================*

		ifne C_ENTRY
			DEFX	_PitchBend
_PitchBend
			move.l	4(sp),a0						; channel
			move.l	8(sp),a1						; midi stream
		endc
PitchBend
			moveq	#0,d0
			move.b	1(a1),d0						; (midi[1] << 7) + midi[0]
			lsl.w	#7,d0
			add.b	(a1),d0
			move.w	d0,chan_PitchBend(a0)			; save as pitch bend value

					; (PitchBend - NO_BEND) * PitchBendRange * 256 / NO_BEND;
			sub.w	#NO_BEND,d0
			moveq	#0,d1
			move.b	chan_PitchBendRange(a0),d1
			lsl.w	#8,d1
			muls.w	d1,d0
			divs.w	#NO_BEND,d0
			move.w	d0,chan_RealBend(a0)			; save as real bend value

			bset.b	#CHANB_ALTERED,chan_Flags(a0)	; set altered flag
			rts

*==========================================================================*
*  Math Routines
*==========================================================================*

divs:		;long divide	(primary = primary/secondary)
			move.l	d4,-(sp)
			clr.l	d4			;mark result as positive
			tst.l	d0
			bpl.s	prim_ok
			neg.l	d0
			add.w	#1,d4			;mark as negative
prim_ok:
			tst.l	d1
			bpl.s	sec_ok
			neg.l	d1
			eor.w	#1,d4			;flip sign of result
sec_ok:
			jsr	comdivide
chksign:
			tst.w	d4
			beq.s	posres
			neg.l	d0
posres:
			move.l	(sp)+,d4
;			tst.l	d0
			rts

modu:		;unsigned long remainder	(primary = primary%secondary)
;			move.l	d1,-(sp)
			jsr	comdivide
			move.l	d1,d0
;			move.l	(sp)+,d1
;			tst.l	d0
			rts

divu:		;unsigned long divide		(primary = primary/secondary)
;			move.l	d1,-(sp)
			jsr	comdivide
;			move.l	(sp)+,d1
;			tst.l	d0
			rts

; divide (dx,ax) by (bx,cx):
;	quotient in (dx,ax)
;	remainder in (bx,cx)
comdivide:
			movem.l	d2/d3,-(sp)
			swap	d1
			tst.w	d1
			bne.s	hardldv
			swap	d1
			move.w	d1,d3		;get divisor
			move.w	d0,d2		;save second part of dividend
			clr.w	d0		;get first part of dividend
			swap	d0
			divu.w	d3,d0		;do first divide
			move.l	d0,d1		;copy first remainder
			swap	d0		;get first quotient
			move.w	d2,d1		;get back second part of dividend
			divu.w	d3,d1		;do second divide
			move.w	d1,d0		;get second quotient
			clr.w	d1		;remainder is small
			swap	d1		;get it
			movem.l	(sp)+,d2/d3
			rts			;and return

hardldv:
			swap	d1
			move.l	d1,d3		;save divisor
			move.l	d0,d1		;copy dividend
			clr.w	d1		;set up remainder
			swap	d1
			swap	d0		;set up quotient
			clr.w	d0
			move.l	#16-1,d2		;do 16 times
.l1
			add.l	d0,d0		;shift the quotient
			addx.l	d1,d1		;shift the remainder
			cmp.l	d1,d3		;check if big enough to subtract
			bhi.s	.l2
			sub.l	d3,d1		;yes, so subtract it
			add.w	#1,d0		;and add one to quotient
.l2
			dbf	d2,.l1		;and loop till done
			movem.l	(sp)+,d2/d3
			rts

mulu:		;unsigned long multiply	(primary = primary*secondary)
			movem.l	d2/d3,-(sp)
			move.w	d1,d2
			mulu.w	d0,d2		;d0.l * d1.l
			move.l	d1,d3
			swap	d3
			mulu.w	d0,d3		;d0.l * d1.h
			swap	d3
			clr.w	d3
			add.l	d3,d2
			swap	d0
			mulu.w	d1,d0		;d0.h * d1.l
			swap	d0
			clr.w	d0
			add.l	d2,d0
			movem.l	(sp)+,d2/d3
			rts

*==========================================================================*
*  IntAlg
*      Inputs:  d0 = logarithm in standard format
*      Outputs: d0 = 32-bit integer value
*      Scratch: d1,a0
*      Max Error: 1 part in 30,000
*==========================================================================*

		ifne C_ENTRY
			DEFX	_IntAlg
_IntAlg:
			move.l	4(sp),d0
		endc
IntAlg:
			move.l	d2,-(sp)
			move.w	d0,d1					; get mantissa

			lsr.w	#8,d1					; get the "lookup" part.
			add.w	d1,d1					; times 2 for table

			lea		alogtable,a0			; address of antilog table
			add.w	d1,a0					; plus offset

			move.w	(a0)+,d1				; get the digits to use.
			move.w	(a0),d2					; get the next value too
			sub.w	d1,d2					; distance between table entries

; now we need to interpolate.
			lsl.w	#8,d0					; adjust interpolation bits
			add.w	#128,d0					; add rounding factor??
			mulu.w	d0,d2					; times interpolation factor
			swap	d2						; get high word
			add.w	d2,d1					; and adjust result.

; now we have 16 bits in the lower half of d1, and the upper half of d1 is how
; many to shift them over...

			swap	d1						; put it in the top
			bset	#0,d1					; make the lowest bit a 1
			ror.l	#1,d1					; make the leading bit a 1, shift others over

			swap	d0						; get bits to shift.
			cmp.w	#32,d0					; if d0 > 33
			bhs.s	.l9						; then fail, we can't alog this.
			eor.b	#31,d0					; reverse direction of shift

			lsr.l	d0,d1					; rotate number
			moveq	#0,d0					; clear d0
			addx.l	d0,d1					; round using carry bit
			move.l	d1,d0					; put in d0
			move.l	(sp)+,d2
			rts								; return

.l9			move.l	(sp)+,d2
			moveq	#0,d0					; return 0 (value that can never be logged)
			rts								; return

alogtable:
			dc.w	00000,00178,00356,00535,00714,00893,01073,01254
			dc.w	01435,01617,01799,01981,02164,02348,02532,02716
			dc.w	02902,03087,03273,03460,03647,03834,04022,04211
			dc.w	04400,04590,04780,04971,05162,05353,05546,05738
			dc.w	05932,06125,06320,06514,06710,06906,07102,07299
			dc.w	07496,07694,07893,08092,08292,08492,08693,08894
			dc.w	09096,09298,09501,09704,09908,10113,10318,10524
			dc.w	10730,10937,11144,11352,11560,11769,11979,12189
			dc.w	12400,12611,12823,13036,13249,13462,13676,13891
			dc.w	14106,14322,14539,14756,14974,15192,15411,15630
			dc.w	15850,16071,16292,16514,16737,16960,17183,17408
			dc.w	17633,17858,18084,18311,18538,18766,18995,19224
			dc.w	19454,19684,19915,20147,20379,20612,20846,21080
			dc.w	21315,21550,21786,22023,22260,22498,22737,22977
			dc.w	23216,23457,23698,23940,24183,24426,24670,24915
			dc.w	25160,25406,25652,25900,26148,26396,26645,26895
			dc.w	27146,27397,27649,27902,28155,28409,28664,28919
			dc.w	29175,29432,29690,29948,30207,30466,30727,30988
			dc.w	31249,31512,31775,32039,32303,32568,32834,33101
			dc.w	33369,33637,33906,34175,34446,34717,34988,35261
			dc.w	35534,35808,36083,36359,36635,36912,37190,37468
			dc.w	37747,38028,38308,38590,38872,39155,39439,39724
			dc.w	40009,40295,40582,40870,41158,41448,41738,42029
			dc.w	42320,42613,42906,43200,43495,43790,44087,44384
			dc.w	44682,44981,45280,45581,45882,46184,46487,46791
			dc.w	47095,47401,47707,48014,48322,48631,48940,49251
			dc.w	49562,49874,50187,50500,50815,51131,51447,51764
			dc.w	52082,52401,52721,53041,53363,53685,54008,54333
			dc.w	54658,54983,55310,55638,55966,56296,56626,56957
			dc.w	57289,57622,57956,58291,58627,58964,59301,59640
			dc.w	59979,60319,60661,61003,61346,61690,62035,62381
			dc.w	62727,63075,63424,63774,64124,64476,64828,65182
			dc.w	00000

*==========================================================================*
*	Sine table for modulation effects
*==========================================================================*

		ifne HAS_MODULATION
			DEFX	_sine_table
_sine_table
			dc.b	0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45
			dc.b	48,51,54,57,59,62,65,67,70,73,75,78,80,82,85,87
			dc.b	89,91,94,96,98,100,102,103,105,107,108,110,112,113,114,116
			dc.b	117,118,119,120,121,122,123,123,124,125,125,126,126,126,126,127
			dc.b	127,127,126,126,126,126,125,125,124,123,123,122,121,120,119,118
			dc.b	117,116,114,113,112,110,108,107,105,103,102,100,98,96,94,91
			dc.b	89,87,85,82,80,78,75,73,70,67,65,62,59,57,54,51
			dc.b	48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3
			dc.b	0,-3,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-36,-39,-42,-45
			dc.b	-48,-51,-54,-57,-59,-62,-65,-67,-70,-73,-75,-78,-80,-82,-85,-87
			dc.b	-89,-91,-94,-96,-98,-100,-102,-103,-105,-107,-108,-110,-112,-113,-114,-116
			dc.b	-117,-118,-119,-120,-121,-122,-123,-123,-124,-125,-125,-126,-126,-126,-126,-127
			dc.b	-127,-127,-126,-126,-126,-126,-125,-125,-124,-123,-123,-122,-121,-120,-119,-118
			dc.b	-117,-116,-114,-113,-112,-110,-108,-107,-105,-103,-102,-100,-98,-96,-94,-91
			dc.b	-89,-87,-85,-82,-80,-78,-75,-73,-70,-67,-65,-62,-59,-57,-54,-51
   			dc.b	-48,-45,-42,-39,-36,-33,-30,-27,-24,-21,-18,-15,-12,-9,-6,-3
		endc
