
modtypecpy:
	 movem.l d0/a0/a1,-(sp)

	 lea.l MName,a1			;copy Module Name
	 moveq	#19,d1
	 bsr	strncpy

	 lea.l	FNametype,a1		; copy Module Type
	 moveq	#30,d1
	 lea.l	MODName,a0
	 move.l	d0,MODtag
	 bsr	strncpy
	 movem.l (sp)+,d0/a0/a1
	rts

s3mtypecpy:
	 movem.l d0/a0/a1,-(sp)

	 lea.l MName,a1			;copy Module Name
	 moveq	#$1a,d1
	 bsr	strncpy

	 lea.l	FNametype,a1		; copy Module Type
	 moveq	#30,d1
	 lea.l	S3MName,a0
	 bsr	strncpy
	 movem.l (sp)+,d0/a0/a1
	rts

mtmtypecpy:
	 movem.l d0/a0/a1,-(sp)

	 addq	#4,a0
	 lea.l MName,a1			;copy Module Name
	 moveq	#$13,d1
	 bsr	strncpy

	 lea.l	FNametype,a1		; copy Module Type
	 moveq	#30,d1
	 lea.l	MTMName,a0
	 bsr	strncpy
	 movem.l (sp)+,d0/a0/a1
	rts

xmtypecpy:
	 movem.l d0/a0/a1/a2,-(sp)
	 move.l	a0,a2

	 add.w	#$11,a0
	 lea.l MName,a1			;copy Module Name
	 moveq	#19,d1
	 bsr	strncpy

	 lea.l	FNametype,a1		; copy Module Type
	 moveq	#30,d1
	 move.l	a2,a0
	 add.w	#$26,a0
	 bsr	strncpy
	 movem.l (sp)+,d0/a0/a1/a2
	rts


**
* a0=src	(zero terminated)
* a1=dst
* d1=maxlen
**

strncpy:
    move.b (a0)+,(a1)+
    dbra d1,strncpy    
    rts

XMName	ds.b	$1a
	dc.b	0
S3MName	dc.b	'Screamtracker 3.x (S3M)',0
MTMName	dc.b	'Multitracker (MTM)',0
MODName	dc.b	'Multichannel MOD ('
MODtag	dc.b	'####)',0

    even

init	lea	data,a5
	clr	mtype(a5)

	move.l	a0,s3m(a5)
	cmp.l	#'SCRM',44(a0)
	beq	.s3m

	move.l	(a0),d0
	lsr.l	#8,d0
	cmp.l	#'MTM',d0
	beq	.mtm

	move.l	a0,a1
	lea	xmsign(a5),a2
	moveq	#3,d0
.ll	cmpm.l	(a1)+,(a2)+
	bne.b	.jj
	dbf	d0,.ll
	bra	.xm

.jj	move.l	1080(a0),d0
	cmp.l	#'OCTA',d0
	beq	.fast8
	cmp.l	#'CD81',d0
	beq	.fast8
	cmp.l	#'M.K.',d0
	beq	.pro4
	cmp.l	#'M!K!',d0
	beq	.pro4
	cmp.l	#'FLT4',d0
	beq	.pro4

	move.l	d0,d1
	and.l	#$ffffff,d1
	cmp.l	#'CHN',d1
	beq.b	.chn

	and.l	#$ffff,d1
	cmp.l	#'CH',d1
	beq.b	.ch

	move.l	d0,d1
	and.l	#$ffffff00,d1
	cmp.l	#'TDZ'<<8,d1
	beq.b	.tdz
	bra	.error

.chn	bsr modtypecpy
	move.l	d0,d1
	swap	d1
	lsr	#8,d1
	sub	#'0',d1
	move	#mtMOD,mtype(a5)
	move	d1,numchans(a5)
	addq	#1,d1
	lsr	d1
	move	d1,maxchan(a5)
	bra	.init

.ch	bsr modtypecpy
	move.l	d0,d1
	swap	d1
	sub	#'00',d1
	move	d1,d0
	lsr	#8,d0
	mulu	#10,d0
	and	#$f,d1
	add	d0,d1

	move	#mtMOD,mtype(a5)
	move	d1,numchans(a5)
	addq	#1,d1
	lsr	d1
	move	d1,maxchan(a5)
	bra.b	.init

.tdz	bsr modtypecpy
	and.l	#$ff,d0
	sub	#'0',d0
	move	#mtMOD,mtype(a5)
	move	d0,numchans(a5)
	addq	#1,d0
	lsr	d0
	move	d0,maxchan(a5)
	bra.b	.init

.fast8	move	#mtMOD,mtype(a5)
	move	#8,numchans(a5)
	move	#4,maxchan(a5)
	bsr modtypecpy
	bra.b	.init

.pro4	move	#mtMOD,mtype(a5)
	move	#4,numchans(a5)
	move	#2,maxchan(a5)
	bsr modtypecpy
	bra.b	.init

.mtm	move	#mtMTM,mtype(a5)
	bsr	mtmtypecpy
	bra.b	.init

.xm	cmp	#$401,xmVersion(a0)		; Kool turbo-optimizin'...
	bne	.jj
	move	#mtXM,mtype(a5)
	bsr	xmtypecpy
	bra.b	.init

.s3m	move	#mtS3M,mtype(a5)
	bsr	s3mtypecpy

.init

; TEMPORARY BUGFIX...

	cmp	#2,maxchan(a5)
	bhs.b	.opk

	move	#2,maxchan(a5)

.opk	tst	mtype(a5)
	beq.b	.error

	cmp	#mtS3M,mtype(a5)
	beq	s3m_init

	cmp	#mtMOD,mtype(a5)
	beq	mt_init

	cmp	#mtMTM,mtype(a5)
	beq	mtm_init

	cmp	#mtXM,mtype(a5)
	beq	xm_init

.error	moveq	#1,d0
	rts
