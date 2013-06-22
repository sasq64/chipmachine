ep_SetCIASpeed:
	movem.l	d0-d2/a0-a6,-(sp)
	move.l	delibase(pc),a5
	move.w	d0,dtg_Timer(a5)
	move.l	dtg_SetTimer(a5),a1
	jsr	(a1)
	movem.l	(sp)+,d0-d2/a0-a6
	rts

ep_Songend:
	movem.l	d0-d7/a0-a6,-(a7)
	move.l  delibase(pc),a5
	move.l	dtg_SongEnd(a5),a1
	jsr (a1)
	movem.l	(a7)+,d0-d7/a0-a6
	rts

ep_Songendflag	dc.b 0
	even
