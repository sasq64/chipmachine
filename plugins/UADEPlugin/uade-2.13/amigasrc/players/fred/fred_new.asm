שתשת

	incdir	asm:
	include	custom.i
	include	rmacros.i
	incdir	include:
	include	misc/deliplayer.i


	section	fredplayer,code
	moveq	#-1,d0
	rts
	dc.b	'DELIRIUM'
	dc.l	table
	dc.b	'$VER: Fred player module V0.1 for UADE (21.12.2001)',0
	dc.b	'$COPYRIGHT: Heikki Orsila <heikki.orsila@tut.fi>',0
	dc.b	'$LICENSE: GNU LGPL',0
	even

table	dc.l	DTP_PlayerName,fredmonitor
	dc.l	DTP_Creator,FredericHahn
	dc.l	DTP_Check2,Check2
	dc.l	DTP_SubSongRange,SubSongRange
	dc.l	DTP_InitPlayer,InitPlayer
	dc.l	DTP_InitSound,InitSound
	dc.l	DTP_Interrupt,Interrupt
	dc.l	DTP_EndSound,EndSound
	dc.l	DTP_EndPlayer,EndPlayer
	dc.l	$80004474,2			* songend support
	dc.l	0
fredmonitor	dc.b	'FredMonitor',0
FredericHahn	dc.b	'FRED player for UADE by shd (based on format by '
	dc.b	'Frederic Hahn)',0
	even

Check2	move.l	dtg_ChkData(a5),a0

	* type relevant byte skip for subsong detection
	moveq	#0,d2

	* check file format
	bsr	checktype1		* normal
	tst.l	d0
	beq.b	checksubs
	bsr	checktype1a		* Fuzball Title and Ingame
	tst.l	d0
	beq.b	checksubs
	bsr	checktype2		* fred.trip for example
	tst.l	d0
	bne.b	endcheck2

checksubs	* check subsongs
	move	2(a0),d0
	add	#$4,d0
	add	d2,d0			* skip type relevant amount
					* of bytes (see checktype1a)
	add	(a0,d0),d0
	lea	(a0,d0),a1
	moveq	#0,d1
	move.b	(a1),d1
	move.l	d1,maxsubsong		* this is right, okay?

	moveq	#0,d0			* ok, it's fred, i think ;-)
endcheck2	rts

checktype1	moveq	#-1,d0
	move.l	#$4efa0010,d1
	move.l	a0,a1
	cmp.l	(a1)+,d1
	bne.b	nottype1
	move	#$a4,d1
	cmp.l	(a1)+,d1
	bne	nottype1
	move	#$8e,d1
	cmp.l	(a1)+,d1
	bne.b	nottype1
	move	#$7c,d1
	cmp.l	(a1)+,d1
	bne.b	nottype1
	cmp.l	#$0f00123a,(a1)+
	bne.b	nottype1
	moveq	#0,d0
nottype1	rts

checktype1a
	moveq	#-1,d0
	move.l	#$4efa0010,d1
	move.l	a0,a1
	cmp.l	(a1)+,d1
	bne.b	nottype1a
	move	#$a2,d1
	cmp.l	(a1)+,d1
	bne	nottype1a
	move	#$94,d1
	cmp.l	(a1)+,d1
	bne.b	nottype1a
	move	#$82,d1
	cmp.l	(a1)+,d1
	bne.b	nottype1a
	addq.l	#2,a1		* word adj.
	cmp.l	#$0f004238,(a1)+
	bne.b	nottype1a
	moveq	#6,d2		* skip 6 bytes
	moveq	#0,d0
nottype1a	rts

checktype2	moveq	#-1,d0
	move.l	#$4efa004e,d1
	move.l	a0,a1
	cmp.l	(a1)+,d1
	bne.b	nottype2
	move	#$e2,d1
	cmp.l	(a1)+,d1
	bne.b	nottype2
	move	#$cc,d1
	cmp.l	(a1)+,d1
	bne.b	nottype2
	move	#$ba,d1
	cmp.l	(a1)+,d1
	bne.b	nottype2
	cmp	#$0f20,(a1)+
	bne.b	nottype2
	moveq	#0,d0
nottype2	rts
	

SubSongRange	moveq	#0,D0
	move.l	maxsubsong,d1
	rts

InitPlayer	push	all
	moveq	#0,d0
	move.l	dtg_GetListData(a5),a0
	jsr	(a0)
	move.l	a0,modptr

	move	2(a0),d0
	addq	#2,d0
	* d0 = init func offset
	add	#$24,d0
	add	(a0,d0),a0
	add	d0,a0
	* d0 = chan struct
	addq.l	#8,a0
	* d0 = chan struct + 8 = pointer to current pattern pos
	move.l	a0,sposptr
	move.l	a5,delibase
	move.l	dtg_AudioAlloc(a5),a0
	jsr	(a0)
	pull	all
	moveq	#0,d0
	rts

InitSound	push	all
	moveq	#0,d0
	move	dtg_SndNum(a5),d0
	move.l	modptr,a0
	jsr	(a0)
	clr.l	firstbeat
	clr.l	endpossible
	pull	all
	rts

Interrupt	push	all
	* call interrupt
	move.l	modptr,a0
	jsr	4(a0)

	* song end checking
	move.l	sposptr(pc),a0
	tst.l	firstbeat
	bne.b	notfirstbeat
	move.l	0*$80(a0),d0
	move.l	1*$80(a0),d1
	move.l	2*$80(a0),d2
	move.l	3*$80(a0),d3
	movem.l	d0-d3,ipos
	st	firstbeat
	bra.b	notend
notfirstbeat	move.l	0*$80(a0),d0
	move.l	1*$80(a0),d1
	move.l	2*$80(a0),d2
	move.l	3*$80(a0),d3
	movem.l	ipos,d4-d7
	cmp.l	d4,d0
	bne.b	notend1
	cmp.l	d5,d1
	bne.b	notend1
	cmp.l	d6,d2
	bne.b	notend1
	cmp.l	d7,d3
	beq.b	tryend

notend1	st	endpossible
	bra.b	notend

tryend	tst.l	endpossible
	beq.b	notend

	* end song
	move.l	delibase(pc),a5
	move.l	dtg_SongEnd(a5),a0
	jsr	(a0)

notend	pull	all
	rts

EndSound	push	all
	move.l	modptr,a0
	jsr	8(a0)
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

modptr	dc.l	0
maxsubsong	dc.l	0
sposptr	dc.l	0
firstbeat	dc.l	0
endpossible	dc.l	0
ipos	dcb.l	4,0
delibase	dc.l	0

	end
