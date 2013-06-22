mblockf	macro				;a0=source, a1=destination
	movem.l	d0-d1/a0-a1,-(a7)	;d0=number of bytes
	move.l	\1,a0
	move.l	\2,a1
	move.l	\3,d0
	move.l	d0,d1
	lsr.l	#2,d0
ltr\@	move.l	(a0)+,(a1)+
	subq.l	#1,d0
	bne.b	ltr\@
	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs\@
ybs\@	move.b	(a0)+,(a1)+
	dbf	d1,ybs\@
nobs\@	movem.l	(a7)+,d0-d1/a0-a1
	endm

mblockr	macro				;a0=source, a1=destination
	movem.l	d0-d1/a0-a1,-(a7)	;d0=number of bytes
	move.l	\1,a0
	move.l	\2,a1
	move.l	\3,d0
	add.l	d0,a0
	add.l	d0,a1
	move.l	d0,d1
	lsr.l	#2,d0
ltr\@	move.l	-(a0),-(a1)
	subq.l	#1,d0
	bne.b	ltr\@
	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs\@
ybs\@	move.b	-(a0),-(a1)
	dbf	d1,ybs\@
nobs\@	movem.l	(a7)+,d0-d1/a0-a1
	endm

cmblock	macro				;a0=address
	movem.l	d0-d2/a0,-(a7)		;d0=number of bytes
	move.l	\1,a0
	move.l	\2,d0
	moveq	#0,d2
	move.l	d0,d1
	lsr.l	#2,d0
ltr\@	move.l	d2,(a0)+
	subq.l	#1,d0
	bne.b	ltr\@
	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs\@
ybs\@	move.b	d0,(a0)+
	dbf	d1,ybs\@
nobs\@	movem.l	(a7)+,d0-d2/a0
	endm
