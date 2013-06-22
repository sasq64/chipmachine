
	incdir	"Amiga:Includes/"
	include "misc/DeliPlayer.i"

;
;
	SECTION Player,Code
;
;

	PLAYERHEADER PlayerTagArray

	dc.b '$VER: Master Soundtracker player 2005-11-30',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,PName
	dc.l	DTP_Creator,CName
	dc.l	DTP_Check2,Chk
	dc.l	DTP_Interrupt,replay_muzak
	dc.l	DTP_InitPlayer,InitPlay
	dc.l	DTP_EndPlayer,EndPlay
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_EndSound,RemSnd
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	DTP_FormatName,Formatpointer
	dc.l	TAG_DONE
	
*-----------------------------------------------------------------------*
;
; Player/Creatorname und lokale Daten
Formatpointer	dc.l	FName
delibase	dc.l	0
song		dc.l	0

PName	dc.b 'MasterSoundtracker',0
FName	dc.b 'MasterSoundtracker or compatible',0
CName	dc.b "'88 by DOC & TIP/The New Masters",10
	dc.b 'adapted for UADE by mld',10
	dc.b '-----------------------',10
	dc.b 'note: filedetection checks for uade!',0
	
	even


*-----------------------------------------------------------------------*
;
; Test Soundtracker-Module

Chk						; UST ?
	move.l	dtg_ChkData(a5),a0
	move.l	dtg_ChkSize(a5),d1
	move.l	a0,song
	bsr.w	mcheck_moduledata		; currently -1 for no mod
	cmp.l	#"M15.",d0
	bne	.Chk_fail

	move.b	fx,Fname
	add.b	#65,Fname

.Chk_ok: moveq	#0,d0
	rts

.Chk_fail:	
	moveq	#-1,d0
	rts


*-----------------------------------------------------------------------*
;
; Init Player

InitPlay
	move.l	a5,delibase
	moveq	#0,d0

	move.l	dtg_AudioAlloc(a5),a0		; Function
	jsr	(a0)				; returncode is already set !
	sf ep_songendflag
	rts

*-----------------------------------------------------------------------*
;
; End Player

EndPlay
	move.l	dtg_AudioFree(a5),a0		; Function
	jsr	(a0)
	rts



*-----------------------------------------------------------------------*
;
; Remove Sound

RemSnd
	lea	$dff000,a0
	moveq	#0,d0
	move.w	d0,$a8(a0)
	move.w	d0,$b8(a0)
	move.w	d0,$c8(a0)
	move.w	d0,$d8(a0)
	move.w	#$000F,$96(a0)			; End Sound
	rts


*-----------------------------------------------------------------------*

******************************************
* Master Soundtracker V1.0 replayroutine *
* based on V9.0 of DOC *******************
******************************************

* Improved by TIP of The New Masters in JULY 1988 *


InitSnd:
	move.l	song,muzakoffset	;** get offset

init0:	move.l	muzakoffset,a0		;** get highest used pattern
	add.l	#472,a0
	move.l	#$80,d0
	clr.l	d1
init1:	move.l	d1,d2
	subq.w	#1,d0
init2:	move.b	(a0)+,d1
	cmp.b	d2,d1
	bgt.s	init1
	dbf	d0,init2
	addq.b	#1,d2

init3:	move.l	muzakoffset,a0		;** calc samplepointers
	lea	pointers(pc),a1
	lsl.l	#8,d2
	lsl.l	#2,d2
	add.l	#600,d2
	add.l	a0,d2
	moveq	#14,d0
init4:	move.l	d2,(a1)+
	move.l	d2,a2
	clr.l	(a2)
	clr.l	d1
	move.w	42(a0),d1
	lsl.l	#1,d1
	add.l	d1,d2
	add.l	#30,a0
	dbf	d0,init4

init5:	clr.w	$dff0a8			;** clear used values
	clr.w	$dff0b8
	clr.w	$dff0c8
	clr.w	$dff0d8
	clr.w	timpos
	clr.l	trkpos
	clr.l	patpos

	rts


replay_muzak:
	move.l	muzakoffset,a0		;** initialize timer irq
	move.b	470(a0),numpat+1	;number of patterns

	tst.b	ep_songendflag
	beq	replay
	bsr	ep_Songend
replay
	movem.l	d0-d7/a0-a6,-(a7)
	addq.w	#1,timpos
speed:	cmp.w	#6,timpos
	beq.L	replaystep

chaneleffects:				;** seek effects
	lea	datach0(pc),a6
	tst.b	3(a6)
	beq.s	ceff1
	lea	$dff0a0,a5
	bsr.s	ceff5
ceff1:	lea	datach1(pc),a6
	tst.b	3(a6)
	beq.s	ceff2
	lea	$dff0b0,a5
	bsr.s	ceff5
ceff2:	lea	datach2(pc),a6
	tst.b	3(a6)
	beq.s	ceff3
	lea	$dff0c0,a5
	bsr.s	ceff5
ceff3:	lea	datach3(pc),a6
	tst.b	3(a6)
	beq.s	ceff4
	lea	$dff0d0,a5
	bsr.s	ceff5
ceff4:	movem.l	(a7)+,d0-d7/a0-a6
	rts

ceff5:	move.b	2(a6),d0		;room for some more
	and.b	#$f,d0			;implementations below
	tst.b	d0
	beq.s	arpreggiato
	cmp.b	#1,d0
	beq.L	pitchup
	cmp.b	#2,d0
	beq.L	pitchdown
	cmp.b	#12,d0
	beq.L	setvol
	cmp.b	#14,d0
	beq.L	setfilt
	cmp.b	#15,d0
	beq.L	setspeed
	rts

arpreggiato:				;** spread by time
	cmp.w	#1,timpos
	beq.s	arp1
	cmp.w	#2,timpos
	beq.s	arp2
	cmp.w	#3,timpos
	beq.s	arp3
	cmp.w	#4,timpos
	beq.s	arp1
	cmp.w	#5,timpos
	beq.s	arp2
	rts

arp1:	clr.l	d0			;** get higher note-values
	move.b	3(a6),d0		;   or play original
	lsr.b	#4,d0
	bra.s	arp4
arp2:	clr.l	d0
	move.b	3(a6),d0
	and.b	#$f,d0
	bra.s	arp4
arp3:	move.w	16(a6),d2
	bra.s	arp6
arp4:	lsl.w	#1,d0
	clr.l	d1
	move.w	16(a6),d1
	lea	notetable,a0
arp5:	move.w	(a0,d0.w),d2
	cmp.w	(a0),d1
	beq.s	arp6
	addq.l	#2,a0
	bra.s	arp5
arp6:	move.w	d2,6(a5)
	rts

pitchdown:
	bsr.s	newrou
	clr.l	d0
	move.b	3(a6),d0
	and.b	#$f,d0
	add.w	d0,(a4)
	cmp.w	#$358,(a4)
	bmi.s	ok1
	move.w	#$358,(a4)
ok1:	move.w	(a4),6(a5)
	rts

pitchup:bsr.s	newrou
	clr.l	d0
	move.b	3(a6),d0
	and.b	#$f,d0
	sub.w	d0,(a4)
	cmp.w	#$71,(a4)
	bpl.s	ok2
	move.w	#$71,(a4)
ok2:	move.w	(a4),6(a5)
	rts

setvol:	move.b	3(a6),8(a5)
	rts

setfilt:move.b	3(a6),d0
	and.b	#1,d0
	lsl.b	#1,d0
	and.b	#$fd,$bfe001
	or.b	d0,$bfe001
	rts

setspeed:
	clr.l	d0
	move.b	3(a6),d0
	and.b	#$f,d0
	move.w	d0,speed+2
	rts

newrou:	cmp.l	#datach0,a6
	bne.s	next1
	lea	voi1(pc),a4
	rts
next1:	cmp.l	#datach1,a6
	bne.s	next2
	lea	voi2(pc),a4
	rts
next2:	cmp.l	#datach2,a6
	bne.s	next3
	lea	voi3(pc),a4
	rts
next3:	lea	voi4(pc),a4
	rts

replaystep:				;** work next pattern-step
	clr.w	timpos
	move.l	muzakoffset,a0
	move.l	a0,a3
	add.l	#12,a3			;ptr to soundprefs
	move.l	a0,a2
	add.l	#472,a2			;ptr to pattern-table
	add.l	#600,a0			;ptr to first pattern
	clr.l	d1
	move.l	trkpos,d0		;get ptr to current pattern
	move.b	(a2,d0),d1
	lsl.l	#8,d1
	lsl.l	#2,d1
	add.l	patpos,d1		;get ptr to current step
	clr.w	enbits
	lea	$dff0a0,a5		;chanel 0
	lea	datach0(pc),a6
	bsr.L	chanelhandler
	lea	$dff0b0,a5		;chanel 1
	lea	datach1(pc),a6
	bsr.L	chanelhandler
	lea	$dff0c0,a5		;chanel 2
	lea	datach2(pc),a6
	bsr.L	chanelhandler
	lea	$dff0d0,a5		;chanel 3
	lea	datach3(pc),a6
	bsr.L	chanelhandler
	move.w	#400,d0			;** wait a while and set len
rep1:
	movem.l	d0-d7/a0-a6,-(a7)
	move.l	delibase,a5
	move.l	dtg_WaitAudioDMA(a5),a0
	jsr (a0)
	movem.l	(a7)+,d0-d7/a0-a6
	
	move.w	#$8000,d0
	or.w	enbits,d0
	move.w	d0,$dff096
	cmp.w	#1,datach0+14
	bne.s	rep2
	clr.w	datach0+14
	move.w	#1,$dff0a4
rep2:	cmp.w	#1,datach1+14
	bne.s	rep3
	clr.w	datach1+14
	move.w	#1,$dff0b4
rep3:	cmp.w	#1,datach2+14
	bne.s	rep4
	clr.w	datach2+14
	move.w	#1,$dff0c4
rep4:	cmp.w	#1,datach3+14
	bne.s	rep5
	clr.w	datach3+14
	move.w	#1,$dff0d4

rep5:	add.l	#16,patpos		;next step
	cmp.l	#64*16,patpos		;pattern finished ?
	bne.s	rep6
	clr.l	patpos
	addq.l	#1,trkpos		;next pattern in table
	clr.l	d0
	move.w	numpat,d0
	cmp.l	trkpos,d0		;song finished ?
	bne.s	rep6
	clr.l	trkpos
	move.b	#1,ep_songendflag
rep6:	movem.l	(a7)+,d0-d7/a0-a6
	rts

chanelhandler:
	move.l	(a0,d1.l),(a6)		;get period & action-word
	addq.l	#4,d1			;point to next chanel
	clr.l	d2
	move.b	2(a6),d2		;get nibble for soundnumber
	lsr.b	#4,d2
	beq.s	chan2			;no soundchange !
	move.l	d2,d4			;** calc ptr to sample
	lsl.l	#2,d2
	mulu	#30,d4
	lea	pointers-4(pc),a1
	move.l	(a1,d2.l),4(a6)		;store sample-address
	move.w	(a3,d4.l),8(a6)		;store sample-len in words
	move.w	2(a3,d4.l),18(a6)	;store sample-volume

	move.l	d0,-(a7)
	move.b	2(a6),d0
	and.b	#$f,d0
	cmp.b	#$c,d0
	bne.s	ok3
	move.b	3(a6),8(a5)
	bra.s	ok4
ok3:	move.w	2(a3,d4.l),8(a5)	;change chanel-volume
ok4:	move.l	(a7)+,d0

	clr.l	d3
	move.w	4(a3,d4),d3		;** calc repeatstart
	add.l	4(a6),d3
	move.l	d3,10(a6)		;store repeatstart
	move.w	6(a3,d4),14(a6)		;store repeatlength
	cmp.w	#1,14(a6)
	beq.s	chan2			;no sustainsound !
	move.l	10(a6),4(a6)		;repstart  = sndstart
	move.w	6(a3,d4),8(a6)		;replength = sndlength
chan2:	tst.w	(a6)
	beq.s	chan4			;no new note set !
	move.w	22(a6),$dff096		;clear dma
	tst.w	14(a6)
	bne.s	chan3			;no oneshot-sample
	move.w	#1,14(a6)		;allow resume (later)
chan3:	bsr.L	newrou
	move.w	(a6),(a4)
	move.w	(a6),16(a6)		;save note for effect
	move.l	4(a6),0(a5)		;set samplestart
	move.w	8(a6),4(a5)		;set samplelength
	move.w	(a6),6(a5)		;set period
	move.w	22(a6),d0
	or.w	d0,enbits		;store dma-bit
	move.w	18(a6),20(a6)		;volume trigger
chan4:	rts

	incdir "amiga:work/players/tracker/common/"
	include	"mod_check.s"
	include	"ep_misc.s"


datach0:	blk.w	11,0
		dc.w	1
datach1:	blk.w	11,0
		dc.w	2
datach2:	blk.w	11,0
		dc.w	4
datach3:	blk.w	11,0
		dc.w	8
voi1:		dc.w	0
voi2:		dc.w	0
voi3:		dc.w	0
voi4:		dc.w	0
pointers:	blk.l	15,0
notetable:	dc.w	856,808,762,720,678,640,604,570
		dc.w	538,508,480,453,428,404,381,360
		dc.w	339,320,302,285,269,254,240,226  
		dc.w	214,202,190,180,170,160,151,143
		dc.w	135,127,120,113,000
muzakoffset:	dc.l	0
trkpos:		dc.l	0
patpos:		dc.l	0
numpat:		dc.w	0
enbits:		dc.w	0
timpos:		dc.w	0

