
	;	AUTO	j\wb\a0\a1\wb\a2\a3\

size=128
		lea	size_struct(pc),a0
		move.l	a0,a1
		moveq	#1,d0		;önskat värde
		move	#size,d2	;max värde

calc_again	moveq	#1,d1		;räknare
		move	d0,d7
		subq	#1,d7

calc_loop	move	d1,d3
		mulu	d2,d3
		divu	d0,d3
		subq	#1,d3
		move.b	d3,(a1)+
		addq	#1,d1
		dbf	d7,calc_loop
		addq	#1,d0
		cmp	#size,d0
		blo.s	calc_again

		lea	size_offset,a2
		move.l	a2,a3

		moveq	#0,d0
		moveq	#0,d1
		move	#size-2,d7

offset_loop	add	d0,d1
		move	d1,(a3)+
		addq	#1,d0
		dbf	d7,offset_loop

		moveq	#0,d0
		rts

size_struct	ds.b	50000
size_offset	ds.w	size-1
