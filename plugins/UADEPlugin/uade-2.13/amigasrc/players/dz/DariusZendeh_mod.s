;               T
* Copyright (c) 2002, Stephen Mifsud (Malta)
* All rights reserved.
*
* You may contact Stephen Mifsud by:
*  Email: teknologik@technologist.com
*  home page: www.marz-kreations.com
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
* 
*     * Redistributions of source code and binary must retain the above
*       copyright notice, this list of conditions, the file StephenMifsud.txt
*       and the following disclaimer.
*     * Redistribution in binary form makes the redistributor obligated to
*       provide the source code (modified and non-modified versions) by
*       reasonable means (email is a reasonable way) to anyone who asks
*       for it.
*     * Redistributed binaries must contain following text inside the binary:
*         Copyright (c) 2002, Stephen Mifsud (Malta)
*         email: teknologik@technologist.com
*         home page: www.marz-kreations.com
*     * Software using this code must point to this license agreement and
*       StephenMifsud.txt file in its documentation.
*     * You may not change terms of this license agreement. If you do not
*       accept some term of this agreement you have no right to use this
*       software in any way.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
* IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*****************************************************************************
**
**      PROJEKT:            Module External Players for DT / EP          
**      ~~~~~~~~            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~          
**	Module format:      Darius Zendeh mod player (old Mark-II mods)
** 	Version:		V1.30
**							   
** 	Written on: 	Devpac Amiga 3.04	
** 	By:		Dr.Who of SYNERGY (Malta)
** 	Date began:  	21/Nov/2k-3
** 	Last revision:  	25/Dec/2k-3
**
**
** 	Info:
** 	-----
** 	Mod Format Info:	old prototype of Mark II Sound System.
**
**	Player Info:	This one plays the susbsongs  as well!
**
**	Mod Examples:	R-Type 1 level music, Crush & Burn
**
*****************************************************************************

	section	player,code
	incdir	include:
	include	misc/deliplayer.i

	PLAYERHEADER PlayerTagArray

	dc.b '$VER: Darius Zendeh (old Mark II SoundSystem) Module '
	dc.b 'Player V1.3.',0

	dc.b ' The first prototype of the Mark II Sound System Editor'
	dc.b ' was originally created by the composer himself - '
	dc.b ' Darius Zendeh and was then improved by Andreas Scholl '
	dc.b ' to the popular MARK II Sound System. '
	dc.b ' The latter was distributed by Cachet Software.',0

	dc.b 'Greets to Synergy members and BuZz of MaNiaCs :)',0
	even

PlayerTagArray	dc.l	DTP_PlayerVersion,1<<16+30
	dc.l	DTP_PlayerName,PName
	dc.l	DTP_Creator,CName
	dc.l	DTP_Check2,ModChk
	dc.l	DTP_CheckLen,$500
	dc.l	DTP_Interrupt,Play
	dc.l	DTP_InitPlayer,InitPlay
	dc.l	DTP_EndPlayer,EndPlay
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_EndSound,KillSnd
	dc.l	DTP_SubSongRange,SubSongNum
	dc.l	DTP_NextSong,NextSng
	dc.l	DTP_PrevSong,PrevSng
	dc.l	DTP_FormatName,FormatPtr
	dc.l	TAG_DONE

PName	dc.b 'Darius Zendeh Player',0

CName	dc.b 'Coded by FantaZY (Stephen Mifsud) of SynerGY on '
	dc.b '(26 Dec 97) for Darius Zendeh mods (old Mark II mods '
	dc.b 'with sub songs done with an editor by Darius Zendeh). '
	dc.b 'Modified by shd for UADE. Thanks to Sunbeam for '
	dc.b 'support for some strange mods. '
	dc.b 'Copyright (c) 2002, Stephen Mifsud (Malta) '
	dc.b 'email: teknologik@technologist.com '
	dc.b 'home page: www.marz-kreations.com',0
	even

FormatPtr	dc.l	Format
Format	dc.b	"Darius Zendeh Mod - type: >"
FType	dc.b	0
	dc.b	"<",0
	even

STRANGETYPE	equ	666

M_Addr	dc.l 	0
M_Type	dc 	1
M_SongMax	dc 	0
M_SongCnt	dc	0
M_Stop	dc.b 	0
	even

ModChk	move.l	dtg_ChkData(a5),a0
	move.l	a0,M_Addr

	moveq	#-1,d0

	* check the weird type
	cmp.l	#$48e778f0,(a0)
	bne.b	chk_notstrange
	cmp	#$41fa,4(a0)
	bne.b	chk_notstrange
	cmp.l	#$4cd80600,8(a0)
	bne.b	chk_notstrange
	cmp.l	#$0c0000ff,12(a0)
	bne.b	chk_notstrange
	move	#STRANGETYPE,M_Type
	move.b	#'S',FType
	moveq	#0,d0
	rts
chk_notstrange
	* check normal types
Chk0	cmp.l  	#$48e700f0,(a0)	; Darius Zendeh modules
	bne	ChkEnd
	cmp.l	#$4cd80600,8(a0)
	bne	ChkEnd

Chk1	move	#$120,D6
	move.l	a0,a1
Chk1pos	cmp.l	#$700033fc,(a1)
	bne.b	Chk1Lp
	cmp.l	#$000f00df,4(a1)
	bne.b	Chk1Lp
	cmp.l	#$f09641fa,8(a1)
	beq.b	Chk2
Chk1Lp	addq	#2,A1
	dbra	D6,Chk1pos
	bra.b	ChkEnd

Chk2	cmp	#$4a44,$c(a0)	; With Subsongs (only RTYPE-1 levs)
	bne.b	Chk2b
	moveq	#1,d1		; type 1
	bra.b	ChkOk
Chk2b	cmp	#$4a00,$c(a0)	; One mod only (no subsongs)
	bne.b	Chk2c		; Like RSI Demo / Crush&Burn
	moveq	#2,d1		; type 2
	bra.b	ChkOk
Chk2c	cmp	#$0c00,$c(a0)	; One mod only (no subsongs)
	bne.b	ChkEnd		; Like Threat's duel cracktro
	moveq	#3,d1		; type 3
ChkOk	move	d1,M_Type
	add.b	#$30,d1
	move.b	d1,FType
	moveq	#0,d0
	clr  	M_SongMax
	clr  	M_SongCnt
ChkEnd	rts


SubSongNum	move	M_Type(pc),d7

	cmp	#STRANGETYPE,d7
	bne.b	subs_notstrange
	moveq	#0,d0
	moveq	#0,d1
	rts
subs_notstrange
	cmp	#1,d7
	bne.b	NoSub

SubTyp1	move	#2,M_SongMax	; 3 subsongs minimum (0,1,2)
	move.l	M_Addr(pc),a2
	move	#$80,d6		; Check length

FindSng	cmp	#$0C04,(a2)
	bne.b	loop1
	cmp.b	#$66,4(a2)
	bne.b	loop1
	cmp	#$41Fa,6(a2)
	beq.b	Count1
loop1	addq.l	#2,a2
	dbra	d6,FindSng
	bra.b	NoSub

Count1	move	2(a2),d1
	add	d1,M_SongMax

NoSub	moveq 	#0,d0
	move	M_SongMax(pc),d1 ; SongMax = actual Max song No. - 1
	rts

NextSng	movem.l	d0-d7/a0-a6,-(sp)

	move	M_Type(pc),d7
	cmp	#1,d7		; Subsongs for Mark 1
	bne	end_		; (type 1) mods only (RTYPE1)

	move	M_SongCnt(pc),D1
	move	M_SongMax(pc),d2
	cmp	D2,D1
	bge	end_

	movem.l	d0-d2,-(sp)
	bsr	KillSnd
	movem.l	(Sp)+,d0-d2

	addq	#1,M_SongCnt

	move	M_SongCnt,dtg_SndNum(a5)

	cmp	#1,d1
	ble.b	sng12

sng	bsr	InitSub_
	bra.b	PlayMod_

sng12	bsr.b 	InitSub1
	bra.b 	PlayModR_



PrevSng	movem.l	d0-d7/a0-a6,-(sp)

	move	M_Type(pc),d7
	cmp	#1,d7
	bne.b	end_

	move	M_SongCnt(pc),D1
	tst	D1
	ble.b	end_		; Already First one mate

	subq	#2,D1
	subq	#1,M_SongCnt

	move	M_SongCnt,dtg_SndNum(a5)

	cmp	#-1,D1
	beq.b	sng
	cmp	#2,D1
	bge.b	sng

	bra.b 	sng12


SubRoutines
InitSub1	moveq	#0,D4
	moveq	#-1,D3
	bsr.b	JumpPrg
	bra	KillSnd

StopMod	moveq	#0,D4
	moveq	#0,D3
	bra.b	JumpPrg

PlayMod	moveq	#0,D4		; Play once
	moveq	#2,D3
JumpPrg	move.l	M_Addr(pc),a0
 	jsr	(a0)
	rts

PlayModR_	moveq	#0,d4		; Repeat play
        	moveq	#2,d3
JumpPrg_	move.l	M_Addr(pc),a0
	jsr	(a0)

end_	movem.l	(sp)+,d0-d7/a0-a6
	rts


PlayMod_	tst.b	M_Stop
	bne.b	end_

	moveq	#2,d4
	bsr.b	JumpPrg
	tst.l	d4
	beq.b	end_
	move.b	#1,M_Stop

	bsr	StopMod

	movem.l	(sp)+,d0-d7/a0-a6
	rts

; Interrupt for Replay
Play	movem.l	d0-d7/a0-a6,-(sp)
	move	M_Type(pc),d7

	cmp	#STRANGETYPE,d7
	bne.b	play_notstrange
	moveq	#2,d0
	move.l	M_Addr(pc),a0
	jsr	(a0)
	movem.l	(a7)+,d0-d7/a0-a6
	moveq	#0,d0
	rts
play_notstrange

Ptype1	cmp	#1,d7		; Type 1 Rtype
	beq.b	Pset1

Pset2_3	moveq	#2,d0               ; Type 2 or 3
	bra.b	JumpPrg_

Pset1	cmp	#1,M_SongCnt      	; Repeat this subsong (2)
	beq	PlayModR_
	cmp	#2,M_SongCnt        ; Repeat this subsong (3)
	beq	PlayModR_

	bra.b	PlayMod_		; Other Subsongs play once.

; Init Player
InitPlay	move.l	dtg_AudioAlloc(a5),a0
	jsr	(a0)
	rts

; End Player
EndPlay	move.l	dtg_AudioFree(a5),a0
	jsr	(a0)
	rts

; Init Sound
InitSnd	move	dtg_SndNum(a5),M_SongCnt

	move	M_Type(pc),d7
	cmp	#STRANGETYPE,d7
	bne.b	initsnd_notstrange
	movem.l	d0-d7/a0-a6,-(a7)
	moveq	#-1,d0
	move.l	M_Addr(pc),a0
	jsr	(a0)
	movem.l	(a7)+,d0-d7/a0-a6
	moveq	#0,d0
	rts
initsnd_notstrange
	cmp	#$1,d7
	beq.b	Init_1

Init0_2_3	moveq	#-1,d0		; #-1,d0 for type 0,2,3
	bsr	JumpPrg
	bra.b	KillSnd

Init_1	cmp	#1,M_SongCnt	; Init subsong 0
	beq.b	InitSub1_
	cmp	#2,M_SongCnt	; Goto Init subsong 2
	beq.b	InitSub2_

InitSub_	move	M_SongCnt,D4
	tst	D4
	beq.b	SetSng0
	subq	#2,D4
SetSng0	lsl	#8,D4
	or.b	#$ff,D4		; 0    3    4    5  etc
	bsr	JumpPrg		; 0ff, 1ff, 2ff, 3ff in D4
	bra.b	KillSnd

InitSub1_	moveq	#0,D1		; Init SubSong 1
	bra	InitSub1

InitSub2_	moveq	#1,D1		; Init Subsong 2
	bra	InitSub1


; Remove Sound
KillSnd	lea	$dff000,a0
	moveq	#0,d0
	move	d0,$a8(a0)
	move	d0,$b8(a0)
	move	d0,$c8(a0)
	move	d0,$d8(a0)
	move	#$000f,$96(a0)
	clr.b	M_Stop
	rts
