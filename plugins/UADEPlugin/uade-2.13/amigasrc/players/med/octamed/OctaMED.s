**
** DeliTracker Player for (see below)
**
	incdir	"Amiga:includes/"
	include	"silva/easy.i"
	include	"misc/DeliPlayer.i"
	include "misc/Devpacmacros.i"

	PLAYERHEADER PlayerTagArray
	dc.b	'$VER: MED/Octamed 8ch player for UADE (20060409)',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,Name
	dc.l	DTP_ModuleName,MNamePTR
	dc.l	DTP_Creator,Comment

	dc.l	DTP_Check2,Checky
	dc.l	DTP_InitPlayer,InitPly
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_StartInt,StartInt
	dc.l	DTP_StopInt,StopInt
	dc.l	DTP_EndPlayer,EndPly
	dc.l	DTP_FormatName, FormatPtr

	dc.l	DTP_SubSongRange,SubsongRange
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	TAG_DONE

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
MNamePTR dc.l	MName
Name	dc.b	"MED/OctaMED (8ch)",0
Comment	dc.b	"MED/OctaMED pro8player 7.0 replay",10
        dc.b    "(c) by Teijo Kinnunen & Ray Burt Frost",10
	dc.b	"http://www.med.uk.com .",10
	dc.b	"adapted for uade by mld",0
MName
	dc.b	"<no songtitle>"
	dc.b	0
	even

mmd0	dc.l	0	; pointer to our mod
dtg	dc.l	0	; delibase

FormatPtr	dc.l	FormatType
FormatType	dc.b	"type: "
Format		dc.l	"MMDx"
		dc.b	0
		even
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

Checky:
	moveq	#-1,d0
	move.l	dtg_ChkData(a5),a0
	move.l	a0,mmd0			; save med module pointer
	move.l	(a0),a1
	sub.l	#$4d4d4430,a1		; clear MMDx

	cmp.l	#0,a1			; MMD0-2 ???
	blt	c_rts
	cmp.l	#3,a1
	bgt	c_rts

	move.l	(a0),Format		; save type
	move.l	a5,dtg
	add.l	mmd_songinfo(a0),a0
	btst	#6,msng_flags(a0)	; 5-8 channel ?
	beq.s	c_rts
	cmp.l	#0,a1			; MMD0 ???
	beq	c_ok

	tst.b	msng_flags2(a0)		; mixing?
	bmi.s	c_rts	
	bsr	fastmemrecomended
	move.w	d0,_fastmemplay8

c_ok	moveq	#0,d0
c_rts	rts


****

InitPly
	moveq	#0,d0
	move.l	dtg_AudioAlloc(a5),a0
	jsr	(a0)

	move.l	mmd0,a0
	bsr	fastmemrecomended
	move.w	d0,_fastmemplay8

	move.l	mmd0,a2
	jsr	_RelocModule
	jsr	_InitPlayer8
	bsr.w	SetMname
	rts

EndPly
	move.l	dtg_AudioFree(a5),a0
	jmp	(a0)
	jsr	_AudioRem
	rts
	
InitSnd:
	bsr	uade_playtable_cls
	move.l	dtg(pc),a5
	move.w	dtg_SndNum(a5),_modnum8
	sub.w	#1,_modnum8
	rts

StartInt:
	move.l	mmd0,a0
	move.b	#1,_hq			; HQ playing
	jsr	_PlayModule8
	rts

StopInt:
	jsr	_StopPlayer8
	move.w	#$f,$dff096
	rts

SubSongrange:
	moveq	#1,d0		; min
	moveq	#1,d1		; max
	move.l	mmd0,a0
ssrloop:
	move.l	mmd_expdata(a0),a1
	tst.l	(a1)
	beq	ssr_end
	addq	#1,d1
	move.l	(a1),a0	
	bra	ssrloop
ssr_end	rts

SetMName:
	move.l	mmd0,a0
	move.l	mmd_expdata(a0),d1	;expdatablock ?
	beq.s	SetNoMName

	move.l	d1,a1
	tst.l	48(a1)			; how long
	beq SetNoMName	
	move.l	44(a1),d1		; title
	beq.s	SetNoMName
	move.l	d1,MNamePTR
	rts
	
	SetNoMname:
	rts


SongEnd:
	movem.l	d0-d6/a0-a6,-(sp)
	move.l	dtg(pc),a5
	move.l	dtg_SongEnd(a5),a0
	jsr	(a0)
	movem.l	(sp)+,d0-d6/a0-a6
	rts	

uade_playtable_setd1:
	movem.l	d0-d6/a0-a6,-(sp)
	move.l	d1,d0
	bra	ups1
uade_playtable_set:
	movem.l	d0-d6/a0-a6,-(sp)
ups1:	lea.l	uade_playtable(pc),a0
	moveq	#7,d1
	sub	d0,d1	; bit position
	lsr	#3,d0	; byte position
	add	d0,a0
	btst	d1,(a0)
	beq	uade_playtable_nvb	; not visited before
	st	uade_songend_flag
uade_playtable_nvb:
	bset	d1,(a0)	; set bit
	movem.l	(sp)+,d0-d6/a0-a6
	rts

uade_playtable_cls:
	movem.l	d0-d6/a0-a6,-(sp)
	sf	uade_songend_flag
	move.w	#256/8,d0
	lea.l	uade_playtable(pc),a0
clearplayt:
	move.b	#0,(a0)+
	dbra	d0,clearplayt
	movem.l	(sp)+,d0-d6/a0-a6
	rts

fastmemrecomended:
		;-- from ocSS_src/loadmod.a ++
		movem.l	d2/a2,-(sp)
		move.l	a0,d1
		beq.s	fmpr_ret0
		movea.l	mmd_songinfo(a0),a1
		moveq	#0,d1
		move.b	msng_numsamples(a1),d1
		move.l	24(a0),d0	;sample array
		beq.s	fmpr_nosamples
		movea.l	d0,a1
		move.l	d1,d0
		subq.l	#1,d0
.1		move.l	(a1)+,d2
		beq.s	.2
		movea.l	d2,a2		;instrument address
		move.l	(a2),d2		;length
		swap	d2		;upper word...
		lsr.w	#1,d2
		bne.s	fmpr_ret1	;length >= 131072
.2		dbra	d0,.1
fmpr_nosamples	move.l	mmd_expdata(a0),d0
		beq.s	fmpr_ret0
		movea.l	d0,a1
		move.l	4(a1),d0	;exp_smp
		beq.s	fmpr_ret0
		move.w	8(a1),d1
		beq.s	fmpr_ret0
		move.w	10(a1),d2
		cmp.w	#18,d2
		blt.s	fmpr_ret0	;no long repeat
		movea.l	d0,a1
		subq.w	#1,d1
.1		move.l	10(a1),d0
		or.l	14(a1),d0
		lsr.l	#1,d0
		bcs.s	fmpr_ret1	;odd... return 1
		swap	d0
		tst.w	d0
		bne.s	fmpr_ret1	;start/len >= 131072
		adda.w	d2,a1
		dbra	d1,.1
fmpr_ret0	moveq	#0,d0
fmpr_ret	movem.l	(sp)+,d2/a2
		rts
fmpr_ret1	moveq	#1,d0
		bra.s	fmpr_ret

uade_playtable:    dcb.b	256/8,0
uade_songend_flag: dc.b		0
		even

		incdir	"Amiga:work/players/med/common/"
		include	"reloc.a"
		include "pro8player.a"
