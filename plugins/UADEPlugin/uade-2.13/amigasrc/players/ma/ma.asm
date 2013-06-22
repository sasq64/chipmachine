;               T

	incdir	asm:
	include	rmacros.i
	include	custom.i
	incdir	include:
	include	misc/deliplayer.i


	section	maplayer,code
	moveq	#-1,d0
	rts
	dc.b	'DELIRIUM'
	dc.l	table
	dc.b	'$VER: Music-Assembler player module V0.1 '
	dc.b	'for UADE (21.12.2001)',0
	dc.b	'$COPYRIGHT: Heikki Orsila <heikki.orsila@tut.fi>',0
	dc.b	'$LICENSE: GNU LGPL',0
	even

table	dc.l	DTP_PlayerName,playername
	dc.l	DTP_Creator,creator
	dc.l	DTP_Check2,Check2
	dc.l	DTP_SubSongRange,SubSongRange
	dc.l	DTP_InitPlayer,InitPlayer
	dc.l	DTP_InitSound,InitSound
	dc.l	DTP_Interrupt,Interrupt
	dc.l	DTP_EndSound,EndSound
	dc.l	DTP_EndPlayer,EndPlayer
	dc.l	DTP_Volume,setvolume
	dc.l	$80004474,2			* songend support
	dc.l	0
playername	dc.b	'Music-Assembler',0
eator	dc.b	'MA-player for UADE by shd, ',10
	dc.b	'Thanks to Laurent Clevy for format info, ',10
	dc.b	'Based on format by Oscar Giesen and '
	dc.b	'Marco Swagerman',0
	even

Check2	move.l	dtg_ChkData(a5),a0
	* check file format
	bsr	checktype1
	rts

checktype1	moveq	#-1,d0
	cmp.l	#'sa-t',$16c(a0)
	bne.b	nottype1
	cmp.l	#'eam ',$170(a0)
	bne.b	nottype1
	cmp.l	#'dyna',$234(a0)
	bne.b	nottype1
	moveq	#0,d0
nottype1	rts


SubSongRange	moveq	#0,D0
	moveq	#9,d1
	rts

InitPlayer	push	all
	moveq	#0,d0
	move.l	dtg_GetListData(a5),a0
	jsr	(a0)
	move.l	a0,modptr
	move.l	a5,delibase
	move.l	dtg_AudioAlloc(a5),A0
	jsr	(a0)
	pull	all
	moveq	#0,d0
	rts

InitSound	push	all
	moveq	#0,d0
	move	dtg_SndNum(a5),d0
	move.l	modptr,a0
	jsr	(a0)
	move.l	dtg_ChkData(a5),a0
	add	#$400,a0
	move	#$400/2-1,d7
sloop	move.l	(a0),d0
	and.l	#$ffffff0f,d0
	cmp.l	#$00dff000,d0
	bne.b	notthis
	move.l	$26(a0),d0
	and.l	#$ffffff0f,d0
	cmp.l	#$00dff000,d0
	bne.b	notthis
	move.l	a0,chandataptr
	bra.b	endsloop
notthis	addq.l	#2,a0
	dbf	d7,sloop
endsloop	pull	all
	rts

Interrupt	push	all
	pushr	a5
	* call interrupt
	move.l	modptr,a0
	jsr	12(a0)
	pullr	a5
	move.l	chandataptr,a0
	add	#10,a0
	move	(a0),d0
	move	$26*1(a0),d1
	move	$26*2(a0),d2
	move	$26*3(a0),d3
	tst.l	firsttime
	bne.b	notfirsttime
	movem	d0-d3,startdata
	st	firsttime
	bra.b	wasfirsttime
notfirsttime	movem	startdata,d4-d7
	cmp	d0,d4
	bne.b	makeendpossible
	cmp	d1,d5
	bne.b	makeendpossible
	cmp	d2,d6
	bne.b	makeendpossible
	cmp	d3,d7
	bne.b	makeendpossible
	tst.l	endpossible
	beq.b	wasfirsttime
	move.l	dtg_SongEnd(a5),a0
	jsr	(a0)
	bra.b	wasfirsttime
makeendpossible	st	endpossible
wasfirsttime	pull	all
	rts

EndSound	push	all
	lea	$dff000,a2
	moveq	#0,d0
	move	d0,aud0vol(a2)
	move	d0,aud1vol(a2)
	move	d0,aud2vol(a2)
	move	d0,aud3vol(a2)
	move	#$000f,dmacon(a2)
	pull	all
	rts

EndPlayer	move.l	dtg_AudioFree(a5),A0
	jsr	(a0)
	rts

setvolume	push	all
	moveq	#0,d0
	move	dtg_SndVol(a5),d0
	moveq	#15,d1
	move.l	modptr,a0
	jsr	8(a0)
	pull	all
	rts

modptr	dc.l	0
delibase	dc.l	0

chandataptr	dc.l	0
firsttime	dc.l	0
endpossible	dc.l	0
counter	dc.l	0
startdata	dcb	4,0

	end
