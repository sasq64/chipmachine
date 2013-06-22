
********** Protracker (Fasttracker player) **************

n_note		equ	0
n_cmd		equ	2
n_cmdlo		equ	3
n_start		equ	4
n_length	equ	8
n_loopstart	equ	10
n_replen	equ	14
n_period	equ	16
n_finetune	equ	18
n_volume	equ	19
n_dmabit	equ	20
n_toneportdirec	equ	22
n_toneportspeed	equ	23
n_wantedperiod	equ	24
n_vibratocmd	equ	26
n_vibratopos	equ	27
n_tremolocmd	equ	28
n_tremolopos	equ	29
n_wavecontrol	equ	30
n_glissfunk	equ	31
n_sampleoffset	equ	32
n_pattpos	equ	33
n_loopcount	equ	34
n_funkoffset	equ	35
n_wavestart	equ	36
n_reallength	equ	40

	basereg	data,a5
mtm_init
	lea	data,a5
	move.l	a0,s3m(a5)
	move.l	a0,mt_songdataptr
	lea	4(a0),a1
	move.l	a1,mname(a5)

	move.l	#mtm_periodtable,peris
	move	#1616,lowlim
	move	#45,upplim
	move	#126,octs

	moveq	#0,d0
	move.b	27(a0),d0
	addq.b	#1,d0
	move.b	d0,slene
	move	d0,positioneita(a5)

	moveq	#0,d1
	move.b	33(a0),d1
	move	d1,numchans(a5)

	moveq	#0,d0
	move.b	32(a0),d0
	lsl	#2,d0
	mulu	d1,d0
	move	d0,patlen

	move.l	a0,d0
	add.l	#66,d0

	moveq	#0,d1
	move.b	30(a0),d1			; NOS
	mulu	#37,d1
	add.l	d1,d0

	move.l	d0,orderz
	add.l	#128,d0
	move.l	d0,tracks

	move	24(a0),d1			; number of tracks
	iword	d1
	mulu	#192,d1
	add.l	d1,d0

	move.l	d0,sequ

	moveq	#0,d1
	move.b	26(a0),d1			; last pattern saved
	addq	#1,d1
	lsl	#6,d1
	add.l	d1,d0

	moveq	#0,d1
	move	28(a0),d1			; length of comment field
	iword	d1
	add.l	d1,d0

	lea	66(a0),a2			; sample infos

	moveq	#0,d7
	move.b	30(a0),d7			; NOS
	subq	#1,d7

	lea	mt_sampleinfos(pc),a1
.loop	move.l	d0,(a1)+
	lea	22(a2),a3
	tlword	(a3)+,d1
	add.l	d1,d0
	lsr.l	#1,d1
	move	d1,(a1)+

	lea	26(a2),a3
	tlword	(a3)+,d1
	lsr.l	#1,d1
	move	d1,(a1)+			; rep offset

	lea	30(a2),a3
	tlword	(a3)+,d2
	lsr.l	#1,d2
	sub.l	d1,d2	
	move	d2,(a1)+			; rep length

	clr.b	(a1)+				; no finetune
	move.b	35(a2),(a1)+			; volume

	lea	37(a2),a2
	dbf	d7,.loop

	or.b	#2,$bfe001
	move.b	#6,mt_speed
	clr.b	mt_counter
	clr.b	mt_songpos
	clr	mt_patternpos

	move	#2,fformat			; unsigned data
	move	#125,tempo
	move.l	#14317056/4,clock		; Clock constant

	lea	34(a0),a2
	lea	pantab(a5),a0
	move.l	a0,a1
	moveq	#7,d0
.ll	clr.l	(a1)+
	dbf	d0,.ll

	move	numchans(a5),d0
	subq	#1,d0
	moveq	#0,d5
	moveq	#0,d6
.lop	move.b	(a2)+,d1
	cmp.b	#8,d1
	blo.b	.vas
	move.b	#-1,(a0)+
	addq	#1,d5
	bra.b	.je
.vas	move.b	#1,(a0)+
	addq	#1,d6
.je	dbf	d0,.lop

	cmp	d5,d6
	bls.b	.k
	move	d6,d5
.k	move	d5,maxchan(a5)

	lea	mt_chan1temp(pc),a0
	move	#44*8-1,d0
.cl	clr.l	(a0)+
	dbf	d0,.cl

	moveq	#0,d0
	rts

orderz	dc.l	0
tracks	dc.l	0
sequ	dc.l	0
slene	dc	0
patlen	dc	0
upplim	dc	0
lowlim	dc	0
peris	dc.l	0
octs	dc	0

mt_init	lea	data,a5
	move.l	s3m(a5),a0
	move.l	a0,mname
	move.l	a0,mt_songdataptr
	move.l	a0,a1
	moveq	#0,d0
	move.b	950(a1),d0
	move.b	d0,slene
	move	d0,positioneita

	move	#256,d0
	mulu	numchans(a5),d0
	move	d0,patlen

	move	#113,upplim
	move	#856,lowlim
	move.l	#mt_periodtable,peris
	move	#36*2,octs

	lea	952(a1),a1
	moveq	#127,d0
	moveq	#0,d1
mtloop	move.l	d1,d2
	subq	#1,d0
mtloop2	move.b	(a1)+,d1
	cmp.b	d2,d1
	bgt.b	mtloop
	dbra	d0,mtloop2
	addq.b	#1,d2
			
	lea	mt_sampleinfos(pc),a1
	asl	#8,d2
	mulu	numchans(a5),d2

	add.l	#1084,d2
	add.l	a0,d2
	move.l	d2,a2
	moveq	#30,d0
mtloop3	move.l	a2,(a1)+
	moveq	#0,d1
	move	42(a0),d1
	move	d1,(a1)+
	asl.l	#1,d1
	add.l	d1,a2

	move	46(a0),(a1)+
	move	48(a0),(a1)+
	move	44(a0),(a1)+			; finetune and volume

	add.l	#30,a0
	dbra	d0,mtloop3

	or.b	#2,$bfe001
	move.b	#6,mt_speed
	clr.b	mt_counter
	clr.b	mt_songpos
	clr	mt_patternpos

	move	#1,fformat
	move	#125,tempo
	move.l	#14187580/4,clock		; Clock constant

	lea	pantab(a5),a0
	move.l	a0,a1
	moveq	#7,d0
.ll	clr.l	(a1)+
	dbf	d0,.ll

	move	numchans(a5),d0
	subq	#1,d0
	moveq	#0,d1
.lop	tst	d1
	beq.b	.vas
	cmp	#3,d1
	beq.b	.vas
.oik	move.b	#-1,(a0)+
	bra.b	.je
.vas	move.b	#1,(a0)+
.je	addq	#1,d1
	and	#3,d1
	dbf	d0,.lop

	lea	mt_chan1temp(pc),a0
	move	#44*8-1,d0
.cl	clr.l	(a0)+
	dbf	d0,.cl

	moveq	#0,d0
	rts

	endb	a5
mt_music
	movem.l	d0-d4/a0-a6,-(sp)
	addq.b	#1,mt_counter
	move.b	mt_counter(pc),d0
	cmp.b	mt_speed(pc),d0
	blo.b	mt_nonewnote
	clr.b	mt_counter
	tst.b	mt_pattdeltime2
	beq.b	mt_getnewnote
	bsr.b	mt_nonewallchannels
	bra	mt_dskip

mt_nonewnote
	bsr.b	mt_nonewallchannels
	bra	mt_nonewposyet

mt_nonewallchannels
	move	numchans,d7
	subq	#1,d7
	lea	cha0,a5
	lea	mt_chan1temp(pc),a6
.loo	move	d7,-(sp)
	bsr	mt_checkefx
	move	(sp)+,d7
	lea	mChanBlock_SIZE(a5),a5
	lea	44(a6),a6			; Size of MT_chanxtemp
	dbf	d7,.loo
	rts

mt_getnewnote
	move.l	mt_songdataptr(pc),a0
	lea	12(a0),a3
	lea	952(a0),a2	;pattpo
	lea	1084(a0),a0	;patterndata
	moveq	#0,d0
	moveq	#0,d1
	move.b	mt_songpos(pc),d0
	move.b	(a2,d0),d1
	asl.l	#8,d1
	mulu	numchans,d1
	add	mt_patternpos(pc),d1
	clr	mt_dmacontemp

	cmp	#mtMTM,mtype
	bne.b	.ei
	moveq	#0,d1
.ei
	move	numchans,d7
	subq	#1,d7
	lea	cha0,a5
	lea	mt_chan1temp(pc),a6
.loo	move	d7,-(sp)

	tst.l	(a6)
	bne.b	.mt_plvskip
	bsr	mt_pernop
.mt_plvskip
	bsr.b	getnew

	bsr	mt_playvoice
	move	(sp)+,d7
	lea	mChanBlock_SIZE(a5),a5
	lea	44(a6),a6			; Size of MT_chanxtemp
	dbf	d7,.loo

	bra	mt_setdma

getnew	cmp	#mtMOD,mtype
	bne.b	.mtm
	move.l	(a0,d1.l),(a6)
	addq.l	#4,d1
	rts

.mtm	move.l	mt_songdataptr(pc),a0
	move.l	orderz(pc),a2
	moveq	#0,d0
	move.b	mt_songpos(pc),d0
	move.b	(a2,d0),d0

	lsl	#6,d0				; 32 channels * word
	move.l	sequ(pc),a2
	add	d1,d0
	move.b	(a2,d0),d2
	lsl	#8,d2
	move.b	1(a2,d0),d2
	move	d2,d0
	beq.b	.zero
	iword	d0
	move.l	tracks(pc),a2
	subq	#1,d0
	mulu	#192,d0

	moveq	#0,d2
	move	mt_patternpos(pc),d2
	divu	numchans,d2
	lsr	#2,d2
	mulu	#3,d2
	add.l	d2,d0

	moveq	#0,d2
	move.b	(a2,d0.l),d2
	lsr	#2,d2
	beq.b	.huu
	move.l	peris(pc),a1
	subq	#1,d2
	add	d2,d2
	move	(a1,d2),d2

.huu	clr.l	(a6)
	or	d2,(a6)

	moveq	#0,d2
	move.b	(a2,d0.l),d2
	lsl	#8,d2
	move.b	1(a2,d0.l),d2
	and	#$3f0,d2
	lsr	#4,d2
	move.b	d2,d3
	and	#$10,d3
	or.b	d3,(a6)
	lsl.b	#4,d2
	or.b	d2,2(a6)

	moveq	#0,d2
	move.b	1(a2,d0.l),d2
	lsl	#8,d2
	move.b	2(a2,d0.l),d2
	and	#$fff,d2
	or	d2,2(a6)

	addq.l	#2,d1
	rts

.zero	clr.l	(a6)
	addq.l	#2,d1
	rts

mt_playvoice
	moveq	#0,d2
	move.b	n_cmd(a6),d2
	and.b	#$f0,d2
	lsr.b	#4,d2
	move.b	(a6),d0
	and.b	#$f0,d0
	or.b	d0,d2
	tst.b	d2
	beq	mt_setregs
	moveq	#0,d3
	lea	mt_sampleinfos(pc),a1
	move	d2,d4
	subq	#1,d4
	mulu	#12,d4
	move.l	(a1,d4.l),n_start(a6)
	move	4(a1,d4.l),n_length(a6)
	move	4(a1,d4.l),n_reallength(a6)
	move.b	10(a1,d4.l),n_finetune(a6)
	move.b	11(a1,d4.l),n_volume(a6)
	move	6(a1,d4.l),d3 ; get repeat
	tst	d3
	beq.b	mt_noloop
	tst	8(a1,d4.l)
	beq.b	mt_noloop
	move.l	n_start(a6),d2	; get start
	asl	#1,d3
	add.l	d3,d2		; add repeat
	move.l	d2,n_loopstart(a6)
	move.l	d2,n_wavestart(a6)
	move	6(a1,d4.l),d0	; get repeat
	add	8(a1,d4.l),d0	; add replen
	move	d0,n_length(a6)
	move	8(a1,d4.l),n_replen(a6)	; save replen
	moveq	#0,d0
	move.b	n_volume(a6),d0
	move	d0,mVolume(a5)	; set volume
	bra.b	mt_setregs

mt_noloop
	move.l	n_start(a6),d2
	move.l	d2,n_loopstart(a6)
	move.l	d2,n_wavestart(a6)
	move	8(a1,d4.l),n_replen(a6)	; save replen
	moveq	#0,d0
	move.b	n_volume(a6),d0
	move	d0,mVolume(a5)	; set volume
mt_setregs
	move	(a6),d0
	and	#$fff,d0
	beq	mt_checkmoreefx	; if no note
	move	2(a6),d0
	and	#$ff0,d0
	cmp	#$e50,d0
	beq.b	mt_dosetfinetune
	move.b	2(a6),d0
	and.b	#$f,d0
	cmp.b	#3,d0	; toneportamento
	beq.b	mt_chktoneporta
	cmp.b	#5,d0
	beq.b	mt_chktoneporta
	cmp.b	#9,d0	; sample offset
	bne.b	mt_setperiod
	bsr	mt_checkmoreefx
	bra.b	mt_setperiod

mt_dosetfinetune
	bsr	mt_setfinetune
	bra.b	mt_setperiod

mt_chktoneporta
	bsr	mt_settoneporta
	bra	mt_checkmoreefx

mt_setperiod
	movem.l	d0-d1/a0-a1,-(sp)
	move	(a6),d1
	and	#$fff,d1
	move.l	peris(pc),a1
	moveq	#0,d0
	move	octs(pc),d7
	lsr	#1,d7
mt_ftuloop
	cmp	(a1,d0),d1
	bhs.b	mt_ftufound
	addq.l	#2,d0
	dbra	d7,mt_ftuloop
mt_ftufound
	moveq	#0,d1
	move.b	n_finetune(a6),d1
	mulu	octs(pc),d1
	add.l	d1,a1
	move	(a1,d0),n_period(a6)
	movem.l	(sp)+,d0-d1/a0-a1

	move	2(a6),d0
	and	#$ff0,d0
	cmp	#$ed0,d0 ; notedelay
	beq	mt_checkmoreefx

	btst	#2,n_wavecontrol(a6)
	bne.b	mt_vibnoc
	clr.b	n_vibratopos(a6)
mt_vibnoc
	btst	#6,n_wavecontrol(a6)
	bne.b	mt_trenoc
	clr.b	n_tremolopos(a6)
mt_trenoc
	move.l	n_start(a6),(a5)	; set start
	moveq	#0,d0
	move	n_length(a6),d0
	add.l	d0,d0
	move.l	d0,mLength(a5)		; set length
	move	n_period(a6),d0
	lsl	#2,d0
	move	d0,mPeriod(a5)		; set period

	clr.b	mOnOff(a5)		; turn on
	clr.l	mFPos(a5)		; retrig
	bra	mt_checkmoreefx

 
mt_setdma
	move	numchans,d7
	subq	#1,d7
	lea	cha0,a5
	lea	mt_chan1temp(pc),a6
.loo	move	d7,-(sp)
	bsr	setreg
	move	(sp)+,d7
	lea	mChanBlock_SIZE(a5),a5
	lea	44(a6),a6			; Size of MT_chanxtemp
	dbf	d7,.loo

mt_dskip
	moveq	#4,d0
	mulu	numchans,d0
	add	d0,mt_patternpos
	move.b	mt_pattdeltime,d0
	beq.b	mt_dskc
	move.b	d0,mt_pattdeltime2
	clr.b	mt_pattdeltime
mt_dskc	tst.b	mt_pattdeltime2
	beq.b	mt_dska
	subq.b	#1,mt_pattdeltime2
	beq.b	mt_dska

	moveq	#4,d0
	mulu	numchans,d0
	sub	d0,mt_patternpos

mt_dska	tst.b	mt_pbreakflag
	beq.b	mt_nnpysk
	sf	mt_pbreakflag
	moveq	#0,d0
	move.b	mt_pbreakpos(pc),d0
	clr.b	mt_pbreakpos
	lsl	#2,d0
	mulu	numchans,d0	
	move	d0,mt_patternpos
mt_nnpysk
	move	patlen(pc),d0
	cmp	mt_patternpos(pc),d0
	bhi.b	mt_nonewposyet
mt_nextposition	
	moveq	#0,d0
	move.b	mt_pbreakpos(pc),d0
	lsl	#2,d0
	mulu	numchans,d0
	move	d0,mt_patternpos
	clr.b	mt_pbreakpos
	clr.b	mt_posjumpflag
	addq.b	#1,mt_songpos
	and.b	#$7f,mt_songpos

	moveq	#0,d1
	move.b	mt_songpos(pc),d1
	st	PS3M_poscha
	move	d1,PS3M_position

	cmp.b	slene(pc),d1
	blo.b	mt_nonewposyet
	clr.b	mt_songpos
	st	PS3M_break
	move.l	#-1,uade_restart_cmd

mt_nonewposyet	
	tst.b	mt_posjumpflag
	bne.b	mt_nextposition
	movem.l	(sp)+,d0-d4/a0-a6
	rts


setreg	move.l	n_loopstart(a6),mLStart(a5)
	moveq	#0,d0
	move	n_replen(a6),d0
	add.l	d0,d0
	move.l	d0,mLLength(a5)
	cmp.l	#2,mLLength(a5)
	bls.b	.eloo
	st	mLoop(a5)
	tst.b	mOnOff(a5)
	beq.b	.ok
	clr.b	mOnOff(a5)
	clr.l	mFPos(a5)
.ok	rts
.eloo	clr.b	mLoop(a5)
	rts


mt_checkefx
	bsr	mt_updatefunk
	move	n_cmd(a6),d0
	and	#$fff,d0
	beq.b	mt_pernop
	move.b	n_cmd(a6),d0
	and.b	#$f,d0
	beq.b	mt_arpeggio
	cmp.b	#1,d0
	beq	mt_portaup
	cmp.b	#2,d0
	beq	mt_portadown
	cmp.b	#3,d0
	beq	mt_toneportamento
	cmp.b	#4,d0
	beq	mt_vibrato
	cmp.b	#5,d0
	beq	mt_toneplusvolslide
	cmp.b	#6,d0
	beq	mt_vibratoplusvolslide
	cmp.b	#$e,d0
	beq	mt_e_commands
setback	move	n_period(a6),d2
	lsl	#2,d2
	move	d2,mPeriod(a5)
	cmp.b	#7,d0
	beq	mt_tremolo
	cmp.b	#$a,d0
	beq	mt_volumeslide
mt_return2
	rts

mt_pernop
	move	n_period(a6),d2
	lsl	#2,d2
	move	d2,mPeriod(a5)
	rts

mt_arpeggio
	moveq	#0,d0
	move.b	mt_counter(pc),d0
	divs	#3,d0
	swap	d0
	cmp	#0,d0
	beq.b	mt_arpeggio2
	cmp	#2,d0
	beq.b	mt_arpeggio1
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	lsr.b	#4,d0
	bra.b	mt_arpeggio3

mt_arpeggio1
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	and.b	#15,d0
	bra.b	mt_arpeggio3

mt_arpeggio2
	move	n_period(a6),d2
	bra.b	mt_arpeggio4

mt_arpeggio3
	asl	#1,d0
	moveq	#0,d1
	move.b	n_finetune(a6),d1
	mulu	octs(pc),d1
	move.l	peris(pc),a0
	add.l	d1,a0
	moveq	#0,d1
	move	n_period(a6),d1
	move	octs(pc),d7
	lsr	#1,d7
	subq	#1,d7
mt_arploop
	move	(a0,d0),d2
	cmp	(a0),d1
	bhs.b	mt_arpeggio4
	addq.l	#2,a0
	dbra	d7,mt_arploop
	rts

mt_arpeggio4
	lsl	#2,d2
	move	d2,mPeriod(a5)
	rts

mt_fineportaup
	tst.b	mt_counter
	bne	mt_return2
	move.b	#$f,mt_lowmask
mt_portaup
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	and.b	mt_lowmask(pc),d0
	move.b	#$ff,mt_lowmask
	sub	d0,n_period(a6)
	move	n_period(a6),d0
	and	#$fff,d0
	cmp	upplim(pc),d0
	bpl.b	mt_portauskip
	and	#$f000,n_period(a6)
	move	upplim(pc),d0
	or	d0,n_period(a6)
mt_portauskip
	move	n_period(a6),d0
	and	#$fff,d0
	lsl	#2,d0
	move	d0,mPeriod(a5)
	rts	
 
mt_fineportadown
	tst.b	mt_counter
	bne	mt_return2
	move.b	#$f,mt_lowmask
mt_portadown
	clr	d0
	move.b	n_cmdlo(a6),d0
	and.b	mt_lowmask(pc),d0
	move.b	#$ff,mt_lowmask
	add	d0,n_period(a6)
	move	n_period(a6),d0
	and	#$fff,d0
	cmp	lowlim(pc),d0
	bmi.b	mt_portadskip
	and	#$f000,n_period(a6)
	move	lowlim(pc),d0
	or	d0,n_period(a6)
mt_portadskip
	move	n_period(a6),d0
	and	#$fff,d0
	lsl	#2,d0
	move	d0,mPeriod(a5)
	rts

mt_settoneporta
	move.l	a0,-(sp)
	move	(a6),d2
	and	#$fff,d2
	moveq	#0,d0
	move.b	n_finetune(a6),d0
	mulu	octs(pc),d0
	move.l	peris(pc),a0
	add.l	d0,a0
	moveq	#0,d0
mt_stploop
	cmp	(a0,d0),d2
	bhs.b	mt_stpfound
	addq	#2,d0
	cmp	octs(pc),d0
	blo.b	mt_stploop
	move	octs(pc),d0
	subq	#2,d0
mt_stpfound
	move.b	n_finetune(a6),d2
	and.b	#8,d2
	beq.b	mt_stpgoss
	tst	d0
	beq.b	mt_stpgoss
	subq	#2,d0
mt_stpgoss
	move	(a0,d0),d2
	move.l	(sp)+,a0
	move	d2,n_wantedperiod(a6)
	move	n_period(a6),d0
	clr.b	n_toneportdirec(a6)
	cmp	d0,d2
	beq.b	mt_cleartoneporta
	bge	mt_return2
	move.b	#1,n_toneportdirec(a6)
	rts

mt_cleartoneporta
	clr	n_wantedperiod(a6)
	rts

mt_toneportamento
	move.b	n_cmdlo(a6),d0
	beq.b	mt_toneportnochange
	move.b	d0,n_toneportspeed(a6)
	clr.b	n_cmdlo(a6)
mt_toneportnochange
	tst	n_wantedperiod(a6)
	beq	mt_return2
	moveq	#0,d0
	move.b	n_toneportspeed(a6),d0
	tst.b	n_toneportdirec(a6)
	bne.b	mt_toneportaup
mt_toneportadown
	add	d0,n_period(a6)
	move	n_wantedperiod(a6),d0
	cmp	n_period(a6),d0
	bgt.b	mt_toneportasetper
	move	n_wantedperiod(a6),n_period(a6)
	clr	n_wantedperiod(a6)
	bra.b	mt_toneportasetper

mt_toneportaup
	sub	d0,n_period(a6)
	move	n_wantedperiod(a6),d0
	cmp	n_period(a6),d0
	blt.b	mt_toneportasetper
	move	n_wantedperiod(a6),n_period(a6)
	clr	n_wantedperiod(a6)

mt_toneportasetper
	move	n_period(a6),d2
	move.b	n_glissfunk(a6),d0
	and.b	#$f,d0
	beq.b	mt_glissskip
	moveq	#0,d0
	move.b	n_finetune(a6),d0
	mulu	octs(pc),d0
	move.l	peris(pc),a0
	add.l	d0,a0
	moveq	#0,d0
mt_glissloop
	cmp	(a0,d0),d2
	bhs.b	mt_glissfound
	addq	#2,d0
	cmp	octs(pc),d0
	blo.b	mt_glissloop
	move	octs(pc),d0
	subq	#2,d0
mt_glissfound
	move	(a0,d0),d2
mt_glissskip
	lsl	#2,d2
	move	d2,mPeriod(a5) ; set period
	rts

mt_vibrato
	move.b	n_cmdlo(a6),d0
	beq.b	mt_vibrato2
	move.b	n_vibratocmd(a6),d2
	and.b	#$f,d0
	beq.b	mt_vibskip
	and.b	#$f0,d2
	or.b	d0,d2
mt_vibskip
	move.b	n_cmdlo(a6),d0
	and.b	#$f0,d0
	beq.b	mt_vibskip2
	and.b	#$f,d2
	or.b	d0,d2
mt_vibskip2
	move.b	d2,n_vibratocmd(a6)
mt_vibrato2
	move.b	n_vibratopos(a6),d0
	lea	mt_vibratotable(pc),a4
	lsr	#2,d0
	and	#$1f,d0
	moveq	#0,d2
	move.b	n_wavecontrol(a6),d2
	and.b	#3,d2
	beq.b	mt_vib_sine
	lsl.b	#3,d0
	cmp.b	#1,d2
	beq.b	mt_vib_rampdown
	move.b	#255,d2
	bra.b	mt_vib_set
mt_vib_rampdown
	tst.b	n_vibratopos(a6)
	bpl.b	mt_vib_rampdown2
	move.b	#255,d2
	sub.b	d0,d2
	bra.b	mt_vib_set
mt_vib_rampdown2
	move.b	d0,d2
	bra.b	mt_vib_set
mt_vib_sine
	move.b	0(a4,d0),d2
mt_vib_set
	move.b	n_vibratocmd(a6),d0
	and	#15,d0
	mulu	d0,d2
	lsr	#7,d2
	move	n_period(a6),d0
	tst.b	n_vibratopos(a6)
	bmi.b	mt_vibratoneg
	add	d2,d0
	bra.b	mt_vibrato3
mt_vibratoneg
	sub	d2,d0
mt_vibrato3
	lsl	#2,d0
	move	d0,mPeriod(a5)
	move.b	n_vibratocmd(a6),d0
	lsr	#2,d0
	and	#$3c,d0
	add.b	d0,n_vibratopos(a6)
	rts

mt_toneplusvolslide
	bsr	mt_toneportnochange
	bra	mt_volumeslide

mt_vibratoplusvolslide
	bsr.b	mt_vibrato2
	bra	mt_volumeslide

mt_tremolo
	move.b	n_cmdlo(a6),d0
	beq.b	mt_tremolo2
	move.b	n_tremolocmd(a6),d2
	and.b	#$f,d0
	beq.b	mt_treskip
	and.b	#$f0,d2
	or.b	d0,d2
mt_treskip
	move.b	n_cmdlo(a6),d0
	and.b	#$f0,d0
	beq.b	mt_treskip2
	and.b	#$f,d2
	or.b	d0,d2
mt_treskip2
	move.b	d2,n_tremolocmd(a6)
mt_tremolo2
	move.b	n_tremolopos(a6),d0
	lea	mt_vibratotable(pc),a4
	lsr	#2,d0
	and	#$1f,d0
	moveq	#0,d2
	move.b	n_wavecontrol(a6),d2
	lsr.b	#4,d2
	and.b	#3,d2
	beq.b	mt_tre_sine
	lsl.b	#3,d0
	cmp.b	#1,d2
	beq.b	mt_tre_rampdown
	move.b	#255,d2
	bra.b	mt_tre_set
mt_tre_rampdown
	tst.b	n_vibratopos(a6)
	bpl.b	mt_tre_rampdown2
	move.b	#255,d2
	sub.b	d0,d2
	bra.b	mt_tre_set
mt_tre_rampdown2
	move.b	d0,d2
	bra.b	mt_tre_set
mt_tre_sine
	move.b	0(a4,d0),d2
mt_tre_set
	move.b	n_tremolocmd(a6),d0
	and	#15,d0
	mulu	d0,d2
	lsr	#6,d2
	moveq	#0,d0
	move.b	n_volume(a6),d0
	tst.b	n_tremolopos(a6)
	bmi.b	mt_tremoloneg
	add	d2,d0
	bra.b	mt_tremolo3
mt_tremoloneg
	sub	d2,d0
mt_tremolo3
	bpl.b	mt_tremoloskip
	clr	d0
mt_tremoloskip
	cmp	#$40,d0
	bls.b	mt_tremolook
	move	#$40,d0
mt_tremolook
	move	d0,mVolume(a5)
	move.b	n_tremolocmd(a6),d0
	lsr	#2,d0
	and	#$3c,d0
	add.b	d0,n_tremolopos(a6)
	rts

mt_sampleoffset
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	beq.b	mt_sononew
	move.b	d0,n_sampleoffset(a6)
mt_sononew
	move.b	n_sampleoffset(a6),d0
	lsl	#7,d0
	cmp	n_length(a6),d0
	bge.b	mt_sofskip
	sub	d0,n_length(a6)
	lsl	#1,d0
	add.l	d0,n_start(a6)
	rts
mt_sofskip
	move	#1,n_length(a6)
	rts

mt_volumeslide
	move.b	n_cmdlo(a6),d0
	lsr.b	#4,d0
	tst.b	d0
	beq.b	mt_volslidedown
mt_volslideup
	add.b	d0,n_volume(a6)
	cmp.b	#$40,n_volume(a6)
	bmi.b	mt_vsuskip
	move.b	#$40,n_volume(a6)
mt_vsuskip
	move.b	n_volume(a6),d0
	move.b	d0,mVolume+1(a5)
	rts

mt_volslidedown
	moveq	#$f,d0
	and.b	n_cmdlo(a6),d0
mt_volslidedown2
	sub.b	d0,n_volume(a6)
	bpl.b	mt_vsdskip
	clr.b	n_volume(a6)
mt_vsdskip
	move.b	n_volume(a6),d0
	move	d0,mVolume(a5)
	rts

mt_positionjump
	move.b	n_cmdlo(a6),d0
	cmp.b	mt_songpos(pc),d0
	bhi.b	.e
	st	PS3M_break

.e	subq.b	#1,d0
	move.b	d0,mt_songpos
mt_pj2	clr.b	mt_pbreakpos
	st 	mt_posjumpflag
	st	PS3M_poscha
	rts

mt_volumechange
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	cmp.b	#$40,d0
	bls.b	mt_volumeok
	moveq	#$40,d0
mt_volumeok
	move.b	d0,n_volume(a6)
	move	d0,mVolume(a5)
	rts

mt_patternbreak
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	move.l	d0,d2
	lsr.b	#4,d0
	mulu	#10,d0
	and.b	#$f,d2
	add.b	d2,d0
	cmp.b	#63,d0
	bhi.b	mt_pj2
	move.b	d0,mt_pbreakpos
	st	mt_posjumpflag
	st	PS3M_poscha
	rts

mt_setspeed
	moveq	#0,d0
	move.b	3(a6),d0
	bne.b	.e
	st	PS3M_break
	st	PS3M_poscha
	rts
.e	clr.b	mt_counter
	cmp	#32,d0
	bhs.b	mt_settempo
	move.b	d0,mt_speed
	rts

mt_settempo
	move.l	d1,-(sp)
	move.l	mrate,d1
	move.l	d1,d2
	lsl.l	#2,d1
	add.l	d2,d1
	add	d0,d0
	divu	d0,d1

	addq	#1,d1
	and	#~1,d1
	move	d1,bytesperframe
	move.l	(sp)+,d1
	rts

mt_checkmoreefx
	bsr	mt_updatefunk
	move.b	2(a6),d0
	and.b	#$f,d0
	cmp.b	#$9,d0
	beq	mt_sampleoffset
	cmp.b	#$b,d0
	beq	mt_positionjump
	cmp.b	#$d,d0
	beq	mt_patternbreak
	cmp.b	#$e,d0
	beq.b	mt_e_commands
	cmp.b	#$f,d0
	beq.b	mt_setspeed
	cmp.b	#$c,d0
	beq	mt_volumechange

	cmp	#mtMOD,mtype
	beq	mt_pernop

; MTM runs these also in set frames

	cmp.b	#1,d0
	beq	mt_portaup
	cmp.b	#2,d0
	beq	mt_portadown
	cmp.b	#3,d0
	beq	mt_toneportamento
	cmp.b	#4,d0
	beq	mt_vibrato
	cmp.b	#5,d0
	beq	mt_toneplusvolslide
	cmp.b	#6,d0
	beq	mt_vibratoplusvolslide
	bra	mt_pernop


mt_e_commands
	move.b	n_cmdlo(a6),d0
	and.b	#$f0,d0
	lsr.b	#4,d0
;	beq.b	mt_filteronoff
	cmp.b	#1,d0
	beq	mt_fineportaup
	cmp.b	#2,d0
	beq	mt_fineportadown
	cmp.b	#3,d0
	beq.b	mt_setglisscontrol
	cmp.b	#4,d0
	beq	mt_setvibratocontrol
	cmp.b	#5,d0
	beq	mt_setfinetune
	cmp.b	#6,d0
	beq	mt_jumploop
	cmp.b	#7,d0
	beq	mt_settremolocontrol
	cmp.b	#9,d0
	beq	mt_retrignote
	cmp.b	#$a,d0
	beq	mt_volumefineup
	cmp.b	#$b,d0
	beq	mt_volumefinedown
	cmp.b	#$c,d0
	beq	mt_notecut
	cmp.b	#$d,d0
	beq	mt_notedelay
	cmp.b	#$e,d0
	beq	mt_patterndelay
	cmp.b	#$f,d0
	beq	mt_funkit
	rts

mt_filteronoff
	move.b	n_cmdlo(a6),d0
	and.b	#1,d0
	asl.b	#1,d0
	and.b	#$fd,$bfe001
	or.b	d0,$bfe001
	rts	

mt_setglisscontrol
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	and.b	#$f0,n_glissfunk(a6)
	or.b	d0,n_glissfunk(a6)
	rts

mt_setvibratocontrol
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	and.b	#$f0,n_wavecontrol(a6)
	or.b	d0,n_wavecontrol(a6)
	rts

mt_setfinetune
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	move.b	d0,n_finetune(a6)
	rts

mt_jumploop
	tst.b	mt_counter
	bne	mt_return2
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	beq.b	mt_setloop
	tst.b	n_loopcount(a6)
	beq.b	mt_jumpcnt
	subq.b	#1,n_loopcount(a6)
	beq	mt_return2
mt_jmploop
	move.b	n_pattpos(a6),mt_pbreakpos
	st	mt_pbreakflag
	rts

mt_jumpcnt
	move.b	d0,n_loopcount(a6)
	bra.b	mt_jmploop

mt_setloop
	move	mt_patternpos(pc),d0
	lsr	#4,d0
	move.b	d0,n_pattpos(a6)
	rts

mt_settremolocontrol
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	lsl.b	#4,d0
	and.b	#$f,n_wavecontrol(a6)
	or.b	d0,n_wavecontrol(a6)
	rts

mt_retrignote
	move.l	d1,-(sp)
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	beq.b	mt_rtnend
	moveq	#0,d1
	move.b	mt_counter(pc),d1
	bne.b	mt_rtnskp
	move	(a6),d1
	and	#$fff,d1
	bne.b	mt_rtnend
	moveq	#0,d1
	move.b	mt_counter(pc),d1
mt_rtnskp
	divu	d0,d1
	swap	d1
	tst	d1
	bne.b	mt_rtnend
mt_doretrig
	move.l	n_start(a6),(a5)	; set start
	moveq	#0,d1
	move	n_length(a6),d1
	add.l	d1,d1
	move.l	d1,mLength(a5)		; set length
	clr.b	mOnOff(a5)		; turn on
	clr.l	mFPos(a5)		; retrig

	move.l	n_loopstart(a6),mLStart(a5)
	moveq	#0,d1
	move	n_replen(a6),d1
	add.l	d1,d1
	move.l	d1,mLLength(a5)
	cmp.l	#2,mLLength(a5)
	bls.b	.eloo
	st	mLoop(a5)
	move.l	(sp)+,d1
	rts
.eloo	clr.b	mLoop(a5)

mt_rtnend
	move.l	(sp)+,d1
	rts

mt_volumefineup
	tst.b	mt_counter
	bne	mt_return2
	moveq	#$f,d0
	and.b	n_cmdlo(a6),d0
	bra	mt_volslideup

mt_volumefinedown
	tst.b	mt_counter
	bne	mt_return2
	moveq	#0,d0
	move.b	n_cmdlo(a6),d0
	and.b	#$f,d0
	bra	mt_volslidedown2

mt_notecut
	moveq	#$f,d0
	and.b	n_cmdlo(a6),d0
	cmp.b	mt_counter(pc),d0
	bne	mt_return2
	clr.b	n_volume(a6)
	clr	mVolume(a5)
	rts

mt_notedelay
	moveq	#$f,d0
	and.b	n_cmdlo(a6),d0
	cmp.b	mt_counter(pc),d0
	bne	mt_return2
	move	(a6),d0
	beq	mt_return2

	move	n_period(a6),d0
	lsl	#2,d0
	move	d0,mPeriod(a5)		; set period
	move.l	d1,-(sp)
	bra	mt_doretrig

mt_patterndelay
	tst.b	mt_counter
	bne	mt_return2
	moveq	#$f,d0
	and.b	n_cmdlo(a6),d0
	tst.b	mt_pattdeltime2
	bne	mt_return2
	addq.b	#1,d0
	move.b	d0,mt_pattdeltime
	rts

mt_funkit
	tst.b	mt_counter
	bne	mt_return2
	move.b	n_cmdlo(a6),d0
	lsl.b	#4,d0
	and.b	#$f,n_glissfunk(a6)
	or.b	d0,n_glissfunk(a6)
	tst.b	d0
	beq	mt_return2
mt_updatefunk
	movem.l	a0/d1,-(sp)
	moveq	#0,d0
	move.b	n_glissfunk(a6),d0
	lsr.b	#4,d0
	beq.b	mt_funkend
	lea	mt_funktable(pc),a0
	move.b	(a0,d0),d0
	add.b	d0,n_funkoffset(a6)
	btst	#7,n_funkoffset(a6)
	beq.b	mt_funkend
	clr.b	n_funkoffset(a6)

	move.l	n_loopstart(a6),d0
	moveq	#0,d1
	move	n_replen(a6),d1
	add.l	d1,d0
	add.l	d1,d0
	move.l	n_wavestart(a6),a0
	addq.l	#1,a0
	cmp.l	d0,a0
	blo.b	mt_funkok
	move.l	n_loopstart(a6),a0
mt_funkok
	move.l	a0,n_wavestart(a6)
	moveq	#-1,d0
	sub.b	(a0),d0
	move.b	d0,(a0)
mt_funkend
	movem.l	(sp)+,a0/d1
	rts


mt_funktable dc.b 0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128

mt_vibratotable	
	dc.b   0, 24, 49, 74, 97,120,141,161
	dc.b 180,197,212,224,235,244,250,253
	dc.b 255,253,250,244,235,224,212,197
	dc.b 180,161,141,120, 97, 74, 49, 24

mt_periodtable
; tuning 0, normal
	dc	856,808,762,720,678,640,604,570,538,508,480,453
	dc	428,404,381,360,339,320,302,285,269,254,240,226
	dc	214,202,190,180,170,160,151,143,135,127,120,113
; tuning 1
	dc	850,802,757,715,674,637,601,567,535,505,477,450
	dc	425,401,379,357,337,318,300,284,268,253,239,225
	dc	213,201,189,179,169,159,150,142,134,126,119,113
; tuning 2
	dc	844,796,752,709,670,632,597,563,532,502,474,447
	dc	422,398,376,355,335,316,298,282,266,251,237,224
	dc	211,199,188,177,167,158,149,141,133,125,118,112
; tuning 3
	dc	838,791,746,704,665,628,592,559,528,498,470,444
	dc	419,395,373,352,332,314,296,280,264,249,235,222
	dc	209,198,187,176,166,157,148,140,132,125,118,111
; tuning 4
	dc	832,785,741,699,660,623,588,555,524,495,467,441
	dc	416,392,370,350,330,312,294,278,262,247,233,220
	dc	208,196,185,175,165,156,147,139,131,124,117,110
; tuning 5
	dc	826,779,736,694,655,619,584,551,520,491,463,437
	dc	413,390,368,347,328,309,292,276,260,245,232,219
	dc	206,195,184,174,164,155,146,138,130,123,116,109
; tuning 6
	dc	820,774,730,689,651,614,580,547,516,487,460,434
	dc	410,387,365,345,325,307,290,274,258,244,230,217
	dc	205,193,183,172,163,154,145,137,129,122,115,109
; tuning 7
	dc	814,768,725,684,646,610,575,543,513,484,457,431
	dc	407,384,363,342,323,305,288,272,256,242,228,216
	dc	204,192,181,171,161,152,144,136,128,121,114,108
; tuning -8
	dc	907,856,808,762,720,678,640,604,570,538,508,480
	dc	453,428,404,381,360,339,320,302,285,269,254,240
	dc	226,214,202,190,180,170,160,151,143,135,127,120
; tuning -7
	dc	900,850,802,757,715,675,636,601,567,535,505,477
	dc	450,425,401,379,357,337,318,300,284,268,253,238
	dc	225,212,200,189,179,169,159,150,142,134,126,119
; tuning -6
	dc	894,844,796,752,709,670,632,597,563,532,502,474
	dc	447,422,398,376,355,335,316,298,282,266,251,237
	dc	223,211,199,188,177,167,158,149,141,133,125,118
; tuning -5
	dc	887,838,791,746,704,665,628,592,559,528,498,470
	dc	444,419,395,373,352,332,314,296,280,264,249,235
	dc	222,209,198,187,176,166,157,148,140,132,125,118
; tuning -4
	dc	881,832,785,741,699,660,623,588,555,524,494,467
	dc	441,416,392,370,350,330,312,294,278,262,247,233
	dc	220,208,196,185,175,165,156,147,139,131,123,117
; tuning -3
	dc	875,826,779,736,694,655,619,584,551,520,491,463
	dc	437,413,390,368,347,328,309,292,276,260,245,232
	dc	219,206,195,184,174,164,155,146,138,130,123,116
; tuning -2
	dc	868,820,774,730,689,651,614,580,547,516,487,460
	dc	434,410,387,365,345,325,307,290,274,258,244,230
	dc	217,205,193,183,172,163,154,145,137,129,122,115
; tuning -1
	dc	862,814,768,725,684,646,610,575,543,513,484,457
	dc	431,407,384,363,342,323,305,288,272,256,242,228
	dc	216,203,192,181,171,161,152,144,136,128,121,114


mtm_periodtable
; Tuning 0, Normal
	dc	1616,1524,1440,1356,1280,1208,1140,1076,1016,960,907
	dc	856,808,762,720,678,640,604,570,538,508,480,453
	dc	428,404,381,360,339,320,302,285,269,254,240,226
	dc	214,202,190,180,170,160,151,143,135,127,120,113
	dc	107,101,95,90,85,80,75,71,67,63,60,56
	dc	53,50,48,45

; Tuning 1
	dc	1604,1514,1430,1348,1274,1202,1134,1070,1010,954,900
	dc	850,802,757,715,674,637,601,567,535,505,477,450
	dc	425,401,379,357,337,318,300,284,268,253,239,225
	dc	213,201,189,179,169,159,150,142,134,126,119,113
	dc	106,100,94,89,84,80,75,71,67,63,59,56
	dc	53,50,47,45

; Tuning 2
	dc	1592,1504,1418,1340,1264,1194,1126,1064,1004,948,894
	dc	844,796,752,709,670,632,597,563,532,502,474,447
	dc	422,398,376,355,335,316,298,282,266,251,237,224
	dc	211,199,188,177,167,158,149,141,133,125,118,112
	dc	105,99,94,88,83,79,74,70,66,62,59,56
	dc	53,50,47,44

; Tuning 3
	dc	1582,1492,1408,1330,1256,1184,1118,1056,996,940,888
	dc	838,791,746,704,665,628,592,559,528,498,470,444
	dc	419,395,373,352,332,314,296,280,264,249,235,222
	dc	209,198,187,176,166,157,148,140,132,125,118,111
	dc	104,99,93,88,83,78,74,70,66,62,59,55
	dc	52,49,47,44

; Tuning 4
	dc	1570,1482,1398,1320,1246,1176,1110,1048,990,934,882
	dc	832,785,741,699,660,623,588,555,524,495,467,441
	dc	416,392,370,350,330,312,294,278,262,247,233,220
	dc	208,196,185,175,165,156,147,139,131,124,117,110
	dc	104,98,92,87,82,78,73,69,65,62,58,55
	dc	52,49,46,44

; Tuning 5
	dc	1558,1472,1388,1310,1238,1168,1102,1040,982,926,874
	dc	826,779,736,694,655,619,584,551,520,491,463,437
	dc	413,390,368,347,328,309,292,276,260,245,232,219
	dc	206,195,184,174,164,155,146,138,130,123,116,109
	dc	103,97,92,87,82,77,73,69,65,61,58,54
	dc	52,49,46,43

; Tuning 6
	dc	1548,1460,1378,1302,1228,1160,1094,1032,974,920,868
	dc	820,774,730,689,651,614,580,547,516,487,460,434
	dc	410,387,365,345,325,307,290,274,258,244,230,217
	dc	205,193,183,172,163,154,145,137,129,122,115,109
	dc	102,97,91,86,81,77,72,68,64,61,57,54
	dc	51,48,46,43

; Tuning 7
	dc	1536,1450,1368,1292,1220,1150,1086,1026,968,914,862
	dc	814,768,725,684,646,610,575,543,513,484,457,431
	dc	407,384,363,342,323,305,288,272,256,242,228,216
	dc	204,192,181,171,161,152,144,136,128,121,114,108
	dc	102,96,91,85,81,76,72,68,64,60,57,54
	dc	51,48,45,43

; Tuning -8
	dc	1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960
	dc	907,856,808,762,720,678,640,604,570,538,508,480
	dc	453,428,404,381,360,339,320,302,285,269,254,240
	dc	226,214,202,190,180,170,160,151,143,135,127,120
	dc	113,107,101,95,90,85,80,75,71,67,63,60
	dc	56,53,50,48

; Tuning -7
	dc	1700,1604,1514,1430,1350,1272,1202,1134,1070,1010,954
	dc	900,850,802,757,715,675,636,601,567,535,505,477
	dc	450,425,401,379,357,337,318,300,284,268,253,238
	dc	225,212,200,189,179,169,159,150,142,134,126,119
	dc	112,106,100,94,89,84,79,75,71,67,63,60
	dc	56,53,50,47

; Tuning -6
	dc	1688,1592,1504,1418,1340,1264,1194,1126,1064,1004,948
	dc	894,844,796,752,709,670,632,597,563,532,502,474
	dc	447,422,398,376,355,335,316,298,282,266,251,237
	dc	223,211,199,188,177,167,158,149,141,133,125,118
	dc	112,105,99,94,89,84,79,75,70,66,63,59
	dc	56,53,50,47

; Tuning -5
	dc	1676,1582,1492,1408,1330,1256,1184,1118,1056,996,940
	dc	887,838,791,746,704,665,628,592,559,528,498,470
	dc	444,419,395,373,352,332,314,296,280,264,249,235
	dc	222,209,198,187,176,166,157,148,140,132,125,118
	dc	111,105,99,93,88,83,78,74,70,66,62,59
	dc	55,52,49,47

; Tuning -4
	dc	1664,1570,1482,1398,1320,1246,1176,1110,1048,988,934
	dc	881,832,785,741,699,660,623,588,555,524,494,467
	dc	441,416,392,370,350,330,312,294,278,262,247,233
	dc	220,208,196,185,175,165,156,147,139,131,123,117
	dc	110,104,98,93,87,82,78,73,69,65,62,58
	dc	55,52,49,46

; Tuning -3
	dc	1652,1558,1472,1388,1310,1238,1168,1102,1040,982,926
	dc	875,826,779,736,694,655,619,584,551,520,491,463
	dc	437,413,390,368,347,328,309,292,276,260,245,232
	dc	219,206,195,184,174,164,155,146,138,130,123,116
	dc	109,103,97,92,87,82,77,73,69,65,61,58
	dc	55,52,49,46

; Tuning -2
	dc	1640,1548,1460,1378,1302,1228,1160,1094,1032,974,920
	dc	868,820,774,730,689,651,614,580,547,516,487,460
	dc	434,410,387,365,345,325,307,290,274,258,244,230
	dc	217,205,193,183,172,163,154,145,137,129,122,115
	dc	108,102,97,91,86,81,77,72,68,64,61,57
	dc	54,51,48,46

; Tuning -1
	dc	1628,1536,1450,1368,1292,1220,1150,1086,1026,968,914
	dc	862,814,768,725,684,646,610,575,543,513,484,457
	dc	431,407,384,363,342,323,305,288,272,256,242,228
	dc	216,203,192,181,171,161,152,144,136,128,121,114
	dc	108,102,96,91,85,81,76,72,68,64,60,57
	dc	54,51,48,45

mt_chan1temp	ds.b	44*32

mt_sampleinfos
	ds	31*12

mt_songdataptr	dc.l 0

mt_speed	dc.b 6
mt_tempo	dc.b 0
mt_counter	dc.b 0
mt_songpos	dc.b 0
mt_pbreakpos	dc.b 0
mt_posjumpflag	dc.b 0
mt_pbreakflag	dc.b 0
mt_lowmask	dc.b 0
mt_pattdeltime	dc.b 0
mt_pattdeltime2	dc.b 0

mt_patternpos	dc 0
mt_dmacontemp	dc 0
