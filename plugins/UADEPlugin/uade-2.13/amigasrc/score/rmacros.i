pull	macro
	ifc	"\1","all"
	movem.l	(a7)+,d0-d7/a0-a6
	endc
	ifnc	"\1","all"
	movem.l	(a7)+,\1
	endc
	endm
push	macro
	ifc	"\1","all"
	movem.l	d0-d7/a0-a6,-(a7)
	endc
	ifnc	"\1","all"
	movem.l	\1,-(a7)
	endc
	endm

pullw	macro
	ifc	"\1","all"
	movem	(a7)+,d0-d7/a0-a6
	endc
	ifnc	"\1","all"
	movem	(a7)+,\1
	endc
	endm
pushw	macro
	ifc	"\1","all"
	movem	d0-d7/a0-a6,-(a7)
	endc
	ifnc	"\1","all"
	movem	\1,-(a7)
	endc
	endm

pushr	macro
	move.l	\1,-(a7)
	endm
pullr	macro
	move.l	(a7)+,\1
	endm

openlib	macro
	move.l	4.w,a6
	lea	\1name,a1
	call	OpenLibrary
	move.l	d0,\1base
	tst.l	d0
	endm

closelib	macro
	move.l	4.w,a6
	move.l	\1base,a1
	call	CloseLibrary
	endm

bwait	macro
\@	btst	#7,2(a2)
	bne.b	\@
	endm
bwait2	macro
\@	btst	#7,$dff002
	bne.b	\@
	endm

waitr	macro
\@	cmp.b	#\1,$dff006
	bne.b	\@
	endm

waitr2	macro
\@	cmp.b	#\1,6(a2)
	bne.b	\@
	endm

waitlb	macro
\@	btst	#6,$bfe001
	bne.b	\@
	endm
waitrb	macro
\@	btst	#$a,$dff016
	bne.b	\@
	endm

waitframes	macro
	move.l	d7,-(a7)
	move	#\1-1,d7
	bmi.b	\@3
\@	cmp.b	#$ff,$dff006
	bne.b	\@
\@2	cmp.b	#$fe,$dff006
	bne.b	\@2
\@3	move.l	(a7)+,d7
	endm

hex2dec	macro		;usage=hex2dex dx
	ifc	"\1","d1"
	printt	"Impossible hex2dec conversion !!!!"
	endc
	ifc	"\1","D1"
	printt	"Impossible hex2dec conversion !!!!"
	endc
	movem.l	d1-d2,-(a7)
	move.l	\1,d1	;dx contains hexdecimal number
	moveq	#0,\1	;which will be converter in decimal
			;used regs d1/d2 (stack'em!)
h2d5\@	move.l	d1,d2
	sub.l	#100000,d1
	bmi.b	h2d4\@
	add.l	#$100000,\1
	bra.b	h2d5\@
h2d4\@	move.l	d2,d1
	sub.l	#10000,d2
	bmi.b	h2d3\@
	add.l	#$10000,\1
	bra.b	h2d4\@
h2d3\@	move.l	d1,d2
	sub.l	#$3e8,d1
	bmi.b	h2d2\@
	add.l	#$1000,\1
	bra.b	h2d3\@
h2d2\@	move.l	d2,d1
	sub.l	#$64,d2
	bmi.b	h2d1\@
	add.l	#$100,\1
	bra.b	h2d2\@
h2d1\@	move.l	d1,d2
	sub.l	#$a,d1
	bmi.b	h2d0\@
	add.l	#$10,\1
	bra.b	h2d1\@
h2d0\@	and.l	#$f,d2
	add.l	d2,\1
	movem.l	(a7)+,d1-d2
	endm

dec2hex	macro
	push	d1-d3/d7
	move.l	\1,d1
	moveq	#0,d0
	move.l	#$10000000,d2
	move.l	#10000000,d3
	moveq	#8-1,d7
d2hloop\@
d2hh3\@	sub.l	d2,d1
	bmi.b	d2hh2\@
	add.l	d3,d0
	bra.b	d2hh3\@
d2hh2\@	add.l	d2,d1
	lsr.l	#4,d2
	divu.l	#10,d3
	dbf	d7,d2hloop\@
	pull	d1-d3/d7
	endm

dec2hex2	macro
	push	d1-d3/d7
	move.l	\1,d1
	moveq	#0,d0
	move.l	#$100000,d2
	move.l	#100000,d3
	moveq	#6-1,d7
d2hloop\@
d2hh3\@	sub.l	d2,d1
	bmi.b	d2hh2\@
	add.l	d3,d0
	bra.b	d2hh3\@
d2hh2\@	add.l	d2,d1
	lsr.l	#4,d2
	divu	#10,d3
	dbf	d7,d2hloop\@
	pull	d1-d3/d7
	endm

cache_off	macro
	push	all
	move.l	4.w,a6
	move.l	#$3818,d0
	move.l	#$3b1b,d1
	jsr	-648(a6)
	move.l	d0,_sholdcache
	pull	all
	endm
cache_on	macro
	push	all
	move.l	4.w,a6
	move.l	_sholdcache,d0
	moveq	#-1,d1
	jsr	-648(a6)
	pull	all
	endm

call	macro
	jsr	\1(a6)
	endm

ciaresetti	macro
	bclr	#3,$bfde00
	endm

cmquick	macro
	push	d0-d1/a0-a1
	moveq	#120,d1
	cmp.l	d1,d0
	bcs.b	lab_0003\@
	push	d2-d7/a2-a6
lab_0002\@	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,(a1)
	moveq	#48,d1
	adda.l	d1,a1
	sub.l	d1,d0
	cmp.l	d1,d0
	bcc.b	lab_0002\@
	pull	d2-d7/a2-a6
lab_0003\@	lsr.l	#2,d0
	beq.b	lab_0009\@
	subq.l	#1,d0
	move.l	d0,d1
	swap	d0
lab_0004\@	move.l	(a0)+,(a1)+
	dbf	d1,lab_0004\@
	dbf	d0,lab_0004\@
lab_0009\@	pull	d0-d1/a0-a1
	endm

absregw	macro
	tst	\1
	bpl.b	nneg\@
	neg	\1
nneg\@
	endm

absregl	macro
	tst.l	\1
	bpl.b	nneg\@
	neg.l	\1
nneg\@
	endm

waitr3	macro
fwait\@	move.l	4(a2),d0
	and.l	#$1ff00,d0
	cmp.l	#\1,d0
	bne.b	fwait\@
	endm

waitr4	macro
fwait\@	move.l	$dff004,d0
	and.l	#$1ff00,d0
	cmp.l	#\1,d0
	bne.b	fwait\@
	endm

getmem	MACRO			* ptr to base, size, reguirements, fail
	move.l	#\2,d0
	move.l	#\3,d1
	move.l	4.w,a6
 	call	AllocMem
	move.l	d0,\1
	beq	\4
	ENDM

freeit	MACRO			* ptr to start, size
	move.l	\1,a1
	move.l	#\2,d0
	move.l	4.w,a6
	call	FreeMem
	ENDM
