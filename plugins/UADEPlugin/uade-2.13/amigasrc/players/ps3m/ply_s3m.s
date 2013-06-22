;;********** S3M Play Routine **********


s3m_init
	lea.l	data,a5
	move.l	s3m(a5),a0
	move.l	a0,mname(a5)
	move	ordernum(a0),d0
	iword	d0
	move	d0,slen(a5)
	move	d0,positioneita(a5)

	move	patnum(a0),d0
	iword	d0
	move	d0,pats(a5)

	move	insnum(a0),d0
	iword	d0
	move	d0,inss(a5)

	move	ffv(a0),d0
	iword	d0
	move	d0,fformat(a5)

	move	flags(a0),d0
	iword	d0
	move	d0,sflags(a5)

	cmp	#$1301,fformat(a5)
	bhi.b	.ok

	bset	#6,sflags+1(a5)

.ok	lea	$60(a0),a1
	moveq	#0,d0
	move	slen(a5),d0
	moveq	#1,d1
	and	d0,d1
	add	d1,d0
	lea	(a1,d0.l),a2
	move.l	a2,samples(a5)

	move	inss(a5),d0
	add	d0,d0
	lea	(a2,d0.l),a3
	move.l	a3,patts(a5)

	moveq	#0,d0
	move.b	(a1),d0
	add	d0,d0
	sub.l	a1,a1
	move	(a3,d0),d0
	beq.b	.q
	iword	d0
	asl.l	#4,d0
	lea	2(a0,d0.l),a1
.q	move.l	a1,ppos(a5)

	moveq	#0,d0
	move.b	initialspeed(a0),d0
	bne.b	.ok2
	moveq	#6,d0
.ok2	move	d0,spd(a5)

	move.b	initialtempo(a0),d0
	cmp	#32,d0
	bhi.b	.qw
	moveq	#125,d0
.qw	move	d0,tempo(a5)

	move	inss(a5),d0
	subq	#1,d0
	move.l	samples(a5),a2
.instloop
	moveq	#0,d1
	move	(a2)+,d1
	iword	d1
	lsl.l	#4,d1
	lea	(a0,d1.l),a1
	btst	#0,insflags(a1)
	beq.b	.eloo
	move.l	insloopend(a1),d1
	ilword	d1
	cmp.l	#2,d1
	bls.b	.eloo
	move.l	insloopend(a1),inslength(a1)
.eloo	dbf	d0,.instloop

	clr	pos(a5)
	clr	rows(a5)
	clr	cn(a5)
	clr	pdelaycnt(a5)
	clr	pjmpflag(a5)
	clr	pbrkflag(a5)
	clr	pbrkrow(a5)

	bsr	detectchannels

	move.l	#14317056/4,clock(a5)		; Clock constant
	move	#64,globalVol(a5)
	moveq	#0,d0
	rts


s3m_music
	lea	data,a5
	move.l	s3m(a5),a0

	addq	#1,cn(a5)
	move	cn(a5),d0
	cmp	spd(a5),d0
	beq.b	uusrow

ccmds	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	subq	#1,d7
.loo	btst	#7,5(a2)
	beq.b	.edi

	lea	cct(pc),a1
	moveq	#0,d0
	move.b	cmd(a2),d0
	cmp	#'Z'-'@',d0
	bhi.b	.edi
	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.edi	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.loo
	rts

uusrow	clr	cn(a5)

	tst	pdelaycnt(a5)
	bne	process

	lea	c0(a5),a2
	move	numchans(a5),d7
	subq	#1,d7
.cl	clr.b	flgs(a2)
	lea	s3mChanBlock_SIZE(a2),a2
	dbf	d7,.cl

	move.l	ppos(a5),a1
	lea	c0(a5),a4		;chanblocks
.loo	move.b	(a1)+,d0
	beq.b	.end

	moveq	#$1f,d5
	and	d0,d5			;chan
	mulu	#s3mChanBlock_SIZE,d5
	lea	(a4,d5),a2

	and	#~31,d0
	move.b	d0,flgs(a2)
	
	moveq	#32,d2
	and	d0,d2
	beq.b	.nnot

	move.b	(a1)+,(a2)
	move.b	(a1)+,inst(a2)

.nnot	moveq	#64,d2
	and	d0,d2
	beq.b	.nvol

	move.b	(a1)+,vol(a2)

.nvol	and	#128,d0
	beq.b	.loo

	move.b	(a1)+,d0
	bmi.b	.d
	move.b	d0,cmd(a2)
.d	move.b	(a1)+,info(a2)
	bra.b	.loo

.end	move.l	a1,ppos(a5)

process	lea	c0(a5),a2
	lea	cha0(a5),a4
	move	numchans(a5),d7
	move.l	samples(a5),a5
	subq	#1,d7
	endb	a5

.lloo	tst.b	flgs(a2)
	beq	.evol

	moveq	#32,d0
	and.b	flgs(a2),d0
	beq	.f

	move.b	inst(a2),d0
	beq	.esmp
	bmi	.esmp

	cmp	inss,d0
	bgt	.mute

	btst	#7,flgs(a2)
	beq.b	.eii
	cmp.b	#'S'-'@',cmd(a2)
	bne.b	.eii
	move.b	info(a2),d1
	and	#$f0,d1
	cmp	#$d0,d1
	beq	.evol

.eii	add	d0,d0
	move	-2(a5,d0),d0
	iword	d0
	lsl	#4,d0
	lea	(a0,d0),a1

	moveq	#0,d0
	move	insmemseg(a1),d0
	iword	d0
	lsl.l	#4,d0
	move.l	a0,d4
	add.l	d0,d4

	move.l	insloopbeg(a1),d1
	ilword	d1
	move.l	insloopend(a1),d2
	ilword	d2
	sub.l	d1,d2
	add.l	d4,d1

	move.l	d1,mLStart(a4)
	move.l	d2,mLLength(a4)
	move.b	insvol(a1),volume+1(a2)
	cmp	#64,volume(a2)
	blo.b	.e
	move	#63,volume(a2)
.e	move.l	a1,sample(a2)

	btst	#0,insflags(a1)
	beq.b	.eloo
	cmp.l	#2,d2
	shi	mLoop(a4)
	bra.b	.esmp


.mute	st	mOnOff(a4)
	bra	.f

.eloo	clr.b	mLoop(a4)
.esmp	moveq	#0,d0
	move.b	(a2),d0
	beq	.f
	cmp.b	#254,d0
	beq.b	.mute
	cmp.b	#255,d0
	beq	.f

	move.b	d0,note(a2)
	move	d0,d1
	lsr	#4,d1

	and	#$f,d0
	add	d0,d0

	move.l	sample(a2),a1
	move.l	$20(a1),d2
	ilword	d2

	lea	Periods(pc),a1
	move	(a1,d0),d0
	mulu	#8363,d0
	lsl.l	#4,d0
	lsr.l	d1,d0	

	divu	d2,d0


	btst	#7,flgs(a2)
	beq.b	.ei

	cmp.b	#'Q'-'@',cmd(a2)	;retrig
	beq.b	.eiik

.ei	clr.b	retrigcn(a2)

.eiik	clr.b	vibpos(a2)


	btst	#7,flgs(a2)
	beq.b	.eitopo

	cmp.b	#'G'-'@',cmd(a2)	;TOPO
	beq.b	.eddo

	cmp.b	#'L'-'@',cmd(a2)	;TOPO+VSLD
	bne.b	.eitopo

.eddo	move	d0,toperiod(a2)
	bra.b	.f

.eitopo	move	d0,mPeriod(a4)
	move	d0,period(a2)
	clr.l	mFPos(a4)

	move.l	sample(a2),d0
	beq.b	.f
	move.l	d0,a1

	moveq	#0,d0
	move	insmemseg(a1),d0
	iword	d0
	lsl.l	#4,d0
	move.l	a0,d4
	add.l	d0,d4

	move.l	inslength(a1),d0
	ilword	d0

	move.l	d4,(a4)
	move.l	d0,mLength(a4)
	clr.b	mOnOff(a4)

.f	moveq	#64,d0
	and.b	flgs(a2),d0
	beq.b	.evol
	move.b	vol(a2),volume+1(a2)
	cmp	#64,volume(a2)
	blo.b	.evol
	move	#63,volume(a2)

.evol	btst	#7,flgs(a2)
	beq.b	.eivib

	cmp.b	#'H'-'@',cmd(a2)
	beq.b	.vib

.eivib	bsr	checklimits
.vib

	btst	#7,flgs(a2)
	beq.b	.eitre

	cmp.b	#'R'-'@',cmd(a2)
	beq.b	.tre
	cmp.b	#'I'-'@',cmd(a2)
	beq.b	.tre

.eitre	move	volume(a2),d0
	mulu	globalVol,d0
	lsr	#6,d0
	move	d0,mVolume(a4)

.tre	btst	#7,flgs(a2)
	beq.b	.edd

	move.b	info(a2),d0
	beq.b	.dd
	move.b	d0,lastcmd(a2)
.dd	lea	ct(pc),a1
	moveq	#0,d0
	move.b	cmd(a2),d0
	cmp	#'Z'-'@',d0
	bhi.b	.edd

	add	d0,d0
	move	(a1,d0),d0
	jsr	(a1,d0)

.edd	lea	s3mChanBlock_SIZE(a2),a2
	lea	mChanBlock_SIZE(a4),a4
	dbf	d7,.lloo

	basereg	data,a5
	lea	data,a5

	tst	pdelaycnt(a5)
	beq.b	.oke

	subq	#1,pdelaycnt(a5)
	bra	xm_exit
.oke
	addq	#1,rows(a5)

	tst	pbrkflag(a5)		; do we have to break this pattern?
	beq.b	.nobrk
	move	pbrkrow(a5),rows(a5)
	clr	pbrkrow(a5)		; clr break position
.nobrk
	cmp	#64,rows(a5)
	bcc.b	.newpos				; worst case opt: branch not taken

	tst	pjmpflag(a5)		; do we have to jump?
	bne.b	.newpos

	tst	pbrkflag(a5)
	beq	dee
	bra.b	cont
.newpos
	move	pbrkrow(a5),rows(a5)
	clr	pbrkrow(a5)		; clr break position

burk	addq	#1,pos(a5)			; inc songposition
	move	slen(a5),d0
	cmp	pos(a5),d0			; are we thru with all patterns?
	bgt.b	cont				; nope
	clr	pos(a5)
	st	PS3M_break(a5)

	 move.l	#-1,uade_restart_cmd

	moveq	#0,d0
	move.b	initialspeed(a0),d0
	bne.b	.ok
	moveq	#6,d0
.ok	move	d0,spd(a5)

cont	move	pos(a5),d0
	move	d0,PS3M_position(a5)
	st	PS3M_poscha(a5)

	moveq	#0,d1
	move.b	orders(a0,d0),d1
	cmp.b	#$fe,d1				; marker that is skipped
	beq.b	burk
	cmp.b	#$ff,d1				; end of tune mark
	beq.b	burk
	cmp	pats(a5),d1
	bhs.b	burk

	add	d1,d1
	move.l	patts(a5),a3
	moveq	#0,d0
	move	(a3,d1),d0
	beq.b	burk
	iword	d0
	lsl.l	#4,d0
	lea	2(a0,d0.l),a1

	clr	pjmpflag(a5)
	clr	pbrkflag(a5)

	move	rows(a5),d0
	beq.b	.setp
	subq	#1,d0
	moveq	#0,d1
.loop	move.b	(a1)+,d1
	beq.b	.next

	moveq	#32,d2
	and	d1,d2
	beq.b	.nnot
	addq	#2,a1
.nnot
	moveq	#64,d2
	and	d1,d2
	beq.b	.nvol
	addq	#1,a1
.nvol
	and	#128,d1
	beq.b	.loop
	addq	#2,a1

	bra.b	.loop
.next
	dbf	d0,.loop
.setp
	move.l	a1,ppos(a5)

dee	bra	xm_dee
	endb	a5


ct	dc	rt-ct
	dc	changespeed-ct
	dc	posjmp-ct
	dc	patbrk-ct
	dc	vslide-ct
	dc	portadwn-ct
	dc	portaup-ct
	dc	rt-ct
	dc	rt-ct
	dc	tremor-ct
	dc	arpeggio-ct
	dc	rt-ct
	dc	rt-ct
	dc	rt-ct
	dc	rt-ct
	dc	soffset-ct
	dc	rt-ct
	dc	retrig-ct
	dc	rt-ct
	dc	specials-ct
	dc	stempo-ct
	dc	rt-ct
	dc	setmaster-ct
	dc	rt-ct
	dc	rt-ct
	dc	rt-ct
	dc	rt-ct



cct	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	vslide-cct
	dc	portadwn-cct
	dc	portaup-cct
	dc	noteporta-cct
	dc	vibrato-cct
	dc	tremor-cct
	dc	arpeggio-cct
	dc	vvslide-cct
	dc	pvslide-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	retrig-cct
	dc	tremolo-cct
	dc	specials-cct
	dc	rt-cct
	dc	finevib-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct
	dc	rt-cct

tremolo
rt	rts

tremor
	move.b	info(a2),d0
	beq.b	.toggle
	move.b	d0,tvalue(a2)
.toggle
	subq.b	#1,tcount(a2)
	bhi.b	.volume
	move.b	tvalue(a2),d0
	not	ttoggle(a2)
	beq.b	.off
	lsr.b	#4,d0				; ontime
.off	and.b	#$f,d0				; offtime
	move.b	d0,tcount(a2)
.volume
	move	volume(a2),d0
	and	ttoggle(a2),d0
	move	d0,mVolume(a4)
	rts

changespeed
	move.b	info(a2),d0
	bne.b	.d
	moveq	#6,d0
.d	cmp.b	#32,d0
	bcs.b	.e
	moveq	#31,d0
.e	move.b	d0,spd+1
	rts

posjmp	clr	pbrkrow
	st	pjmpflag

	moveq	#0,d0
	move.b	pos,d0
	addq	#1,d0

	cmp	slen,d0
	bne.b	.notlast
	st	PS3M_break
.notlast
	moveq	#0,d0
	move.b	info(a2),d0
	cmp	pos,d0
	bhi.b	.e
	st	PS3M_break
.e	subq	#1,d0
	move	d0,pos
	st	PS3M_poscha
	rts

patbrk	moveq	#0,d0
	move.b	info(a2),d0
	moveq	#$f,d2
	and	d0,d2
	lsr	#4,d0
	add.b	.dtab(pc,d0),d2
	cmp.b	#63,d2		; valid line number given?
	ble.b	.ok	
	moveq	#0,d2		; else zero it
.ok	move	d2,pbrkrow
	st	pjmpflag
	st	PS3M_poscha
	rts

.dtab:	dc.b	0,10,20,30	; Don't think this little table is a waste!
	dc.b 	40,50,60,70	; The routine is shorter using this table
	dc.b	80,90,100,110	; and faster too :-)
	dc.b 	120,130,140,150	; 16 bytes vs. 8 instructions (wordlength)

vslide	moveq	#0,d0
	move.b	lastcmd(a2),d0
	moveq	#$f,d1
	and	d0,d1
	move	d0,d2
	lsr	#4,d2

	cmp.b	#$f,d1
	beq.b	.addfine

	cmp.b	#$f,d2
	beq.b	.subfine

	btst	#6,sflags+1
	bne.b	.ok

	tst	cn
	beq.b	.dd	

.ok	tst	d1
	beq.b	.add
	and	#$f,d0
	bra.b	.sub

.subfine
	tst	cn
	bne.b	.dd
	and	#$f,d0
.sub	sub	d0,volume(a2)
	bpl.b	.dd
	clr	volume(a2)
.dd	move	volume(a2),d0
	mulu	globalVol,d0
	lsr	#6,d0
	move	d0,mVolume(a4)
	rts

.addfine
	tst	d2
	beq.b	.sub
	tst	cn
	bne.b	.dd
.add	lsr	#4,d0

.add2	add	d0,volume(a2)
	cmp	#64,volume(a2)
	blo.b	.dd
	move	#63,volume(a2)
	bra.b	.dd


portadwn
	moveq	#0,d0
	move.b	lastcmd(a2),d0

	tst	cn
	beq.b	.fined
	cmp.b	#$e0,d0
	bhs.b	.dd
	lsl	#2,d0

.ddd	add	d0,period(a2)
	bra.b	checklimits
.dd	rts

.fined	cmp.b	#$e0,d0
	bls.b	.dd
	cmp.b	#$f0,d0
	bls.b	.extr
	and	#$f,d0
	lsl	#2,d0
	bra.b	.ddd

.extr	and	#$f,d0
	bra.b	.ddd

portaup
	moveq	#0,d0
	move.b	lastcmd(a2),d0

	tst	cn
	beq.b	.fined
	cmp.b	#$e0,d0
	bhs.b	.dd
	lsl	#2,d0

.ddd	sub	d0,period(a2)
	bra.b	checklimits

.dd	rts

.fined	cmp.b	#$e0,d0
	bls.b	.dd
	cmp.b	#$f0,d0
	bls.b	.extr
	and	#$f,d0
	lsl	#2,d0
	bra.b	.ddd

.extr	and	#$f,d0
	bra.b	.ddd


checklimits
	move	period(a2),d0
	btst	#4,sflags+1
	beq.b	.sii
	
	cmp	#856*4,d0
	bls.b	.dd
	move	#856*4,d0
.dd	cmp	#113*4,d0
	bhs.b	.dd2
	move	#113*4,d0
.dd2	move	d0,period(a2)
	move	d0,mPeriod(a4)
	rts

.sii	cmp	#$7fff,d0
	bls.b	.dd3
	move	#$7fff,d0
.dd3	cmp	#64,d0
	bhs.b	.dd4
	move	#64,d0
.dd4	move	d0,mPeriod(a4)
	rts


noteporta
	move.b	info(a2),d0
	beq.b	notchange
	move.b	d0,notepspd(a2)
notchange
	move	toperiod(a2),d0
	beq.b	.1
	moveq	#0,d1
	move.b	notepspd(a2),d1
	lsl	#2,d1

	cmp	period(a2),d0
	blt.b	.topoup

	add	d1,period(a2)
	cmp	period(a2),d0
	bgt.b	.1
	move	d0,period(a2)
	clr	toperiod(a2)
.1	move	period(a2),mPeriod(a4)
	rts

.topoup	sub	d1,period(a2)
	cmp	period(a2),d0
	blt.b	.dd
	move	d0,period(a2)
	clr	toperiod(a2)
.dd	move	period(a2),mPeriod(a4)
	rts


vibrato	move.b	cmd(a2),d0
	bne.b	.e
	move.b	vibcmd(a2),d0
	bra.b	.skip2

.e	move	d0,d1
	and	#$f0,d1
	bne.b	.skip2

	move.b	vibcmd(a2),d1
	and	#$f0,d1
	or	d1,d0

.skip2
	move.b	d0,vibcmd(a2)

vibrato2
	moveq	#$1f,d0
	and.b	vibpos(a2),d0
	moveq	#0,d2
	lea	mt_vibratotable(pc),a3
	move.b	(a3,d0),d2
	moveq	#$f,d0
	and.b	vibcmd(a2),d0
	mulu	d0,d2

	moveq	#4,d0
	btst	#0,sflags+1
	bne.b	.sii
	moveq	#5,d0
.sii	lsr	d0,d2
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


finevib	move.b	cmd(a2),d0
	bne.b	.e
	move.b	vibcmd(a2),d0
	bra.b	.skip2

.e	move	d0,d1
	and	#$f0,d1
	bne.b	.skip2

	move.b	vibcmd(a2),d1
	and	#$f0,d1
	or	d1,d0

.skip2
	move.b	d0,vibcmd(a2)
	moveq	#$1f,d0
	and.b	vibpos(a2),d0
	moveq	#0,d2
	lea	mt_vibratotable(pc),a3
	move.b	(a3,d0),d2
	moveq	#$f,d0
	and.b	vibcmd(a2),d0
	mulu	d0,d2

	lsr	#7,d2
	move	period(a2),d0
	btst	#5,vibpos(a2)
	bne.b	.neg
	add	d2,d0
	bra.b	.vib3
.neg	sub	d2,d0
.vib3	move	d0,mPeriod(a4)
	move.b	vibcmd(a2),d0
	lsr.b	#4,d0
	add.b	d0,vibpos(a2)
	rts


arpeggio
	moveq	#0,d0
	move.b	note(a2),d0
	beq.b	.qq

	moveq	#$70,d1
	and	d0,d1
	and	#$f,d0

	moveq	#0,d2
	move	cn,d2
	divu	#3,d2
	swap	d2
	tst	d2
	beq.b	.norm
	subq	#1,d2
	beq.b	.1

	moveq	#$f,d2
	and.b	lastcmd(a2),d2
	add	d2,d0
.f	cmp	#12,d0
	blt.b	.norm
	sub	#12,d0
	add	#$10,d1
	bra.b	.f

.1	move.b	lastcmd(a2),d2
	lsr.b	#4,d2
	add.b	d2,d0
.f2	cmp	#12,d0
	blt.b	.norm
	sub	#12,d0
	add	#$10,d1
	bra.b	.f2

.norm	add	d0,d0
	lsr	#4,d1

	move.l	sample(a2),a1

	move.l	$20(a1),d2
	ilword	d2

	lea	Periods(pc),a1
	move	(a1,d0),d0
	mulu	#8363,d0
	lsl.l	#4,d0
	lsr.l	d1,d0
	divu	d2,d0
	move	d0,mPeriod(a4)
.qq	rts


pvslide	bsr	notchange
	bra	vslide

vvslide	bsr	vibrato2
	bra	vslide

soffset	moveq	#32,d0
	and.b	flgs(a2),d0
	beq	.f
	move.b	(a2),d0
	beq	.f
	cmp.b	#255,d0
	beq	.f

	move.l	sample(a2),d0
	beq.b	.f
	move.l	d0,a1

	moveq	#0,d0
	move	insmemseg(a1),d0
	iword	d0
	lsl.l	#4,d0
	move.l	a0,d4
	add.l	d0,d4

	move.l	inslength(a1),d0
	ilword	d0

	moveq	#0,d2
	move.b	lastcmd(a2),d2
	lsl.l	#8,d2
	add.l	d2,d4
	sub.l	d2,d0
	bpl.b	.ok
	move.l	mLStart(a4),d4
	move.l	mLLength(a4),d0
.ok	move.l	d4,(a4)
	move.l	d0,mLength(a4)
.f	rts


retrig	move.b	retrigcn(a2),d0
	subq.b	#1,d0
	cmp.b	#0,d0
	ble.b	.retrig

	move.b	d0,retrigcn(a2)
	rts

.retrig	move.l	sample(a2),d0
	beq	.f
	move.l	d0,a1
	moveq	#0,d1
	move	insmemseg(a1),d1
	iword	d1
	lsl.l	#4,d1
	move.l	a0,d4
	add.l	d1,d4

	move.l	inslength(a1),d1
	ilword	d1

	move.l	d4,(a4)
	move.l	d1,mLength(a4)
	clr.b	mOnOff(a4)
	clr.l	mFPos(a4)

	move.b	lastcmd(a2),d0
	moveq	#$f,d1
	and.b	d0,d1
	move.b	d1,retrigcn(a2)

	and	#$f0,d0
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
	blo.b	.ei64
	move	#63,volume(a2)
.ei64	move	volume(a2),d0
	mulu	globalVol,d0
	lsr	#6,d0
	move	d0,mVolume(a4)
.f	rts

; NOTE: All subroutines expect the command parameter (the x of Fx, that is) in d0!
specials
	move.b	info(a2),d1
	moveq	#$f,d0
	and	d1,d0
	and	#$f0,d1
	cmp	#$b0,d1
	beq.b	.ploop
	cmp	#$d0,d1
	beq.b	.delay
	cmp	#$e0,d1
	beq.b	.pdelay
	cmp	#$c0,d1
	bne.b	.dd

	cmp	cn,d0
	bne.b	.dd
	clr	volume(a2)
	clr	mVolume(a4)
.dd	rts

.ploop	tst	cn
	bne.b	.dd

	tst	d0
	beq.b	.setlp			; 0 means "set loop mark" in current line

	tst	loopcnt(a2)		; dont allow nesting, accept value
	beq.b	.jcnt			; only if we counted down the last loop

	subq	#1,loopcnt(a2)		; count down
	bne.b	.jloop			; jump again if still not zero
	rts
.jcnt	move	d0,loopcnt(a2)		; accept new loop value
.jloop	move	looprow(a2),pbrkrow	; put line number to jump to
	st	pbrkflag
	rts
.setlp	move	rows,looprow(a2)
	rts

.pdelay	tst	cn
	bne.b	.dd

	tst	pdelaycnt
	bne.b	.skip

	move	d0,pdelaycnt
.skip	rts

.delay	cmp	cn,d0
	bne.b	.dd
	
	moveq	#32,d0
	and.b	flgs(a2),d0
	beq	.f

	move.b	inst(a2),d0
	beq	.esmp
	bmi	.esmp

	cmp	inss,d0
	bgt	.dd

	move.l	samples,a5
	add	d0,d0
	move	-2(a5,d0),d0
	iword	d0
	asl	#4,d0
	lea	(a0,d0),a1

	moveq	#0,d0
	move	insmemseg(a1),d0
	iword	d0
	asl.l	#4,d0
	move.l	a0,d4
	add.l	d0,d4

	move.l	insloopbeg(a1),d1
	ilword	d1
	move.l	insloopend(a1),d2
	ilword	d2
	sub.l	d1,d2
	add.l	d4,d1

	move.l	inslength(a1),d0
	ilword	d0

	move.l	d4,(a4)
	move.l	d0,mLength(a4)
	move.l	d1,mLStart(a4)
	move.l	d2,mLLength(a4)
	move.b	insvol(a1),volume+1(a2)
	cmp	#64,volume(a2)
	blo.b	.e
	move	#63,volume(a2)
.e	clr.b	mOnOff(a4)

	move.l	a1,sample(a2)

	btst	#0,insflags(a1)
	bne.b	.loo
	clr.b	mLoop(a4)
	bra.b	.esmp
.loo	cmp.l	#2,d2
	shi	mLoop(a4)

.esmp	moveq	#0,d0
	move.b	(a2),d0
	beq.b	.f
	bmi.b	.f

	moveq	#$70,d1
	and	d0,d1
	lsr	#4,d1

	and	#$f,d0
	add	d0,d0

	move.l	sample(a2),a1

	move.l	$20(a1),d2
	ilword	d2

	lea	Periods(pc),a1
	move	(a1,d0),d0
	mulu	#8363,d0
	lsl.l	#4,d0
	lsr.l	d1,d0
	divu	d2,d0

	move	d0,mPeriod(a4)
	move	d0,period(a2)
	clr.l	mFPos(a4)
	clr.b	vibpos(a2)

.f	moveq	#64,d0
	and.b	flgs(a2),d0
	beq.b	.evol
	move.b	vol(a2),volume+1(a2)
	cmp	#64,volume(a2)
	blo.b	.evol
	move	#63,volume(a2)
.evol	move	volume(a2),d0
	mulu	globalVol,d0
	lsr	#6,d0
	move	d0,mVolume(a4)
	rts


stempo	moveq	#0,d0
	move.b	info(a2),d0
	cmp	#32,d0
	bls.b	.e
	move.l	mrate,d1
	move.l	d1,d2
	lsl.l	#2,d1
	add.l	d2,d1
	add	d0,d0
	divu	d0,d1

	addq	#1,d1
	and	#~1,d1
	move	d1,bytesperframe
.e	rts

setmaster
	moveq	#0,d0
	move.b	info(a2),d0
	cmp	#64,d0
	bls.b	.d
	moveq	#64,d0
.d	move	d0,globalVol
	rts

Periods
 dc	1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,907

ftab1	dc.b	0,-1,-2,-4,-8,-16,0,0
	dc.b	0,1,2,4,8,16,0,0

ftab2	dc.b	0,0,0,0,0,0,10,8
	dc.b	0,0,0,0,0,0,24,32









