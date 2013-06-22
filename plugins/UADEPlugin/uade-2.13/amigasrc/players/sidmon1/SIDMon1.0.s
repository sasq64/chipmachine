	incdir	"amiga:includes"
	include "misc/DeliPlayer.i"

	SECTION Player,Code

	PLAYERHEADER PlayerTagArray
	dc.b '$VER: SIDMon 1.0 player module '
	dc.b 'for UADE (23 Dec. 2001)',0
	dc.b '$LICENSE: GNU LGPL',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,2
	dc.l	DTP_PlayerName,PlayerName
	dc.l	DTP_Creator,Credits
	dc.l	DTP_Check2,SID1_Check
	dc.l	DTP_Interrupt,SID1_Interrupt
	dc.l	DTP_InitPlayer,SID1_InitPlayer
	dc.l	DTP_EndPlayer,SID1_EndPlayer
	dc.l	DTP_InitSound,SID1_InitSnd
	dc.l	DTP_EndSound,SID1_EndSnd
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	TAG_DONE


PlayerName	dc.b 'SIDMon 1.0',0
Credits:	dc.b 'Reinier van Vliet,',10
		dc.b 'adapted for UADE by mld',0

SidmonText	DC.B	' SID-MON BY R.v.VLIET  (c) 1988 ',0
	even

*-----------------------------------------------------------------------*
SID1_Check
	move.l	dtg_ChkData(a5),a0
	move.l	dtg_ChkSize(a5),d1
	move.l	a5,delibase

CheckLoop1:
	cmp.w	#$41fa,(a0)
	bne	cl1next
	cmp.l	#$ffd443fa,6(a0)
	bne	cl1next
	cmp.l	#$228841fa,$c(a0)
	bne	cl1next
	cmp.l	#$d1e8ffd8,$12(a0)
	bne	cl1next
	cmp.w	#$43fa,$16(a0)
	beq	SID1_CheckOk

cl1next:
	addq.l	#2,a0
	dbra d1,checkloop1

SID1_Checkfail
	moveq	#-1,d0
	bra	SID1_CheckEnd

SID1_CheckOk
	move.l	a0,SID1_init_offset		; save init address
	moveq	#0,d0
SID1_CheckEnd
	rts	

;--------------
; Init:		0x00: 41fa aa aa xx xx ff d4 43 fa 
; aa aa -> rel. offset to Sidmessage
;
; Playroutine: 4e7f 48e7 xx xx 4dfa
; EnigmaGun = 		0x13e
; BeatToThePulp = 	0x13e
; BlackMonks	=	0x16a
; BlackMonk	=	0x16a
; E.A.R		=	0x164	(jmp->rts)
; Raggae	=	0x182	(jmp->rts)
; Seat		=	0x162	(jmp->rts)
; Arpeggiator	=	0x16a


SID1_InitPlayer
	move.l	SID1_init_offset(pc),a0
	move.w	2(a0),d1			; offset to sid1msg

initloop:
	cmp.w	#$4e75,(a0)
	bne	nextinitloop
	cmp.l	#$48e7fffe,2(a0)
	beq	SID1_Init_ok

nextinitloop:
	addq.l	#1,a0
	dbra	d1,initloop

SID1_Init_fail:
	moveq	#-1,d0
	rts
	
SID1_Init_ok:
	addq.l	#2,a0
	move.l	a0,SID1_play_offset


	bsr	patchjmps			; patch jmps -> rts

	bsr	SongEndPatch1			; patch sid1 to support songend
	bsr	SongEndPatch2

	move.l	dtg_AudioAlloc(a5),a0		; allocate audio
	jsr	(a0)
	rts

;-------------
SID1_Interrupt
	movem.l	d0-d6/a0-a6,-(sp)
	move.l	SID1_play_offset(pc),a0
	jsr	(a0)
	movem.l	(sp)+,d0-d6/a0-a6
	rts	

;-------------
SID1_EndPlayer
	move.l	dtg_AudioFree(a5),a0		; free audio
	jsr	(a0)
	rts

;-----------
SID1_InitSnd
	move.l	SID1_init_offset(pc),a0		; call sid1 init
	jmp	(a0)

;-----------
SID1_EndSnd
	lea	$dff000,a0
	moveq	#0,d0
	move.w	d0,$a8(a0)			; stop audio
	move.w	d0,$b8(a0)
	move.w	d0,$c8(a0)
	move.w	d0,$d8(a0)			
	move.w	#$000F,$96(a0)
	rts

;-----------
patchjmps:
	move.l	SID1_init_offset,a0
	move.w	2(a0),d1

patchjmpsloop:
	cmp.l	#$4cdf7fff,(a0)
	bne.s	nextpatchjmps
	cmp.w	#$4ef9,4(a0)
	bne.s	nextpatchjmps
	cmp.w	#$4dfa,10(a0)
	bne.s	nextpatchjmps
	move.w	#$4e75,4(a0)	; patch jmp to rts

nextpatchjmps
	addq.l	#1,a0
	dbra	d1,patchjmpsloop

	rts


;-----------
SongEndPatch1:
	move.l	dtg_ChkData(a5),a1
	move.l	dtg_ChkSize(a5),d1
	lea.l	$800(a1),a0
SEP_loop1:
	cmp.l	#$06900000,(a1)
	bne.b	next
	cmp.l	#$00012011,4(a1)
	bne.b	next
	cmp.l	#$b0906200,8(a1)
	bne.b	next
	cmp.l	#$20bc0000,14(a1)
	bne.b	next
	lea.l	songendfunc1(pc),a2
	move	jsrcom,14(a1)			; jsr
	move.l	a2,16(a1)
	;bra.b	patched1
next	addq.l	#2,a1
	cmp.l	a1,a0
	bne.b	SEP_loop1

	move.l	dtg_ChkData(a5),a1
	move.l	dtg_ChkSize(a5),d1
	lea.l	$800(a1),a0
SEP_loop1a:
	cmp.l	#$45fa0db8,(a1)
	bne.b	next1a
	cmp.l	#$52902011,4(a1)
	bne.b	next1a
	cmp.l	#$b090620c,8(a1)
	bne.b	next1a
	cmp.l	#$20bc0000,12(a1)
	bne.b	next1a
	lea.l	songendfunc1(pc),a2
	move	jsrcom,12(a1)			; jsr
	move.l	a2,14(a1)
	;bra.b	patched1
next1a	addq.l	#2,a1
	cmp.l	a1,a0
	bne.b	SEP_loop1a
patched1:
	rts

SongEndPatch2:
	move.l	dtg_ChkData(a5),a1
	move.l	dtg_ChkSize(a5),d1
	lea.l	$800(a1),a0
SEP_loop2:
	cmp.l	#$00060c96,(a1)
	bne.b	next2
	cmp.l	#$00000001,4(a1)
	bne.b	next2
	cmp.l	#$66000006,8(a1)
	bne.b	next2
	cmp.l	#$2455208a,12(a1)
	bne.b	next2
	lea.l	songendfunc2(pc),a2
	move	jsrcom,8(a1)			; jsr
	move.l	a2,10(a1)
	move	nopcom,14(a1)	
	bra.b	patched2
next2	addq.l	#2,a1
	cmp.l	a1,a0
	bne.b	SEP_loop2
patched2:
	rts

jsrcom:	jsr	0
nopcom:	nop
SongEndfunc1:
	move.l	#1,(a0)
	bsr	SongEndfunction
	rts

SongEndfunc2:
	cmp.l	#1,(a6)
	bne	SongEndfunc2End
	bsr	SongEndfunction
	dc.l	$2455208a
SongEndfunc2End:
	rts

SongEndfunction:
	movem.l	d0-d6/a0-a6,-(a7)
	move.l	delibase(pc),a5
	move.l	$5c(a5),a0
	jsr	(a0)				; Call songend
	movem.l	(a7)+,d0-d6/a0-a6
	rts

delibase:		dc.l	0
SID1_init_offset:	dc.l	0
SID1_play_offset:	dc.l	0
