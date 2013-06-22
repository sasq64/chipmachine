; thanks to Brian Postma for placing his soundmon replay into the public
; domain!


;---- deli stuff ----

	incdir	"Amiga:Includes/"
	include "misc/DeliPlayer.i"

;
;
	SECTION Player,Code
;
; 

	PLAYERHEADER PlayerTagArray

	dc.b '$VER: SoundMon 2.0 player module (Jul 2004)',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,PName
	dc.l	DTP_Creator,CName
	dc.l	DTP_ModuleName,MNamePTR
	dc.l	DTP_Check2,Chk
	dc.l	DTP_Interrupt,Int
	dc.l	DTP_Config,Config
	dc.l	DTP_InitPlayer,InitPlay
	dc.l	DTP_EndPlayer,EndPlay
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_EndSound,RemSnd
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	TAG_DONE
	

; Player/Creatorname und lokale Daten
MnamePTR	dc.l	MName

PName	dc.b 'SoundMon 2.0',0
CName	dc.b "Brian Postma's SoundMon 2.0",10
	dc.b 'adapted for UADE by mld',0
	
MName
	ds.b	25
	dc.b	0
	even
delibase	dc.l	0


; call playroutine from interrupt
Int
	movem.l	d0-d7/a0-a6,-(sp)
	jsr	bpmusic 			; playroutine
	movem.l	(sp)+,d0-d7/a0-a6
	rts


; Test for SoundMon-Modul
Chk						; SoundMon V2 ?
	move.l	dtg_ChkData(a5),a0
	move.l	$1a(a0),d0
	lsr.l	#8,d0
	sub.l	#"V.2",d0
	rts


; Player init
Config
	lea	null,a0
	lea	bp_buffer,a1
	move.w	#bpsong-null-1,d0
ConfigCopy
	move.b	(a0)+,(a1)+
	dbra	d0,ConfigCopy
	moveq	#0,d0				; no Error
	rts


; Init Player
InitPlay
	move.l	a5,delibase
	moveq	#0,d0
	move.l	dtg_GetListData(a5),a0		; Function
	jsr	(a0)
	move.l	a0,bpsong

	lea	MName,a1
	move.w	#25,d0
TitleCopy:
	move.b	(a0)+,(a1)+
	dbra	d0,TitleCopy

	move.l	dtg_AudioAlloc(a5),a0
	jsr	(a0)
	rts


; End Player
EndPlay
	move.l	dtg_AudioFree(a5),a0		; Function
	jsr	(a0)
	move.b	#0, MName			; Clear title
	rts


; Init Sound
InitSnd

	lea	bp_buffer,a0
	lea	null,a1
	move.w	#bpsong-null-1,d0
InitSndCopy
	move.b	(a0)+,(a1)+
	dbra	d0,InitSndCopy
	jsr	bpinit				; Init Sound

	rts


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

;---- replay ----

*-----------------------------------------------------------------------*

 ************************************************
 *                                              *
 * Soundmon music player (C) 1990 Brian Postma. *
 *   This Version was made PC Relative by DEL   *
 *                                              *
 ************************************************
	
 **************
 * Initialise *
 **************

bpinit	lea variables(pc),a6		;Variables pointer in a6!
	lea samples(pc),a0
	move.l bpsong(pc),a1
	clr.b numtables(a6)
	cmp.w #'V.',26(a1)
	bne.s bpnotv2
	cmp.b #'2',28(a1)
	bne.s bpnotv2
	move.b 29(a1),numtables(a6)
bpnotv2	move.l #512,d0
	move.w 30(a1),d1			;D1 now contains length in steps
	add.w d1,d1
	add.w d1,d1
	subq.w #1,d1 			;Correction for DBRA
	moveq #1,d2 			;Highest pattern number is 1!
findhighest
	cmp.w (a1,d0.w),d2			;Is it higher
	bge.s nothigher			;No
	move.w (a1,d0.w),d2			;Yes, so let D2 be highest
nothigher	addq.w #4,d0 			;Next Voice
	dbra d1,findhighest			;And search
	moveq #0,d1
	move.w 30(a1),d1
	lsl.w #4,d1 			;16 bytes per step
	move.l #512,d0 			;header is 512 bytes
	mulu #48,d2 			;48 bytes per pattern
	add.l d2,d0
	add.l d1,d0 			;offset for samples
	move.l bpsong(pc),a5
	add.l a5,d0
	move.l d0,tables(a6)
	moveq #0,d1
	move.b numtables(a6),d1		;Number of tables
	lsl.l #6,d1 			;x 64
	add.l d1,d0
	moveq #14,d1 			;15 samples
	lea 32(a1),a1
initloop	move.l d0,(a0)+
	cmp.b #-1,(a1)
	beq.s bpissynth
	move.w 24(a1),d2
	add.w d2,d2			;Length is in words
	add.l d2,d0 			;offset next sample
bpissynth	lea 32(a1),a1 			;Length of Sample Part in header
	dbra d1,initloop

	move.b #1,bpcount(a6)
	move.b #6,bpdelay(a6)
	move.b #1,arpcount(a6)
	move.b #1,bprepcount(a6)
	move.w #$8000,most(a6)
	lea null(pc),a0
	lea bpcurrent(pc),a1
	move.l a0,nullpoint(a6)
	move.l a0,4(a1)
	move.l a0,36(a1)
	move.l a0,68(a1)
	move.l a0,100(a1)			;Init nullpointers!
	rts

 ************
 ** Player **
 ************

bpmusic	lea variables(pc),a6		;Variables pointer in a6!
	bsr bpsynth
	subq.b #1,arpcount(a6)
	moveq #3,d0
	lea bpcurrent(pc),a0
	lea $dff0a0,a1
bploop1	move.b 12(a0),d4
	ext.w d4
	add.w d4,(a0)
	tst.b $1e(a0)
	bne.s bplfo
	move.w (a0),6(a1)
bplfo	move.l 4(a0),(a1)
	move.w 8(a0),4(a1)
	tst.b 11(a0)
	bne.s bpdoarp
	tst.b 13(a0)
	beq.s not2
bpdoarp	tst.b arpcount(a6)
	bne.s not0
	move.b 11(a0),d3
	move.b 13(a0),d4
	and.w #240,d4
	and.w #240,d3
	lsr.w #4,d3
	lsr.w #4,d4
	add.w d3,d4
	add.b 10(a0),d4
	bsr bpplayarp
	bra.s not2
not0	cmp.b #1,arpcount(a6) 
	bne.s not1
	move.b 11(a0),d3
	move.b 13(a0),d4
	and.w #15,d3
	and.w #15,d4
	add.w d3,d4
	add.b 10(a0),d4
	bsr bpplayarp
	bra.s not2
not1	move.b 10(a0),d4
	bsr bpplayarp
not2	lea $10(a1),a1
	lea $20(a0),a0
	dbra d0,bploop1
	tst.b arpcount(a6)
	bne.s arpnotzero
	move.b #3,arpcount(a6)
arpnotzero
	subq.b #1,bpcount(a6)
	beq.s bpskip1
	rts
bpskip1	move.b bpdelay(a6),bpcount(a6)
bpplay	bsr.s bpnext
	move.w dma(a6),$dff096
	move.w #$1f4,d0			;is this a waste ?????
bpxx	dbra d0,bpxx
	moveq #3,d0
	lea $dff0a0,a1
	moveq #1,d1
	lea bpcurrent(pc),a2
	lea bpbuffer(pc),a5
bploop2	btst #15,(a2)
	beq.s bpskip7
	bsr bpplayit
bpskip7	asl.w #1,d1
	lea $10(a1),a1
	lea $20(a2),a2
	lea $24(a5),a5
	dbra d0,bploop2
	rts
bpnext	clr.w dma(a6)
	move.l bpsong(pc),a0
	lea $dff0a0,a3
	moveq #3,d0
	moveq #1,d7
	lea bpcurrent(pc),a1
bploop3	moveq #0,d1
	move.w bpstep(a6),d1
	lsl.w #4,d1
	move.l d0,d2
	lsl.l #2,d2
	add.l d2,d1
	add.l #512,d1
	move.w (a0,d1),d2
	move.b 2(a0,d1),st(a6)
	move.b 3(a0,d1),tr(a6)
	subq.w #1,d2
	mulu #48,d2
	moveq #0,d3
	move.w 30(a0),d3
	lsl.w #4,d3
	add.l d2,d3
	move.l #$00000200,d4
	move.b bppatcount(a6),d4
	add.l d3,d4
	move.l d4,a2
	add.l a0,a2
	moveq #0,d3 
	move.b (a2),d3
	tst.b d3
	bne.s bpskip4
	bra bpoptionals
bpskip4	clr.w 12(a1)	  		;Clear autoslide/autoarpeggio
	move.b 1(a2),d4
	and.b #15,d4
	cmp.b #10,d4    			;Option 10->transposes off
	bne.s bp_do1
	move.b 2(a2),d4
	and.b #240,d4	  		;Higher nibble=transpose
	bne.s bp_not1
bp_do1	add.b tr(a6),d3
	ext.w d3
bp_not1	move.b d3,10(a1) 			; Voor Arpeggio's
	lea bpper(pc),a4
	add.w d3,d3
	move.w -2(a4,d3.w),(a1)
	bset #15,(a1)
	move.b #$ff,2(a1)
	moveq #0,d3
	move.b 1(a2),d3
	lsr.b #4,d3
	and.b #15,d3
	tst.b d3
	bne.s bpskip5
	move.b 3(a1),d3 
bpskip5 	move.b 1(a2),d4
	and.b #15,d4
	cmp.b #10,d4 			;option 10
	bne.s bp_do2
	move.b 2(a2),d4
	and.b #15,d4
	bne.s bp_not2
bp_do2	add.b st(a6),d3
bp_not2	cmp.w #1,8(a1)
	beq.s bpsamplechange
	cmp.b 3(a1),d3
	beq.s bpoptionals
bpsamplechange
	move.b d3,3(a1)
	or.w d7,dma(a6)
bpoptionals 
	moveq #0,d3
	moveq #0,d4
	move.b 1(a2),d3
	and.b #15,d3
	move.b 2(a2),d4
	tst.b d3				; Optionals Here
	bne.s notopt0
	move.b d4,11(a1)
notopt0	cmp.b #1,d3
	bne.s bpskip3
	move.w d4,8(a3)
	move.b d4,2(a1) 			; Volume ook in BPCurrent
bpskip3	cmp.b #2,d3  			; Set Speed
	bne.s bpskip9
	move.b d4,bpcount(a6)
	move.b d4,bpdelay(a6)
bpskip9	cmp.b #3,d3 			; Filter = LED control
	bne.s bpskipa
	tst.b d4
	bne.s bpskipb
	bset #1,$bfe001
	bra.s bpskip2
bpskipb	bclr #1,$bfe001
bpskipa	cmp.b #4,d3 			; PortUp
	bne.s noportup
	sub.w d4,(a1) 			; Slide data in BPCurrent
	clr.b 11(a1) 			; Arpeggio's uit
noportup	cmp.b #5,d3 			; PortDown
	bne.s noportdn
	add.w d4,(a1) 			; Slide down
	clr.b 11(a1)
noportdn	cmp.b #6,d3			; SetRepCount
	bne.s notopt6
	move.b d4,bprepcount(a6)
notopt6	cmp.b #7,d3			; DBRA repcount
	bne.s notopt7
	subq.b #1,bprepcount(a6)
	beq.s notopt7
	move.w d4,bpstep(a6)
notopt7	cmp.b #8,d3			;Set AutoSlide
	bne.s notopt8
	move.b d4,12(a1)
notopt8	cmp.b #9,d3			;Set AutoArpeggio
	bne.s notopt9
	move.b d4,13(a1)
notopt9
bpskip2	lea $10(a3),a3
	lea $20(a1),a1
	asl.w #1,d7
	dbra d0,bploop3   
	addq.b #3,bppatcount(a6)
	cmp.b #48,bppatcount(a6)
	bne.s bpskip8
	clr.b bppatcount(a6)
	addq.w #1,bpstep(a6)
	move.l bpsong(pc),a0
	move.w 30(a0),d1
	cmp.w bpstep(a6),d1
	bne.s bpskip8
	clr.w bpstep(a6)

	movem.l	d0-d6/a0-a6,-(a7)
	move.l	delibase(pc),a5
	move.l	dtg_Songend(a5),a0
	jsr	(a0)
	movem.l	(a7)+,d0-d6/a0-a6

bpskip8	rts
bpplayit	bclr #15,(a2)
	tst.l (a5) 			;Was EG used
	beq.s noeg1 			;No ??
	moveq #0,d3 			;Well then copy
	move.l (a5),a4 			;Old waveform back
	moveq #7,d7 			;to waveform tables
eg1loop	move.l 4(a5,d3.w),(a4)+		;Copy...
	addq.w #4,d3 			;Copy...
	dbra d7,eg1loop			;Copy...
noeg1	move.w (a2),6(a1)			;Period from bpcurrent
	moveq #0,d7
	move.b 3(a2),d7			;Instrument number
	move.l d7,d6 			;Also in d6
	lsl.l #5,d7 			;Header offset	
	move.l bpsong(pc),a3
	cmp.b #-1,(a3,d7.w)			;Is synthetic
	beq.s bpplaysynthetic		;Yes ??
	clr.l (a5) 			;EG Off
	clr.b $1a(a2) 			;Synthetic mode off
	clr.w $1e(a2) 			;Lfo Off
	add.l #24,d7 			;24 is name->ignore
	lsl.l #2,d6 			;x4 for sample offset
	lea samples(pc),a4
	move.l -4(a4,d6),d4			;Fetch sample pointer
	beq.s bp_nosamp			;is zero->no sample
	move.l d4,(a1) 			;Sample pointer in hardware
	move.w (a3,d7),4(a1)		;length in hardware
	move.b 2(a2),9(a1)			;and volume from bpcurrent
	cmp.b #-1,2(a2)			;Use default volume
	bne.s skipxx 			;No ??
	move.w 6(a3,d7),8(a1)		;Default volume in hardware
skipxx 	move.w 4(a3,d7),8(a2)		;Length in bpcurrent
	moveq #0,d6
	move.w 2(a3,d7),d6			;Calculate repeat
	add.l d6,d4
	move.l d4,4(a2)			;sample start in bpcurrent
	cmp.w #1,8(a2)			;has sample repeat part
	bne.s bpskip6 			;Yes ??
bp_nosamp	move.l nullpoint(a6),4(a2)			;Play no sample
	bra.s bpskip10
bpskip6	move.w 8(a2),4(a1)			;Length to hardware
	move.l 4(a2),(a1)			;pointer to hardware
bpskip10	or.w #$8000,d1			;Turn on DMA for this voice
	move.w d1,$dff096			;Yeah, do it
	rts
bpplaysynthetic
	move.b #$1,$1a(a2)			;Synthetic mode on
	clr.w $e(a2) 			;EG Pointer restart
	clr.w $10(a2) 			;LFO Pointer restart
	clr.w $12(a2) 			;ADSR Pointer restart
	move.w 22(a3,d7.w),$14(a2)		;EG Delay
	addq.w #1,$14(a2)			;0 is nodelay
	move.w 14(a3,d7.w),$16(a2)		;LFO Delay
	addq.w #1,$16(a2)			;So I need correction
	move.w #1,$18(a2)			;ADSR Delay->Start immediate
	move.b 17(a3,d7.w),$1d(a2)		;EG OOC
	move.b 9(a3,d7.w),$1e(a2)		;LFO OOC
	move.b 4(a3,d7.w),$1f(a2)		;ADSR OOC
	move.b 19(a3,d7.w),$1c(a2)		;Current EG Value
	move.l tables(a6),a4		; so far so good,now what ??
	moveq #0,d3			;Pointer to waveform tables
	move.b 1(a3,d7.w),d3		;Which waveform
	lsl.l #6,d3 			;x64 is length waveform table
	add.l d3,a4
	move.l a4,(a1) 			;Sample Pointer
	move.l a4,4(a2)			;In bpcurrent
	move.w 2(a3,d7.w),4(a1)		;Length in words
	move.w 2(a3,d7.w),8(a2)		;Length in bpcurrent
	tst.b 4(a3,d7.w)			;Is ADSR on
	beq.s bpadsroff			;No ??
	move.l tables(a6),a4		;Tables
	moveq #0,d3
	move.b 5(a3,d7.w),d3		;ADSR table number
	lsl.l #6,d3 			;x64 for length
	add.l d3,a4 			;Add it
	moveq #0,d3
	move.b (a4),d3 			;Get table value
	add.b #128,d3 			;I want it from 0..255
	lsr.w #2,d3 			;Divide by 4->0..63
	cmp.b #-1,2(a2)
	bne.s bpskip99
	move.b 25(a3,d7.w),2(a2)
bpskip99	moveq #0,d4
	move.b 2(a2),d4			;Default volume
	mulu d4,d3 			;default maal init volume
	lsr.w #6,d3 			;divide by 64
	move.w d3,8(a1)			;is new volume
	bra.s bpflipper
bpadsroff	move.b 2(a2),9(a1)
	cmp.b #-1,2(a2)
	bne.s bpflipper			;No ADSR
	move.b 25(a3,d7.w),9(a1)		;So use default volume
bpflipper	move.l 4(a2),a4			;Pointer on waveform
	move.l a4,(a5) 			;Save it
	moveq #0,d3 			;Save Old waveform
	moveq #7,d4 			;data in bpbuffer
eg2loop	move.l (a4,d3.w),4(a5,d3.w)
	addq.w #4,d3 			;Copy 		
	dbra d4,eg2loop
	tst.b 17(a3,d7.w)			;EG off
	beq bpskip10			;Yes ??
	tst.b 19(a3,d7.w)			;Is there an init value for EG
	beq bpskip10			;No ??
	moveq #0,d3
	move.b 19(a3,d7.w),d3
	lsr.l #3,d3 			;Divide by 8 ->0..31
	move.b d3,$1c(a2)			;Current EG Value
	subq.w #1,d3 			;-1,DBRA correction
eg3loop	neg.b (a4)+
	dbra d3,eg3loop
	bra bpskip10
bpplayarp	lea bpper(pc),a4
	ext.w d4
	asl.w #1,d4
	move.w -2(a4,d4.w),6(a1)
	rts
bpsynth	moveq #3,d0
	lea bpcurrent(pc),a2
	lea $dff0a0,a1
	move.l bpsong(pc),a3
	lea bpbuffer(pc),a5
bpsynthloop
	tst.b $1a(a2) 			;Is synthetic sound
	beq.s bpnosynth			;No ??
	bsr.s bpyessynth			;Yes 		
bpnosynth	lea $24(a5),a5
	lea $20(a2),a2
	lea $10(a1),a1
	dbra d0,bpsynthloop
	rts
bpyessynth
	moveq #0,d7
	move.b 3(a2),d7			;Which instr. was I playing
	lsl.w #5,d7 			;x32, is length of instr.
	tst.b $1f(a2) 			;ADSR off
	beq.s bpendadsr			;Yes ??
	subq.w #1,$18(a2)			;Delay,May I
	bne.s bpendadsr			;No ??
	moveq #0,d3
	move.b 8(a3,d7.w),d3
	move.w d3,$18(a2)			;Reset Delay Counter
	move.l tables(a6),a4
	move.b 5(a3,d7.w),d3		;Which ADSR table
	lsl.l #6,d3 			;x64
	add.l d3,a4 			;This is my table
	move.w $12(a2),d3			;Get ADSR table pointer
	moveq #0,d4
	move.b (a4,d3.w),d4			;Value from table
	add.b #128,d4 			;Want it from 0..255
	lsr.w #2,d4 			;And now from 0..63
	moveq #0,d3
	move.b 2(a2),d3			;Current Volume
	mulu d3,d4 			;MultiPly with table volume
	lsr.w #6,d4 			;Divide by 64=New volume
	move.w d4,8(a1)			;Volume in hardware
	addq.w #1,$12(a2)			;Increment of ADSR pointer
	move.w 6(a3,d7.w),d4		;Length of adsr table
	cmp.w $12(a2),d4			;End of table reached
	bne.s bpendadsr			;No ??
	clr.w $12(a2) 			;Clear ADSR Pointer
	cmp.b #1,$1f(a2)			;Once
	bne.s bpendadsr			;No ??
	clr.b $1f(a2) 			;ADSR off
bpendadsr	tst.b $1e(a2) 			;LFO On
	beq.s bpendlfo			;No ??
	subq.w #1,$16(a2)			;LFO delay,May I
	bne.s bpendlfo			;No
	moveq #0,d3
	move.b 16(a3,d7.w),d3
	move.w d3,$16(a2)			;Set LFO Count
	move.l tables(a6),a4
	move.b 10(a3,d7.w),d3		;Which LFO table
	lsl.l #6,d3 			;x64
	add.l d3,a4
	move.w $10(a2),d3			;LFO pointer
	moveq #0,d4
	move.b (a4,d3.w),d4			;That's my value
	ext.w d4 				;Make it a word
	ext.l d4 				;And a longword
	moveq #0,d5
	move.b 11(a3,d7.w),d5		;LFO depth
	tst.b d5
	beq.s bpnotx
	divs d5,d4 			;Calculate it
bpnotx	move.w (a2),d5 			;Period
	add.w d4,d5 			;New Period
	move.w d5,6(a1)			;In hardware
	addq.w #1,$10(a2)			;Next position
	move.w 12(a3,d7.w),d3		;LFO table Length
	cmp.w $10(a2),d3			;End Reached
	bne.s bpendlfo			;NO ??
	clr.w $10(a2)		 	;Reset LFO Pointer
	cmp.b #1,$1e(a2)			;Once LFO
	bne.s bpendlfo			;NO ??
	clr.b $1e(a2) 			;LFO Off
bpendlfo	tst.b $1d(a2) 			;EG On
	beq bpendeg 			;No ??
	subq.w #1,$14(a2)			;EG delay,May I
	bne bpendeg 			;No
	tst.l (a5)
	beq bpendeg
	moveq #0,d3
	move.b 24(a3,d7.w),d3
	move.w d3,$14(a2)			;Set EG Count
	move.l tables(a6),a4
	move.b 18(a3,d7.w),d3		;Which EG table
	lsl.l #6,d3 			;x64
	add.l d3,a4
	move.w $e(a2),d3			;EG pointer
	moveq #0,d4
	move.b (a4,d3.w),d4			;That's my value
	move.l (a5),a4 			;Pointer to waveform
	add.b #128,d4 			;0..255
	lsr.l #3,d4 			;0..31
	moveq #0,d3
	move.b $1c(a2),d3			;Old EG Value
	move.b d4,$1c(a2)
	add.l d3,a4 			;WaveForm Position

	move.l a0,-(sp)

	move.l a5,a0 			;Buffer
	add.l d3,a0 			;Position
	addq.w #4,a0 			;For adress in buffer
	cmp.b d3,d4 			;Compare old with new value
	beq.s bpnexteg			;no change ??
	bgt bpishigh			;new value is higher
bpislow	sub.l d4,d3 			;oldvalue-newvalue
	subq.w #1,d3 			;Correction for DBRA
bpegloop1a
	move.b -(a0),d4
	move.b d4,-(a4)
	dbra d3,bpegloop1a  
	bra.s bpnexteg
bpishigh	sub.l d3,d4 			;Newvalue-oldvalue
	subq.w #1,d4 			;Correction for DBRA
bpegloop1b
	move.b (a0)+,d3
	neg.b d3
	move.b d3,(a4)+			;DoIt
	dbra d4,bpegloop1b

bpnexteg	move.l (sp)+,a0

	addq.w #1,$e(a2)			;Next position
	move.w 20(a3,d7.w),d3		;EG table Length
	cmp.w $e(a2),d3			;End Reached
	bne.s bpendeg 			;NO ??
	clr.w $e(a2) 			;Reset EG Pointer
	cmp.b #1,$1d(a2)			;Once EG
	bne.s bpendeg 			;NO ??
	clr.b $1d(a2) 			;EG Off
bpendeg	rts

 **********************
 ** Variables & data **
 **********************

bpcurrent	;dc.w 0,0				;periode,instrument =(volume.b,instr nr.b)
	;dc.l 0				;start
	;dc.w 1				;length (words)
	;dc.b 0,0,0,0 			;noot,arpeggio,autoslide,autoarpeggio
	;dc.w 0,0,0			;EG,LFO,ADSR pointers
	;dc.w 0,0,0			;EG,LFO,ADSR count
	;dc.b 0,0				;Synthetic yes/no, Volume Slide
	;dc.b 0,0				;Current EG value,EG OOC
	;dc.b 0,0				;LFO OOC,ADSR OOC


	dc.w 0,0
	dc.l 0
	dc.w 1,0,0
	dc.w 0,0,0,0,0,0,0,0,0

	dc.w 0,0
	dc.l 0
	dc.w 1,0,0
	dc.w 0,0,0,0,0,0,0,0,0

	dc.w 0,0
	dc.l 0
	dc.w 1,0,0
	dc.w 0,0,0,0,0,0,0,0,0

	dc.w 0,0
	dc.l 0
	dc.w 1,0,0
	dc.w 0,0,0,0,0,0,0,0,0


bpbuffer	dcb.b 144,0

	dc.w 6848,6464,6080,5760,5440,5120,4832,4576,4320,4064,3840,3616
	dc.w 3424,3232,3040,2880,2720,2560,2416,2288,2160,2032,1920,1808
	dc.w 1712,1616,1520,1440,1360,1280,1208,1144,1080,1016,0960,0904
bpper	dc.w 0856,0808,0760,0720,0680,0640,0604,0572,0540,0508,0480,0452
	dc.w 0428,0404,0380,0360,0340,0320,0302,0286,0270,0254,0240,0226
	dc.w 0214,0202,0190,0180,0170,0160,0151,0143,0135,0127,0120,0113
	dc.w 0107,0101,0095,0090,0085,0080,0076,0072,0068,0064,0060,0057

samples	dcb.l 15,0
null	dc.w 0

	rsreset
dma	rs.w 1
tables	rs.l 1
bpstep	rs.w 1
most	rs.w 1
nullpoint	rs.l 1
st	rs.b 1
tr	rs.b 1
bpcount	rs.b 1
bpdelay	rs.b 1
arpcount	rs.b 1
numtables	rs.b 1
bppatcount rs.b 1
bprepcount rs.b 1

varisize	rs.w 0
variables	dcb.b varisize,0

bpsong dc.l 0

	Section Buffer,BSS

bp_buffer	ds.b	$500



