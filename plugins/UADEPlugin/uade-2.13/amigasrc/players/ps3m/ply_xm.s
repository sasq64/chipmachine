
********** Fasttracker ][ XM player **************

	basereg data,a5
	
xm_init
	lea	data,a5
	move.l	s3m(a5),a0
	lea	xmName(a0),a1
	move.l	a1,mname(a5)

	lea	xmNum1A(a0),a2
	moveq	#18,d0
.t	cmp.b	#' ',-(a2)
	bne.b	.x
	dbf	d0,.t
.x	clr.b	1(a2)

	move	xmSpeed(a0),d0
	iword	d0
	tst	d0
	bne.b	.ok
	moveq	#6,d0
.ok	move	d0,spd(a5)

	move	xmTempo(a0),d0
	iword	d0
	cmp	#32,d0
	bhi.b	.qw
	moveq	#125,d0
.qw	move	d0,tempo(a5)

	move	xmFlags(a0),d0
	iword	d0
	move	d0,sflags(a5)

	tst	PS3M_reinit(a5)
	bne	xm_skipinit

	move	xmSongLength(a0),d0
	iword	d0
	move	d0,slen(a5)
	move	d0,positioneita(a5)

	moveq	#0,d0
	move.b	xmNumChans(a0),d0
	move	d0,numchans(a5)
	addq	#1,d0
	lsr	#1,d0
	move	d0,maxchan(a5)			;!!!

	move	xmNumInsts(a0),d0
	iword	d0
	move	d0,inss(a5)

	lea	xmHdrSize(a0),a1
	move.l	(a1),d0
	ilword	d0
	add.l	d0,a1
	lea	xm_patts,a2
	move	xmNumPatts(a0),d7
	iword	d7
	subq	#1,d7
.pattloop
	move.l	a1,(a2)+
	move.l	a1,a3				; xmPattHdrSize
	tlword	(a3)+,d0
	lea	xmPattDataSize(a1),a3
	add.l	d0,a1
	moveq	#0,d0
	tword	(a3)+,d0
	add.l	d0,a1
	dbf	d7,.pattloop

	lea	xm_insts,a2
	move	inss(a5),d7
	subq	#1,d7
.instloop
	moveq	#0,d5				; instlength
	move.l	a1,(a2)+
	move.l	a1,a3				; xmInstSize
	tlword	(a3)+,d0
	lea	xmNumSamples(a1),a3
	tword	(a3)+,d1
	lea	xmSmpHdrSize(a1),a3
	add.l	d0,a1
	tst	d1
	beq	.q
	tlword	(a3)+,d2			; xmSmpHdrSize
	move	d2,d6
	mulu	d1,d6
	lea	(a1,d6.l),a4			; sample start
	subq	#1,d1
.ll	move.l	a1,a3				; xmSmpLength
	tlword	(a3)+,d0
	tst.l	d0
	beq.b	.e
	add.l	d0,d5

	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8

; Dedelta the samples

.bit16	moveq	#0,d4
	move.l	a4,a6
.l3	move.b	(a4)+,d3
	move.b	(a4)+,d6
	lsl	#8,d6
	move.b	d3,d6
	add	d4,d6
	move	d6,d4
	lsr	#8,d6
	move.b	d6,(a6)+
	subq.l	#2,d0
	bne.b	.l3
	bra.b	.e

.bit8	moveq	#0,d4
.l2	add.b	(a4),d4
	move.b	d4,(a4)+
	subq.l	#1,d0
	bne.b	.l2

.e	add.l	d2,a1
	dbf	d1,.ll

	add.l	d5,a1

.q	dbf	d7,.instloop


xm_skipinit
	clr	pos(a5)
	clr	rows(a5)
	clr	cn(a5)
	clr	pdelaycnt(a5)
	clr	pjmpflag(a5)
	clr	pbrkflag(a5)
	clr	pbrkrow(a5)
	move	#64,globalVol(a5)

	lea	pantab(a5),a1
	move.l	a1,a2
	moveq	#7,d0
.l9	clr.l	(a2)+
	dbf	d0,.l9

	move	numchans(a5),d0
	subq	#1,d0
	moveq	#0,d1
.lop	tst	d1
	beq.b	.vas
	cmp	#3,d1
	beq.b	.vas
.oik	move.b	#-1,(a1)+
	bra.b	.je
.vas	move.b	#1,(a1)+
.je	addq	#1,d1
	and	#3,d1
	dbf	d0,.lop

	move.l	#8363*1712/4,clock(a5)		; Clock constant
	move	#1,fformat(a5)			; signed samples

	moveq	#0,d1
	move.b	xmOrders(a0),d1
	lsl.l	#2,d1
	lea	xm_patts,a1
	move.l	(a1,d1),a1

	lea	xmNumRows(a1),a3
	tword	(a3)+,d0
	move	d0,plen(a5)
	move.l	a1,a3
	tlword	(a3)+,d0
	add.l	d0,a1
	move.l	a1,ppos(a5)

	st	PS3M_reinit(a5)
	moveq	#0,d0
	rts


xm_music
	lea	data,a5
	move.l	s3m(a5),a0
	pea	xm_runEnvelopes(pc)

	addq	#1,cn(a5)
	move	cn(a5),d0
	cmp	spd(a5),d0
	beq	xm_newrow

xm_ccmds
	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	subq	#1,d7

.loo	moveq	#0,d0
	move.b	vol(a2),d0
	cmp.b	#$60,d0
	blo.b	.eivol

	lea	xm_cvct(pc),a1
	moveq	#$f,d1
	and	d0,d1
	move	d1,d2
	lsr	#4,d0
	subq	#6,d0
	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.eivol	lea	xm_cct(pc),a1
	moveq	#0,d0
	move.b	cmd(a2),d0
	cmp.b	#$20,d0
	bhi.b	.edi
	moveq	#0,d1
	move.b	info(a2),d1
	beq.b	.zero
	move.b	d1,lastcmd(a2)
.zero	moveq	#0,d2
	move.b	lastcmd(a2),d2
	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.edi	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.loo
	rts


xm_runEnvelopes
	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	subq	#1,d7
.envloop
	move.l	sample(a2),d1
	beq	.skip

	move.l	d1,a1	

	move	rVolume(a2),d0

	tst.b	volEnvOn(a2)
	beq	.nsus

	tst.b	fading(a2)
	beq.b	.envvol

	mulu	fadeOut(a2),d0
	swap	d0

	tst	fadeOut(a2)
	beq.b	.envvol

	lea	xmVolFadeout(a1),a3
	moveq	#0,d1
	tword	(a3)+,d1
	moveq	#0,d2
	move	fadeOut(a2),d2
	sub.l	d1,d2
	bpl.b	.ok
	moveq	#0,d2
.ok	move	d2,fadeOut(a2)

.envvol
	moveq	#0,d1
	move.b	xmNumVolPnts(a1),d1
	lea	xmVolEnv(a1),a3
	subq	#1,d1
	bcs.b	.xtst
.xlop	tword	(a3)+,d2
	addq	#2,a3
	cmp	volEnvX(a2),d2
	bcc.b	.xfnd
	dbra	d1,.xlop
.xfnd	subq	#4,a3
.xtst
	move.l	a3,a0
	tword	(a0)+,d2
	cmp	volEnvX(a2),d2
	bne.b	.points
.point
	lea	2(a3),a0
	tword	(a0)+,d3
	lsl	#8,d3				; y
	bra.b	.vol
.points
	lea	-4(a3),a0
	tword	(a0)+,d2			; x
	tword	(a0)+,d3			; y
	tword	(a0)+,d4
	sub	d2,d4				; tx
	tword	(a0)+,d5
	sub	d3,d5				; ty
	lsl	#8,d3
	ext.l	d5
	lsl.l	#8,d5

	move	volEnvX(a2),d1
	sub	d2,d1
	muls	d4,d1
	beq.b	.vol
	divs	d1,d5
	add	d5,d3
.vol
	muls	d3,d0
	bpl.b	.vtst
	moveq	#0,d0
	bra.b	.sust
.vtst	lsl.l	#2,d0
	swap	d0
	moveq	#64,d1
	cmp	d0,d1
	bcc.b	.sust
	move	d1,d0
.sust
	tst.b	volSustained(a2)
	bne	.nsus

	btst	#xmEnvOn,xmVolType(a1)
	beq.b	.ninc

	moveq	#0,d1
	move.b	xmNumVolPnts(a1),d1
	bmi.b	.ninc
	lsl	#2,d1
	subq	#4,d1
	lea	xmVolEnv(a1),a3
	add	d1,a3
	tword	(a3)+,d1
	cmp	volEnvX(a2),d1
	bls.b	.ninc
	addq	#1,volEnvX(a2)
.ninc
	btst	#xmEnvLoop,xmVolType(a1)
	beq.b	.nloo

	moveq	#0,d1
	move.b	xmVolLoopEnd(a1),d1
	bmi.s	.nloo
	lsl	#2,d1
	lea	xmVolEnv(a1),a3
	add	d1,a3
	tword	(a3)+,d1
	cmp	volEnvX(a2),d1
	bhi.b	.nloo
	moveq	#0,d1
	move.b	xmVolLoopStart(a1),d1
	bmi.s	.nloo
	lsl	#2,d1
	lea	xmVolEnv(a1),a3
	add	d1,a3
	tword	(a3)+,d1
	move	d1,volEnvX(a2)
.nloo
	btst	#xmEnvSustain,xmVolType(a1)
	beq.b	.nsus

	tst.b	keyoff(a2)
	bne.s	.nsus
	moveq	#0,d1
	move.b	xmVolSustain(a1),d1
	bmi.b	.nsus
	lsl	#2,d1
	lea	xmVolEnv(a1),a3
	add	d1,a3
	tword	(a3)+,d1
	cmp	volEnvX(a2),d1
	bne.b	.nsus	
	st	volSustained(a2)
.nsus
	cmp	#64,globalVol(a5)
	beq.b	.skipgvol

	mulu	globalVol(a5),d0
	lsr	#6,d0
.skipgvol
	move	d0,mVolume(a4)

.skip	btst	#0,sflags+1(a5)
	beq.b	.amigaperiods

	moveq	#0,d0
	move	rPeriod(a2),d0
	divu	#768,d0
	move	d0,d1
	swap	d0
	lsl	#2,d0
	lea	xm_linFreq(pc),a0
	move.l	(a0,d0),d0
	lsr.l	d1,d0
	move.l	d0,d1
	move.l	#8363*1712,d0
	bsr	divu_32

	move	d0,mPeriod(a4)
	bra.b	.k

.amigaperiods
	move	rPeriod(a2),mPeriod(a4)

.k	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.envloop
	rts


xm_newrow
	clr	cn(a5)

	tst	pdelaycnt(a5)
	bne	.process

	move	pos(a5),d0
	moveq	#0,d1
	move.b	xmOrders(a0,d0),d1	
	lsl	#2,d1
	lea	xm_patts,a1
	move.l	(a1,d1),a1
	addq.l	#xmPattDataSize,a1
	tst.b	(a1)+
	bne.b	.pattok
	tst.b	(a1)+
	bne.b	.pattok

	lea	c0(a5),a2		;chanblocks
	move	numchans(a5),d7
	subq	#1,d7
.luu	clr.l	(a2)
	clr.b	info(a2)
	lea	s3mChanBlock_SIZE(a2),a2
	dbf	d7,.luu
	bra.b	.process

.pattok	move.l	ppos(a5),a1
	lea	c0(a5),a2		;chanblocks
	move	numchans(a5),d7
	subq	#1,d7
.loo	move.b	(a1)+,d0
	bpl.b	.all

	clr.l	(a2)
	clr.b	info(a2)

	btst	#0,d0
	beq.b	.nonote
	move.b	(a1)+,(a2)
.nonote	btst	#1,d0
	beq.b	.noinst
	move.b	(a1)+,inst(a2)
.noinst	btst	#2,d0
	beq.b	.novol
	move.b	(a1)+,vol(a2)
.novol	btst	#3,d0
	beq.b	.nocmd
	move.b	(a1)+,cmd(a2)
.nocmd	btst	#4,d0
	beq.b	.next
	move.b	(a1)+,info(a2)
	bra.b	.next
	
.all	move.b	d0,(a2)
	move.b	(a1)+,inst(a2)
	move.b	(a1)+,vol(a2)
	move.b	(a1)+,cmd(a2)
	move.b	(a1)+,info(a2)

.next	lea	s3mChanBlock_SIZE(a2),a2
	dbf	d7,.loo
	move.l	a1,ppos(a5)

.process
	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	subq	#1,d7
.channelloop
	tst	pdelaycnt(a5)
	bne	.skip

	tst	(a2)
	beq	.skip

	moveq	#0,d0
	move.b	(a2),d0
	bne.b	.note
	move.b	note(a2),d0
.note	move.b	d0,note(a2)

	moveq	#0,d1
	move.b	inst(a2),d1
	beq.b	.esmp

	cmp	inss,d1
	bgt.b	.esmp

	lsl	#2,d1
	lea	xm_insts,a1
	move.l	-4(a1,d1),a1

	move.l	a1,sample(a2)
	bra.b	.ju
.esmp	move.l	sample(a2),d2
	beq	.skip
	move.l	d2,a1

.ju	moveq	#$f,d1
	and.b	cmd(a2),d1
	cmp	#$e,d1
	bne.b	.s
	move.b	info(a2),d1
	and	#$f0,d1
	cmp	#$d0,d1
	beq	.skip

.s	bsr	xm_getInst
	beq	.skip

	tst.b	inst(a2)
	beq	.smpok

; Handle envelopes
	move	#$ffff,fadeOut(a2)
	clr.b	fading(a2)
	clr.b	keyoff(a2)

	move.l	sample(a2),d2
	beq	.skip
	move.l	d2,a3

	btst	#xmEnvOn,xmVolType(a3)
	beq.b	.voloff

	clr	volEnvX(a2)
	st	volEnvOn(a2)
	clr.b	volSustained(a2)
	bra.b	.jep

.voloff	clr.b	volEnvOn(a2)

.jep	btst	#xmEnvOn,xmPanType(a3)
	beq.b	.panoff

	st	panEnvOn(a2)
	clr.b	panSustained(a2)
	bra.b	.jep2

.panoff	clr.b	panEnvOn(a2)

.jep2	move.b	xmVolume(a1),volume+1(a2)
	cmp	#64,volume(a2)
	bls.b	.e
	move	#64,volume(a2)
.e	move	volume(a2),rVolume(a2)

	tst.b	(a2)
	beq.b	.smpok

	lea	xmLoopStart(a1),a3
	tlword	(a3)+,d1
	lea	xmLoopLength(a1),a3
	tlword	(a3)+,d2

	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8
	lsr.l	#1,d1
	lsr.l	#1,d2
.bit8	add.l	a6,d1

	move.l	d1,mLStart(a4)
	move.l	d2,mLLength(a4)
	cmp.l	#2,d2
	bhi.b	.ok

	clr.b	mLoop(a4)
	st.b	mOnOff(a4)
	bra.b	.smpok

.ok	moveq	#xmLoopType,d1
	and.b	xmSmpFlags(a1),d1
	sne	mLoop(a4)

.smpok	tst.b	(a2)
	beq	.skip

	cmp.b	#97,(a2)			; Key off -note
	beq	.keyoff

	bsr	xm_getPeriod

	cmp.b	#3,cmd(a2)
	beq	.tonep
	cmp.b	#5,cmd(a2)
	beq	.tonep

	move	d0,rPeriod(a2)
	move	d0,period(a2)
	clr.l	mFPos(a4)

	move.l	a1,a3
	tst.b	mLoop(a4)
	beq.b	.nloop

	addq.l	#4,a3
	tlword	(a3)+,d0
	tlword	(a3)+,d1
	add.l	d1,d0
	cmp.l	#2,d0
	bgt.b	.look
	subq.l	#8,a3

.nloop	tlword	(a3)+,d0			; sample length

.look	moveq	#0,d1
	cmp.b	#9,cmd(a2)
	bne.b	.nooffset

	move.b	info(a2),d1
	bne.b	.ok3
	move.b	lastOffset(a2),d1
.ok3	move.b	d1,lastOffset(a2)
	lsl	#8,d1
	add.l	d1,a6
	sub.l	d1,d0
	bpl.b	.nooffset
	st	mOnOff(a4)
	bra.b	.skip

.nooffset
	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8_2
	lsr.l	#1,d0
	lsr.l	#1,d1
	sub.l	d1,a6
.bit8_2	move.l	a6,(a4)				; sample start
	move.l	d0,mLength(a4)
	clr.b	mOnOff(a4)
	bra.b	.skip

.keyoff	tst.b	volEnvOn(a2)
	beq.b	.vol0

	clr.b	volSustained(a2)
	st	fading(a2)
	st	keyoff(a2)
	bra.b	.skip

.vol0	tst.b	inst(a2)
	bne.b	.skip
	clr	volume(a2)
	bra.b	.skip

.tonep	move	d0,toperiod(a2)

.skip	moveq	#0,d0
	move.b	vol(a2),d0
	cmp.b	#$10,d0
	blo.b	.eivol

	cmp.b	#$50,d0
	bhi.b	.volcmd

	sub	#$10,d0
	move	d0,volume(a2)
	bra.b	.eivol

.volcmd	cmp.b	#$60,d0
	blo.b	.eivol

	lea	xm_vct(pc),a1
	moveq	#$f,d1
	and	d0,d1
	move	d1,d2
	lsr	#4,d0
	subq	#6,d0
	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.eivol	lea	xm_ct(pc),a1
	moveq	#0,d0
	move.b	cmd(a2),d0
	cmp.b	#$20,d0
	bhs.b	.skipa
	moveq	#0,d1
	move.b	info(a2),d1
	beq.b	.zero
	move.b	d1,lastcmd(a2)

.zero	moveq	#0,d2
	move.b	lastcmd(a2),d2

;	ifne	debug
	move.l	kalas(a5),a3
	st	(a3,d0)
;	endc

	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.skipa	move	volume(a2),rVolume(a2)
	move	period(a2),rPeriod(a2)

	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.channelloop

	tst	pdelaycnt(a5)
	beq.b	.oke

	subq	#1,pdelaycnt(a5)
	bra	xm_exit

.oke	addq	#1,rows(a5)		; incr. linecounter

	tst	pbrkflag(a5)		; do we have to break this pattern?
	beq.b	.nobrk
	move	pbrkrow(a5),rows(a5)
	clr	pbrkrow(a5)		; clr break position
.nobrk
	move	rows(a5),d0
	cmp	plen(a5),d0
	bcc.b	.newpos				; worst case opt: branch not taken

	tst	pjmpflag(a5)		; do we have to jump?
	bne.b	.newpos

	tst.w	pbrkflag(a5)
	beq	xm_dee
	moveq	#0,d2
	bra.b	conti
.newpos
	move	pbrkrow(a5),rows(a5)
	clr	pbrkrow(a5)		; clr break position
burki
	moveq	#0,d2
burkii
	addq.w	#1,pos(a5)			; inc songposition
	move.w	slen(a5),d0
	cmp.w	pos(a5),d0			; are we thru with all patterns?
	bhi.b	conti				; nope


	move.l	#-1,uade_restart_cmd

	move.w	xmRestart(a0),d0
	iword	d0
	tst.l	d2
	beq.b	.okay
	moveq	#0,d0
.okay	move.w	d0,pos(a5)
	st	PS3M_break(a5)


	moveq	#1,d2

conti	move	pos(a5),d0
	move	d0,PS3M_position(a5)
	st	PS3M_poscha(a5)

	moveq	#0,d1
	move.b	xmOrders(a0,d0),d1	
	cmp.b	xmNumPatts(a0),d1
	bhs.b	burkii
	lsl	#2,d1
	lea	xm_patts,a1
	move.l	(a1,d1),a1
	lea	xmNumRows(a1),a3
	tword	(a3)+,d0
	move	d0,plen(a5)
	move.l	a1,a3
	tlword	(a3)+,d0
	add.l	d0,a1

	clr	pjmpflag(a5)
	clr	pbrkflag(a5)

	move.w	rows(a5),d1
	beq.b	.setp
	subq.w	#1,d1
.loop	move.w	numchans(a5),d7
	subq.w	#1,d7
.llop	move.b	(a1)+,d0
	bpl.b	.tout
	btst	#0,d0
	beq.b	.nono
	addq.w	#1,a1
.nono	btst	#1,d0
	beq.b	.noin
	addq.w	#1,a1
.noin	btst	#2,d0
	beq.b	.novo
	addq.w	#1,a1
.novo	btst	#3,d0
	beq.b	.nocm
	addq.w	#1,a1
.nocm	btst	#4,d0
	beq.b	.cont
	addq.w	#1,a1
	bra.b	.cont
.tout	addq.w	#4,a1
.cont	dbf	d7,.llop
	dbf	d1,.loop
.setp	move.l	a1,ppos(a5)

xm_dee	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	subq	#1,d7

.luu	tst	volume(a2)
	bne.b	.noaging

	cmp.b	#8,age(a2)
	bhs.b	.stop
	addq.b	#1,age(a2)
	bra.b	.nextt
.stop	st	mOnOff(a4)
	bra.b	.nextt
.noaging
	clr.b	age(a2)

.nextt	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.luu
xm_exit	rts

xm_ret	rts


; COMMANDS!


; Returns zero-flag set, if no samples in this instrument
; Needs pattern note number in d0 and instrument in a1

xm_getInst
	moveq	#0,d6
	move.b	xmSmpNoteNums-1(a1,d0),d6	; sample number
	lea	xmNumSamples(a1),a3
	tword	(a3)+,d2
	tst	d2
	beq	.nosample
	lea	xmSmpHdrSize(a1),a3
	tlword	(a3)+,d3
	move.l	a1,a3				; InstHdrSize
	tlword	(a3)+,d1
	add.l	d1,a1				; Now at the first sample!

	move.l	d3,d4
	mulu	d2,d4
	lea	(a1,d4),a6

	tst	d6
	beq.b	.rightsample

.skiploop
	lea	xmSmpLength(a1),a3
	tlword	(a3)+,d4
	add.l	d4,a6
	add.l	d3,a1
	subq	#1,d6
	bne.b	.skiploop

.rightsample
	moveq	#1,d6
	rts

.nosample
	st	mOnOff(a4)
	moveq	#0,d6
	rts


; Needs instrument in a1

xm_getPeriod
	move.b	xmRelNote(a1),d1
	ext	d1
	add	d1,d0
	bpl.b	.ok
	moveq	#0,d0
.ok	cmp	#118,d0
	bls.b	.ok2
	moveq	#118,d0
.ok2	move.b	xmFinetune(a1),d1
	ext.l	d0
	ext	d1

	btst	#0,sflags+1(a5)
	beq.b	.amigafreq

	move	#121*64,d2
	lsl	#6,d0
	sub	d0,d2
	asr	d1
	sub	d1,d2
	move	d2,d0
	rts

.amigafreq
	divu	#12,d0
	swap	d0
	move	d0,d2				; note
	clr	d0
	swap	d0				; octave
	lsl	#3,d2

	move	d1,d3
	asr	#4,d3
	move	d2,d4
	add	d3,d4

	add	d4,d4
	lea	xm_periods(pc),a3
	moveq	#0,d5
	move	(a3,d4),d5

	tst	d1
	bpl.b	.k
	subq	#1,d3
	neg	d1
	bra.b	.k2
.k	addq	#1,d3
.k2	move	d2,d4
	add	d3,d4
	add	d4,d4
	moveq	#0,d6
	move	(a3,d4),d6

	and	#$f,d1
	mulu	d1,d6
	move	#16,d3
	sub	d1,d3
	mulu	d3,d5
	add.l	d6,d5

	subq	#1,d0
	bmi.b	.f2
	lsr.l	d0,d5
	bra.b	.d

.f2	add.l	d5,d5
.d	move	d5,d0
	rts


; Command 0 - Arpeggio

xm_arpeggio
	tst.b	info(a2)
	beq.b	.skip

	moveq	#0,d0
	move.b	note(a2),d0
	beq.b	.skip

	move.l	sample(a2),d2
	beq.b	.skip
	move.l	d2,a1

	bsr	xm_getInst
	beq.b	.skip

	moveq	#0,d2
	move	cn(a5),d2
	divu	#3,d2
	swap	d2
	tst	d2
	beq.b	.f
	subq	#1,d2
	beq.b	.1

.2	moveq	#$f,d2
	and.b	lastcmd(a2),d2
	add	d2,d0
	bra.b	.f

.1	move.b	lastcmd(a2),d2
	lsr.b	#4,d2
	add.b	d2,d0

.f	bsr	xm_getPeriod
	move	d0,mPeriod(a4)
.skip	rts



; Command 1 - Portamento up
; Also command E1 - fine portamento up
; and command X1 - extra fine portamento up

xm_slideup
	lsl	#2,d2
xm_xslideup
	sub	d2,period(a2)
	bra.b	xm_checklimits


; Command 2 - Portamento down
; Also command E2 - fine portamento down
; and command X2 - extra fine portamento down

xm_slidedwn
	lsl	#2,d2
xm_xslidedwn
	add	d2,period(a2)

xm_checklimits
	move	period(a2),d0
	btst	#0,sflags+1(a5)
	beq.b	.amiga

	cmp	#2*64,d0
	bhs.b	.ok
	move	#2*64,d0
.ok	cmp	#121*64,d0
	bls.b	.dd2
	move	#121*64,d0
	bra.b	.dd2

.amiga	cmp	#$7fff,d0
	bls.b	.dd
	move	#$7fff,d0
.dd	cmp	#64,d0
	bhs.b	.dd2
	move	#64,d0
.dd2	move	d0,period(a2)
	move	d0,rPeriod(a2)
	rts


; Command 3 - Tone portamento

xm_tonep
	tst	d1
	beq.b	xm_tonepnoch
	move.b	d1,notepspd(a2)
xm_tonepnoch
	move	toperiod(a2),d0
	beq.b	.1
	moveq	#0,d1
	move.b	notepspd(a2),d1
	lsl	#2,d1

	cmp	period(a2),d0
	blt.b	.topoup

	add	d1,period(a2)
	cmp	period(a2),d0
	bhi.b	.1
	move	d0,period(a2)
	clr	toperiod(a2)
.1	move	period(a2),rPeriod(a2)
	rts

.topoup	sub	d1,period(a2)
	cmp	period(a2),d0
	blt.b	.dd
	move	d0,period(a2)
	clr	toperiod(a2)
.dd	move	period(a2),rPeriod(a2)
	rts


; Command 4 - Vibrato

xm_svibspd
	move.b	vibcmd(a2),d2
	moveq	#$f,d0
	and	d1,d0
	beq.b	.skip
	and	#$f0,d2
	or	d0,d2
.skip	move.b	d2,vibcmd(a2)
	rts

xm_vibrato
	move.b	vibcmd(a2),d2
	move	d1,d0
	and	#$f0,d0
	beq.b	.vib2

	and	#$f,d2
	or	d0,d2

.vib2	moveq	#$f,d0
	and	d1,d0
	beq.b	.vibskip2

	and	#$f0,d2
	or	d0,d2
.vibskip2
	move.b	d2,vibcmd(a2)

xm_vibrato2
	moveq	#$1f,d0
	and.b	vibpos(a2),d0
	moveq	#0,d2
	lea	mt_vibratotable(pc),a3
	move.b	(a3,d0),d2
	moveq	#$f,d0
	and.b	vibcmd(a2),d0
	mulu	d0,d2
	lsr	#5,d2

	move	period(a2),d0
	btst	#5,vibpos(a2)
	bne.b	.neg
	add	d2,d0
	bra.b	.vib3
.neg
	sub	d2,d0
.vib3
	move	d0,mPeriod(a4)
	move.b	vibcmd(a2),d0
	lsr.b	#4,d0
	add.b	d0,vibpos(a2)
	rts

; Command 5 - Tone portamento and volume slide

xm_tpvsl
	bsr	xm_tonepnoch
	bra.b	xm_vslide

; Command 6 - Vibrato and volume slide

xm_vibvsl
	move	d2,-(sp)
	bsr.b	xm_vibrato2
	move	(sp)+,d2
	bra.b	xm_vslide


; Command 7 - Tremolo

xm_tremolo
	move.b	vibcmd(a2),d2
	move	d1,d0
	and	#$f0,d0
	beq.b	.vib2

	and	#$f,d2
	or	d0,d2

.vib2	moveq	#$f,d0
	and	d1,d0
	beq.b	.vibskip2

	and	#$f0,d2
	or	d0,d2
.vibskip2
	move.b	d2,vibcmd(a2)

	moveq	#$1f,d0
	and.b	vibpos(a2),d0
	moveq	#0,d2
	lea	mt_vibratotable(pc),a3
	move.b	(a3,d0),d2
	moveq	#$f,d0
	and.b	vibcmd(a2),d0
	mulu	d0,d2
	lsr	#6,d2

	move	volume(a2),d0
	btst	#5,vibpos(a2)
	bne.b	.neg
	add	d2,d0
	bra.b	.vib3
.neg
	sub	d2,d0
.vib3	move	d0,mVolume(a4)
	move.b	vibcmd(a2),d0
	lsr.b	#4,d0
	add.b	d0,vibpos(a2)
	rts


; Command A - Volume slide
; Also commands EA and EB, fine volume slides

xm_vslide
	lsr	#4,d2
	beq.b	xm_vslidedown
xm_vslideup
	add	d2,volume(a2)
	cmp	#64,volume(a2)
	bls.b	xm_vsskip
	move	#64,volume(a2)
xm_vsskip
	move	volume(a2),rVolume(a2)
	rts

xm_vslidedown
	moveq	#$f,d2
	and.b	lastcmd(a2),d2
xm_vslidedown2
	sub	d2,volume(a2)
	bpl.b	xm_vsskip
	clr	volume(a2)
	clr	rVolume(a2)
	rts


; Command B - Pattern jump

xm_pjmp	cmp.b	pos(a5),d1
	bhi.b	.e
	st	PS3M_break
.e	subq	#1,d1
	move	d1,pos(a5)
	clr	pbrkrow(a5)
	st	pjmpflag(a5)
	st	PS3M_poscha(a5)
	rts


; Command C - Set volume

xm_setvol
	cmp	#64,d1
	bls.b	.ok
	moveq	#64,d1
.ok	move	d1,volume(a2)
	rts


; Command D - Pattern break

xm_pbrk	st	pjmpflag(a5)
	moveq	#$f,d2
	and.l	d1,d2
	lsr.l	#4,d1
	mulu	#10,d1
	add	d2,d1
	move	d1,pbrkrow(a5)
	st	PS3M_poscha(a5)
	rts


; Command E - Extended commands

xm_ecmds
	lea	xm_ect(pc),a1
xm_ee	move	d1,d0
	moveq	#$f,d1
	and	d0,d1
	move	d1,d2
	lsr	#4,d0

;	ifne	debug
	move.l	kalas(a5),a3
	st	$40(a3,d0)
;	endc

	add	d0,d0
	move	(a1,d0),d0
	jmp	(a1,d0)

xm_cecmds
	lea	xm_cect(pc),a1
	bra.b	xm_ee


; Command E6 - Pattern loop

xm_pattloop
	tst	d1
	beq.b	.setlp				; 0 means "set loop mark" in current line
	tst.w	loopcnt(a2)			; dont allow nesting, accept value
	beq.b	.jcnt				; only if we counted down the last loop
	subq.w	#1,loopcnt(a2)			; count down
	bne.b	.jloop				; jump again if still not zero
	rts
.jcnt	move.w	d1,loopcnt(a2)			; accept new loop value
.jloop	move.w	looprow(a2),pbrkrow(a5)	; put line number to jump to
	st	pbrkflag(a5)
	rts
.setlp	move.w	rows(a5),looprow(a2)
	rts


; Command E9 - Retrig note

xm_retrig
	subq.b	#1,retrigcn(a2)
	bne	xm_eret

	move.l	sample(a2),d2
	beq	xm_eret
	move.l	d2,a1

	move	d1,-(sp)

	moveq	#0,d0
	move.b	note(a2),d0
	beq	xm_sretrig

	bsr	xm_getInst
	beq	.skip

	clr.l	mFPos(a4)
; Handle envelopes
	move	#$ffff,fadeOut(a2)
	clr.b	fading(a2)
	clr.b	keyoff(a2)

	move.l	sample(a2),a3
	btst	#xmEnvOn,xmVolType(a3)
	beq.b	.voloff

	clr	volEnvX(a2)
	st	volEnvOn(a2)
	clr.b	volSustained(a2)
	bra.b	.jep

.voloff	clr.b	volEnvOn(a2)


.jep	btst	#xmEnvOn,xmPanType(a3)
	beq.b	.panoff

	st	panEnvOn(a2)
	clr.b	panSustained(a2)
	bra.b	.jep2

.panoff	clr.b	panEnvOn(a2)

.jep2	move.l	a1,a3
	tst.b	mLoop(a4)
	beq.b	.nloop

	addq.l	#4,a3
	tlword	(a3)+,d0
	tlword	(a3)+,d1
	add.l	d1,d0
	cmp.l	#2,d0
	bgt.b	.look
	subq.l	#8,a3

.nloop	tlword	(a3)+,d0			; sample length

.look	move.l	a6,(a4)				; sample start
	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8_2
	lsr.l	#1,d0
.bit8_2	move.l	d0,mLength(a4)
	clr.b	mOnOff(a4)
.skip	move	(sp)+,d1
xm_sretrig
	move.b	d1,retrigcn(a2)
	rts


; Command EC - Note cut

xm_ncut	cmp	cn(a5),d1
	bne.b	xm_eret
	clr	volume(a2)
	clr	rVolume(a2)
xm_eret	rts


; Command ED - Delay note

xm_ndelay
	cmp	cn(a5),d1
	bne.b	xm_eret

	tst	(a2)
	beq	.skip

	moveq	#0,d0
	move.b	(a2),d0
	beq	.skip

	cmp.b	#97,d0				; Key off -note
	beq	.keyoff

	move.b	d0,note(a2)

	moveq	#0,d1
	move.b	inst(a2),d1
	beq.b	.esmp

	cmp	inss,d1
	bgt.b	.esmp

	lsl	#2,d1
	lea	xm_insts,a1
	move.l	-4(a1,d1),a1

	move.l	a1,sample(a2)
	bra.b	.ju
.esmp	move.l	sample(a2),d2
	beq	.skip
	move.l	d2,a1

.ju	bsr	xm_getInst
	beq	.skip

	tst.b	inst(a2)
	beq.b	.smpok

	lea	xmLoopStart(a1),a3
	tlword	(a3)+,d1
	lea	xmLoopLength(a1),a3
	tlword	(a3)+,d2

	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8
	lsr.l	#1,d1
	lsr.l	#1,d2
.bit8	add.l	a6,d1

	move.l	d1,mLStart(a4)
	move.l	d2,mLLength(a4)
	cmp.l	#2,d2
	bhi.b	.ok

	clr.b	mLoop(a4)
	st.b	mOnOff(a4)
	bra.b	.huu

.ok	moveq	#xmLoopType,d1
	and.b	xmSmpFlags(a1),d1
	sne	mLoop(a4)

.huu	move.b	xmVolume(a1),volume+1(a2)
	cmp	#64,volume(a2)
	bls.b	.e
	move	#64,volume(a2)
.e	move	volume(a2),rVolume(a2)

.smpok	bsr	xm_getPeriod

	move	d0,rPeriod(a2)
	move	d0,period(a2)
	clr.l	mFPos(a4)

; Handle envelopes
	move	#$ffff,fadeOut(a2)
	clr.b	fading(a2)
	clr.b	keyoff(a2)

	move.l	sample(a2),a3

	btst	#xmEnvOn,xmVolType(a3)
	beq.b	.voloff

	clr	volEnvX(a2)
	st	volEnvOn(a2)
	clr.b	volSustained(a2)
	bra.b	.jep

.voloff	clr.b	volEnvOn(a2)


.jep	btst	#xmEnvOn,xmPanType(a3)
	beq.b	.panoff

	st	panEnvOn(a2)
	clr.b	panSustained(a2)
	bra.b	.jep2

.panoff	clr.b	panEnvOn(a2)

.jep2	move.l	a1,a3
	tst.b	mLoop(a4)
	beq.b	.nloop

	addq.l	#4,a3
	tlword	(a3)+,d0
	tlword	(a3)+,d1
	add.l	d1,d0
	cmp.l	#2,d0
	bgt.b	.look
	subq.l	#8,a3

.nloop	tlword	(a3)+,d0			; sample length

.look	move.l	a6,(a4)				; sample start
	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8_2
	lsr.l	#1,d0
.bit8_2	move.l	d0,mLength(a4)
	clr.b	mOnOff(a4)
	bra.b	.skip

.keyoff	tst.b	volEnvOn(a2)
	beq.b	.vol0

	clr.b	volSustained(a2)
	st	fading(a2)
	st	keyoff(a2)
	bra.b	.skip

.vol0	tst.b	inst(a2)
	bne.b	.skip
	clr	volume(a2)
.skip
.ret	rts


; Command EE - Pattern delay

xm_pdelay
	tst	pdelaycnt(a5)
	bne.b	.skip

	move	d1,pdelaycnt(a5)

.skip	rts


; Command F - Set speed

xm_spd	cmp	#$20,d1
	bhs.b	.bpm

	tst	d1
	bne.b	.g
	st	PS3M_break(a5)
	st	PS3M_poscha(a5)
	rts

.g	move	d1,spd(a5)
	rts

.bpm	move	d1,tempo(a5)
	move.l	mrate(a5),d0
	move.l	d0,d2
	lsl.l	#2,d0
	add.l	d2,d0
	add	d1,d1
	divu	d1,d0

	addq	#1,d0
	and	#~1,d0
	move	d0,bytesperframe(a5)
	rts


; Command G - Set global volume

xm_sgvol
	cmp	#64,d1
	bls.b	.ok
	moveq	#64,d1
.ok	move	d1,globalVol(a5)
	rts


; Command H - Global volume slide

xm_gvslide
	lsr	#4,d2
	beq.b	.down
	add	d2,globalVol(a5)
	cmp	#64,globalVol(a5)
	bls.b	.x
	move	#64,globalVol(a5)
.x	rts

.down	moveq	#$f,d2
	and.b	lastcmd(a2),d2
	sub	d2,globalVol(a5)
	bpl.b	.x
	clr	globalVol(a5)
	rts


; Command K - Key off

xm_keyoff
	clr.b	volSustained(a2)
	st	fading(a2)
	st	keyoff(a2)
	rts


; Command L - Set envelope position

xm_setenvpos
	tst.b	volEnvOn(a2)
	beq.b	.skip

	move.l	sample(a2),d2
	beq.b	.skip
	move.l	d2,a3

	btst	#xmEnvOn,xmVolType(a1)
	beq.b	.skip

	move	d1,volEnvX(a2)
	st	volEnvOn(a2)
	clr.b	volSustained(a2)
.skip	rts



; Command R - Multi retrig note

xm_rretrig
	subq.b	#1,retrigcn(a2)
	bne	xm_eret

	move.l	sample(a2),d2
	beq	xm_eret
	move.l	d2,a1

	move	d1,-(sp)

	moveq	#0,d0
	move.b	note(a2),d0
	beq	xm_srretrig

	bsr	xm_getInst
	beq.b	.skip

	clr.l	mFPos(a4)
; Handle envelopes
	move	#$ffff,fadeOut(a2)
	clr.b	fading(a2)
	clr.b	keyoff(a2)

	move.l	sample(a2),a3
	btst	#xmEnvOn,xmVolType(a3)
	beq.b	.voloff

	clr	volEnvX(a2)
	st	volEnvOn(a2)
	clr.b	volSustained(a2)
	bra.b	.jep

.voloff	clr.b	volEnvOn(a2)


.jep	btst	#xmEnvOn,xmPanType(a3)
	beq.b	.panoff

	st	panEnvOn(a2)
	clr.b	panSustained(a2)
	bra.b	.jep2

.panoff	clr.b	panEnvOn(a2)

.jep2	move.l	a1,a3
	tlword	(a3)+,d0			; sample length

	move.l	a6,(a4)				; sample start
	btst	#xm16bitf,xmSmpFlags(a1)
	beq.b	.bit8_2
	lsr.l	#1,d0
.bit8_2	move.l	d0,mLength(a4)
	clr.b	mOnOff(a4)

.skip	moveq	#0,d0
	move.b	lastcmd(a2),d0
	lsr	#4,d0
	lea	ftab2(pc),a3
	moveq	#0,d2
	move.b	(a3,d0),d2
	beq.b	.ddq

	mulu	volume(a2),d2
	lsr	#4,d2
	move	d2,volume(a2)
	bra.b	.ddw

.ddq	lea	ftab1(pc),a3
	move.b	(a3,d0),d2
	ext	d2
	add	d2,volume(a2)

.ddw	tst	volume(a2)
	bpl.b	.ei0
	clr	volume(a2)
.ei0	cmp	#64,volume(a2)
	bls.b	.ei64
	move	#64,volume(a2)
.ei64	move	volume(a2),mVolume(a4)
	move	(sp)+,d1
xm_srretrig
	and	#$f,d1
	move.b	d1,retrigcn(a2)
	rts


; Command T - Tremor

xm_tremor
	rts


; Command X - Extra fine slides

xm_xfinesld
	move.b	d2,d1
	and	#$f,d2
	cmp.b	#$10,d1
	blo.b	.q
	cmp.b	#$20,d1
	blo	xm_xslideup
	cmp.b	#$30,d1
	blo	xm_xslidedwn
.q	rts

	dc	960,954,948,940,934,926,920,914
xm_periods
	dc	907,900,894,887,881,875,868,862,856,850,844,838,832,826,820,814
	dc	808,802,796,791,785,779,774,768,762,757,752,746,741,736,730,725
	dc	720,715,709,704,699,694,689,684,678,675,670,665,660,655,651,646
	dc	640,636,632,628,623,619,614,610,604,601,597,592,588,584,580,575
	dc	570,567,563,559,555,551,547,543,538,535,532,528,524,520,516,513
	dc	508,505,502,498,494,491,487,484,480,477,474,470,467,463,460,457

xm_linFreq
	dc.l	535232,534749,534266,533784,533303,532822,532341,531861
	dc.l	531381,530902,530423,529944,529466,528988,528511,528034
	dc.l	527558,527082,526607,526131,525657,525183,524709,524236
	dc.l	523763,523290,522818,522346,521875,521404,520934,520464
	dc.l	519994,519525,519057,518588,518121,517653,517186,516720
	dc.l	516253,515788,515322,514858,514393,513929,513465,513002
	dc.l	512539,512077,511615,511154,510692,510232,509771,509312
	dc.l	508852,508393,507934,507476,507018,506561,506104,505647
	dc.l	505191,504735,504280,503825,503371,502917,502463,502010
	dc.l	501557,501104,500652,500201,499749,499298,498848,498398
	dc.l	497948,497499,497050,496602,496154,495706,495259,494812
	dc.l	494366,493920,493474,493029,492585,492140,491696,491253
	dc.l	490809,490367,489924,489482,489041,488600,488159,487718
	dc.l	487278,486839,486400,485961,485522,485084,484647,484210
	dc.l	483773,483336,482900,482465,482029,481595,481160,480726
	dc.l	480292,479859,479426,478994,478562,478130,477699,477268
	dc.l	476837,476407,475977,475548,475119,474690,474262,473834
	dc.l	473407,472979,472553,472126,471701,471275,470850,470425
	dc.l	470001,469577,469153,468730,468307,467884,467462,467041
	dc.l	466619,466198,465778,465358,464938,464518,464099,463681
	dc.l	463262,462844,462427,462010,461593,461177,460760,460345
	dc.l	459930,459515,459100,458686,458272,457859,457446,457033
	dc.l	456621,456209,455797,455386,454975,454565,454155,453745
	dc.l	453336,452927,452518,452110,451702,451294,450887,450481
	dc.l	450074,449668,449262,448857,448452,448048,447644,447240
	dc.l	446836,446433,446030,445628,445226,444824,444423,444022
	dc.l	443622,443221,442821,442422,442023,441624,441226,440828
	dc.l	440430,440033,439636,439239,438843,438447,438051,437656
	dc.l	437261,436867,436473,436079,435686,435293,434900,434508
	dc.l	434116,433724,433333,432942,432551,432161,431771,431382
	dc.l	430992,430604,430215,429827,429439,429052,428665,428278
	dc.l	427892,427506,427120,426735,426350,425965,425581,425197
	dc.l	424813,424430,424047,423665,423283,422901,422519,422138
	dc.l	421757,421377,420997,420617,420237,419858,419479,419101
	dc.l	418723,418345,417968,417591,417214,416838,416462,416086
	dc.l	415711,415336,414961,414586,414212,413839,413465,413092
	dc.l	412720,412347,411975,411604,411232,410862,410491,410121
	dc.l	409751,409381,409012,408643,408274,407906,407538,407170
	dc.l	406803,406436,406069,405703,405337,404971,404606,404241
	dc.l	403876,403512,403148,402784,402421,402058,401695,401333
	dc.l	400970,400609,400247,399886,399525,399165,398805,398445
	dc.l	398086,397727,397368,397009,396651,396293,395936,395579
	dc.l	395222,394865,394509,394153,393798,393442,393087,392733
	dc.l	392378,392024,391671,391317,390964,390612,390259,389907
	dc.l	389556,389204,388853,388502,388152,387802,387452,387102
	dc.l	386753,386404,386056,385707,385359,385012,384664,384317
	dc.l	383971,383624,383278,382932,382587,382242,381897,381552
	dc.l	381208,380864,380521,380177,379834,379492,379149,378807
	dc.l	378466,378124,377783,377442,377102,376762,376422,376082
	dc.l	375743,375404,375065,374727,374389,374051,373714,373377
	dc.l	373040,372703,372367,372031,371695,371360,371025,370690
	dc.l	370356,370022,369688,369355,369021,368688,368356,368023
	dc.l	367691,367360,367028,366697,366366,366036,365706,365376
	dc.l	365046,364717,364388,364059,363731,363403,363075,362747
	dc.l	362420,362093,361766,361440,361114,360788,360463,360137
	dc.l	359813,359488,359164,358840,358516,358193,357869,357547
	dc.l	357224,356902,356580,356258,355937,355616,355295,354974
	dc.l	354654,354334,354014,353695,353376,353057,352739,352420
	dc.l	352103,351785,351468,351150,350834,350517,350201,349885
	dc.l	349569,349254,348939,348624,348310,347995,347682,347368
	dc.l	347055,346741,346429,346116,345804,345492,345180,344869
	dc.l	344558,344247,343936,343626,343316,343006,342697,342388
	dc.l	342079,341770,341462,341154,340846,340539,340231,339924
	dc.l	339618,339311,339005,338700,338394,338089,337784,337479
	dc.l	337175,336870,336566,336263,335959,335656,335354,335051
	dc.l	334749,334447,334145,333844,333542,333242,332941,332641
	dc.l	332341,332041,331741,331442,331143,330844,330546,330247
	dc.l	329950,329652,329355,329057,328761,328464,328168,327872
	dc.l	327576,327280,326985,326690,326395,326101,325807,325513
	dc.l	325219,324926,324633,324340,324047,323755,323463,323171
	dc.l	322879,322588,322297,322006,321716,321426,321136,320846
	dc.l	320557,320267,319978,319690,319401,319113,318825,318538
	dc.l	318250,317963,317676,317390,317103,316817,316532,316246
	dc.l	315961,315676,315391,315106,314822,314538,314254,313971
	dc.l	313688,313405,313122,312839,312557,312275,311994,311712
	dc.l	311431,311150,310869,310589,310309,310029,309749,309470
	dc.l	309190,308911,308633,308354,308076,307798,307521,307243
	dc.l	306966,306689,306412,306136,305860,305584,305308,305033
	dc.l	304758,304483,304208,303934,303659,303385,303112,302838
	dc.l	302565,302292,302019,301747,301475,301203,300931,300660
	dc.l	300388,300117,299847,299576,299306,299036,298766,298497
	dc.l	298227,297958,297689,297421,297153,296884,296617,296349
	dc.l	296082,295815,295548,295281,295015,294749,294483,294217
	dc.l	293952,293686,293421,293157,292892,292628,292364,292100
	dc.l	291837,291574,291311,291048,290785,290523,290261,289999
	dc.l	289737,289476,289215,288954,288693,288433,288173,287913
	dc.l	287653,287393,287134,286875,286616,286358,286099,285841
	dc.l	285583,285326,285068,284811,284554,284298,284041,283785
	dc.l	283529,283273,283017,282762,282507,282252,281998,281743
	dc.l	281489,281235,280981,280728,280475,280222,279969,279716
	dc.l	279464,279212,278960,278708,278457,278206,277955,277704
	dc.l	277453,277203,276953,276703,276453,276204,275955,275706
	dc.l	275457,275209,274960,274712,274465,274217,273970,273722
	dc.l	273476,273229,272982,272736,272490,272244,271999,271753
	dc.l	271508,271263,271018,270774,270530,270286,270042,269798
	dc.l	269555,269312,269069,268826,268583,268341,268099,267857

xm_ct	dc	xm_arpeggio-xm_ct	;0
	dc	xm_ret-xm_ct		;1
	dc	xm_ret-xm_ct		;2
	dc	xm_ret-xm_ct		;3
	dc	xm_ret-xm_ct		;4
 	dc	xm_ret-xm_ct		;5
	dc	xm_ret-xm_ct		;6
	dc	xm_ret-xm_ct		;7
	dc	xm_ret-xm_ct		;8
	dc	xm_ret-xm_ct		;9
	dc	xm_ret-xm_ct		;A
 	dc	xm_pjmp-xm_ct		;B
 	dc	xm_setvol-xm_ct		;C
 	dc	xm_pbrk-xm_ct		;D
 	dc	xm_ecmds-xm_ct		;E
 	dc	xm_spd-xm_ct		;F
	dc	xm_sgvol-xm_ct		;G
 	dc	xm_ret-xm_ct		;H
 	dc	xm_ret-xm_ct		;I
 	dc	xm_ret-xm_ct		;J
 	dc	xm_keyoff-xm_ct		;K
 	dc	xm_setenvpos-xm_ct	;L
 	dc	xm_ret-xm_ct		;M
 	dc	xm_ret-xm_ct		;N
 	dc	xm_ret-xm_ct		;O
 	dc	xm_ret-xm_ct		;P
 	dc	xm_ret-xm_ct		;Q
 	dc	xm_srretrig-xm_ct	;R
 	dc	xm_ret-xm_ct		;S
 	dc	xm_tremor-xm_ct		;T
 	dc	xm_ret-xm_ct		;U
 	dc	xm_ret-xm_ct		;V
 	dc	xm_ret-xm_ct		;W
 	dc	xm_xfinesld-xm_ct	;X
 	dc	xm_ret-xm_ct		;Y
 	dc	xm_ret-xm_ct		;Z

xm_cct	dc	xm_arpeggio-xm_cct	;0
	dc	xm_slideup-xm_cct	;1
	dc	xm_slidedwn-xm_cct	;2
	dc	xm_tonep-xm_cct		;3
	dc	xm_vibrato-xm_cct	;4
	dc	xm_tpvsl-xm_cct		;5
	dc	xm_vibvsl-xm_cct	;6
	dc	xm_tremolo-xm_cct	;7
	dc	xm_ret-xm_cct		;8
	dc	xm_ret-xm_cct		;9
	dc	xm_vslide-xm_cct	;A
 	dc	xm_ret-xm_cct		;B
 	dc	xm_ret-xm_cct		;C
 	dc	xm_ret-xm_cct		;D
 	dc	xm_cecmds-xm_cct	;E
 	dc	xm_ret-xm_cct		;F
 	dc	xm_ret-xm_cct		;G
	dc	xm_gvslide-xm_cct	;H
 	dc	xm_ret-xm_cct		;I
 	dc	xm_ret-xm_cct		;J
 	dc	xm_ret-xm_cct		;K
 	dc	xm_ret-xm_cct		;L
 	dc	xm_ret-xm_cct		;M
 	dc	xm_ret-xm_cct		;N
 	dc	xm_ret-xm_cct		;O
 	dc	xm_ret-xm_cct		;P
 	dc	xm_ret-xm_cct		;Q
 	dc	xm_rretrig-xm_cct	;R
 	dc	xm_ret-xm_cct		;S
 	dc	xm_tremor-xm_cct	;T
 	dc	xm_ret-xm_cct		;U
 	dc	xm_ret-xm_cct		;V
 	dc	xm_ret-xm_cct		;W
 	dc	xm_ret-xm_cct		;X
 	dc	xm_ret-xm_cct		;Y
 	dc	xm_ret-xm_cct		;Z

xm_ect	dc	xm_ret-xm_ect		;0
	dc	xm_slideup-xm_ect	;1
	dc	xm_slidedwn-xm_ect	;2
	dc	xm_ret-xm_ect		;3
	dc	xm_ret-xm_ect		;4
	dc	xm_ret-xm_ect		;5
	dc	xm_pattloop-xm_ect	;6
	dc	xm_ret-xm_ect		;7
	dc	xm_ret-xm_ect		;8
	dc	xm_sretrig-xm_ect	;9
	dc	xm_vslideup-xm_ect	;A
 	dc	xm_vslidedown2-xm_ect	;B
 	dc	xm_ncut-xm_ect		;C
 	dc	xm_ret-xm_ect		;D
 	dc	xm_pdelay-xm_ect	;E
 	dc	xm_ret-xm_ect		;F

xm_cect	dc	xm_ret-xm_cect		;0
	dc	xm_ret-xm_cect		;1
	dc	xm_ret-xm_cect		;2
	dc	xm_ret-xm_cect		;3
	dc	xm_ret-xm_cect		;4
	dc	xm_ret-xm_cect		;5
	dc	xm_ret-xm_cect		;6
	dc	xm_ret-xm_cect		;7
	dc	xm_ret-xm_cect		;8
	dc	xm_retrig-xm_cect	;9
	dc	xm_ret-xm_cect		;A
 	dc	xm_ret-xm_cect		;B
 	dc	xm_ncut-xm_cect		;C
 	dc	xm_ndelay-xm_cect	;D
 	dc	xm_ret-xm_cect		;E
 	dc	xm_ret-xm_cect		;F

xm_vct	dc	xm_ret-xm_vct		;6
	dc	xm_ret-xm_vct		;7
	dc	xm_vslidedown2-xm_vct	;8
	dc	xm_vslideup-xm_vct	;9
	dc	xm_svibspd-xm_vct	;A
 	dc	xm_ret-xm_vct		;B
 	dc	xm_ret-xm_vct		;C
 	dc	xm_ret-xm_vct		;D
 	dc	xm_ret-xm_vct		;E
 	dc	xm_ret-xm_vct		;F

xm_cvct	dc	xm_vslidedown2-xm_cvct	;6
	dc	xm_vslideup-xm_cvct	;7
	dc	xm_ret-xm_cvct		;8
	dc	xm_ret-xm_cvct		;9
	dc	xm_ret-xm_cvct		;A
 	dc	xm_vibrato-xm_cvct	;B
 	dc	xm_ret-xm_cvct		;C
 	dc	xm_ret-xm_cvct		;D
 	dc	xm_ret-xm_cvct		;E
 	dc	xm_tonep-xm_cvct	;F


   *************************
   *   Standard effects:   *
   *************************

;!      0      Arpeggio
;!      1  (*) Porta up
;!      2  (*) Porta down
;!      3  (*) Tone porta
;-      4  (*) Vibrato
;!      5  (*) Tone porta+Volume slide
;-      6  (*) Vibrato+Volume slide
;-      7  (*) Tremolo
;*      8      Set panning
;!      9      Sample offset
;!      A  (*) Volume slide
;!      B      Position jump
;!      C      Set volume
;!      D      Pattern break
;!      E1 (*) Fine porta up
;!      E2 (*) Fine porta down
;-      E3     Set gliss control
;-      E4     Set vibrato control
;-      E5     Set finetune
;-      E6     Set loop begin/loop
;-      E7     Set tremolo control
;!      E9     Retrig note
;!      EA (*) Fine volume slide up
;!      EB (*) Fine volume slide down
;!      EC     Note cut
;!      ED     Note delay
;-      EE     Pattern delay
;!      F      Set tempo/BPM
;!      G      Set global volume
;!      H  (*) Global volume slide
;!     	K      Key off
;!      L      Set envelope position
;*      P  (*) Panning slide
;!      R  (*) Multi retrig note
;-      T      Tremor
;-      X1 (*) Extra fine porta up
;-      X2 (*) Extra fine porta down
;
;      (*) = If the command byte is zero, the last nonzero byte for the
;            command should be used.
;
;   *********************************
;   *   Effects in volume column:   *
;   *********************************
;
;   All effects in the volume column should work as the standard effects.
;   The volume column is interpreted before the standard effects, so
;   some standard effects may override volume column effects.
;
;   Value      Meaning
;
;      0       Do nothing
;    $10-$50   Set volume Value-$10
;      :          :        :
;      :          :        :
;!    $60-$6f   Volume slide down
;!    $70-$7f   Volume slide up
;!    $80-$8f   Fine volume slide down
;!    $90-$9f   Fine volume slide up
;-    $a0-$af   Set vibrato speed
;-    $b0-$bf   Vibrato
;*    $c0-$cf   Set panning
;*    $d0-$df   Panning slide left
;*    $e0-$ef   Panning slide right
;!    $f0-$ff   Tone porta

xm_patts	ds.l	256
xm_insts	ds.l	128