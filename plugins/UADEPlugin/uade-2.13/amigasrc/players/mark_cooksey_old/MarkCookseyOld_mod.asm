;               T

	incdir	asm:
	include	bincs.i

dtg_AudioAlloc	EQU	$4C
DTP_Check2	EQU	$8000445C
DTP_InitSound	EQU	$80004465
DTP_EndSound	EQU	$80004466
DTP_Interrupt	EQU	$8000445E
DTP_NextSong	EQU	$8000446F
dtg_AudioFree	EQU	$50
DTP_PlayerVersion	EQU	$80004458
DTP_PlayerName	EQU	$80004459
DTP_EndPlayer	EQU	$80004464
DTP_PrevSong	EQU	$80004470
dtg_ChkSize	EQU	$28
dtg_ChkData	EQU	$24
DTP_Creator	EQU	$8000445A
DTP_InitPlayer	EQU	$80004463
dtg_SongEnd	equ	$5C
****************************************************************************

	SECTION	MarkCookseyOld000000,CODE
ProgStart	MOVEQ	#-1,D0
	RTS

	dc.b	'DELIRIUM'
	dc.l	lbL000010
lbL000010	dc.l	DTP_PlayerVersion
	dc.l	1
	dc.l	DTP_PlayerName
	dc.l	MarkCookseyOl.MSG
	dc.l	DTP_Creator
	dc.l	MarkCookseyad.MSG
	dc.l	DTP_Check2
	dc.l	check2
	dc.l	DTP_InitPlayer
	dc.l	initplayer
	dc.l	DTP_InitSound
	dc.l	initsound
	dc.l	DTP_Interrupt
	dc.l	interrupt
	dc.l	DTP_EndSound
	dc.l	endsound
	dc.l	DTP_EndPlayer
	dc.l	endplayer
	dc.l	DTP_NextSong
	dc.l	nextsong
	dc.l	DTP_PrevSong
	dc.l	prevsong
	dc.l	$80004473,eaglebase
	dc.l	$80004474,2		* sucky song end support
	dc.l	0
MarkCookseyOl.MSG	dc.b	'MarkCookseyOld',0
MarkCookseyad.MSG	dc.b	'Mark Cooksey,',$A
	dc.b	'adapted by Andy Silva',10
	dc.b	'dma wait and song end changes by shd',0

check2	MOVEQ	#-1,D0
	MOVE.L	(dtg_ChkData,A5),A0
	CMP.B	#$60,(A0)
	BNE.B	lbC0000F8
	MOVE.L	(dtg_ChkSize,A5),D1
	MOVE.L	(2,A0),D2
	CLR.B	D1
	CLR.B	D2
	CMP.L	D1,D2
	BNE.B	lbC0000F8
	TST.B	($20,A0)
	BNE.B	lbC0000F8
	LEA	($1C,A0),A0
	MOVE.L	A0,(moduleptr_off_1C)
	MOVEQ	#0,D0
lbC0000F8	RTS

initplayer	CLR.W	(subsong)
	MOVE.L	(dtg_AudioAlloc,A5),A0
	JSR	(A0)
	TST.L	D0
	RTS

endplayer	MOVE.L	(dtg_AudioFree,A5),A0
	JSR	(A0)
	RTS

shortdmawait	push	d0-d1
	moveq	#5-1,d1
	bra.b	dmawaitloop_1

dmawait	push	d0-d1
	moveq	#10-1,d1
dmawaitloop_1	move.b	$dff006,d0
dmawaitloop_2	cmp.b	$dff006,d0
	beq.b	dmawaitloop_2
	dbf	d1,dmawaitloop_1
	pull	d0-d1
	rts

report_song_end	push	all
	move.l	eaglebase(pc),a5
	move.l	dtg_SongEnd(a5),a0
	jsr	(a0)
	pull	all
	rts

eaglebase	dc.l	0
moduleptr_off_1C	dc.l	0
subsong	dc.w	0
maxsubsong	dc.w	0
PlayBit_11A	dc.w	0

initsound	MOVEM.L	D1-D7/A0-A6,-(SP)
	CLR.W	(PlayBit_11A)
	MOVE.L	(moduleptr_off_1C,PC),A0
	MOVE.L	(8,A0),D0
	SUBQ.L	#8,D0
	LSR.W	#4,D0
	SUBQ.W	#1,D0
	MOVE.W	D0,(maxsubsong)
	MOVE.W	(subsong,PC),D0
	BSR.W	setsubsong_190
	MOVE.W	#1,(PlayBit_11A)
	MOVEM.L	(SP)+,D1-D7/A0-A6
	RTS

interrupt	MOVEM.L	D1-D7/A0-A6,-(SP)
	TST.W	(PlayBit_11A)
	BEQ.B	noplay_160
	BSR.W	interrupt_2A2
noplay_160
	lea	lbW00054E,a0
	moveq	#4-1,d0
endcheckloop	tst	(a0)
	bpl.b	notend
	add	#$42,a0
	dbf	d0,endcheckloop
	bsr	report_song_end
notend
	MOVEM.L	(SP)+,D1-D7/A0-A6
	moveq	#0,d0
	RTS

nextsong	ADDQ.W	#1,(subsong)
	MOVE.W	(subsong,PC),D0
	CMP.W	(maxsubsong,PC),D0
	BLE.B	initsound
	CLR.W	(subsong)
	BRA.B	initsound

prevsong	SUBQ.W	#1,(subsong)
	BGE.W	initsound
	MOVE.W	(maxsubsong,PC),(subsong)
	BRA.W	initsound

setsubsong_190	MOVEM.L	D0/A0-A3,-(SP)
	ASL.W	#4,D0
	LEA	(lbW000656,PC),A1
	MOVE.L	A0,A2
	ADD.L	(A0),A2
	MOVE.L	A2,(A1)+
	MOVE.L	A0,A2
	ADD.L	(4,A0),A2
	MOVE.L	A2,(A1)
	LEA	(8,A0,D0.W),A3
	LEA	(lbW00054E,PC),A1
	MOVE.W	#1,(0,A1)
	MOVE.L	A0,A2
	ADD.L	(A3)+,A2
	MOVE.L	A2,(2,A1)
	LEA	($42,A1),A2
	MOVE.L	A2,($16,A1)
	LEA	(lbW000502,PC),A2
	MOVE.L	A2,(14,A1)
	MOVE.W	#1,(6,A1)
	MOVE.W	#$80,(8,A1)
	LEA	($42,A1),A1
	MOVE.W	#1,(0,A1)
	MOVE.L	A0,A2
	ADD.L	(A3)+,A2
	MOVE.L	A2,(2,A1)
	LEA	($42,A1),A2
	MOVE.L	A2,($16,A1)
	LEA	(lbW000502,PC),A2
	MOVE.L	A2,(14,A1)
	MOVE.W	#2,(6,A1)
	MOVE.W	#$100,(8,A1)
	LEA	($42,A1),A1
	MOVE.W	#1,(0,A1)
	MOVE.L	A0,A2
	ADD.L	(A3)+,A2
	MOVE.L	A2,(2,A1)
	LEA	($42,A1),A2
	MOVE.L	A2,($16,A1)
	LEA	(lbW000502,PC),A2
	MOVE.L	A2,(14,A1)
	MOVE.W	#4,(6,A1)
	MOVE.W	#$200,(8,A1)
	LEA	($42,A1),A1
	MOVE.W	#1,(0,A1)
	MOVE.L	A0,A2
	ADD.L	(A3)+,A2
	MOVE.L	A2,(2,A1)
	LEA	($42,A1),A2
	MOVE.L	A2,($16,A1)
	LEA	(lbW000502,PC),A2
	MOVE.L	A2,(14,A1)
	MOVE.W	#8,(6,A1)
	MOVE.W	#$400,(8,A1)
	LEA	(lbW00065E,PC),A0
	MOVE.W	#15,(A0)
	MOVEM.L	(SP)+,D0/A0-A3
	RTS

endsound	MOVEM.L	D0/A0/A1,-(SP)
	MOVEQ	#3,D0
	LEA	(lbW00054E,PC),A0
	LEA	($DFF000),A1
lbC000282	MOVE.W	(6,A0),($96,A1)
	MOVE.W	#$FFFF,(0,A0)
	LEA	($42,A0),A0
	DBRA	D0,lbC000282
	LEA	(lbW00065E,PC),A0
	CLR.W	(A0)
	MOVEM.L	(SP)+,D0/A0/A1
	RTS

interrupt_2A2	MOVEM.L	D4/A0/A2-A5,-(SP)
	LEA	($DFF000),A0
	LEA	(lbW00054E,PC),A2
	LEA	($A0,A0),A5
	BSR.W	handlechannel_2E2
	LEA	($42,A2),A2
	LEA	($10,A5),A5
	BSR.W	handlechannel_2E2
	LEA	($42,A2),A2
	LEA	($10,A5),A5
	BSR.W	handlechannel_2E2
	LEA	($42,A2),A2
	LEA	($10,A5),A5
	BSR.W	handlechannel_2E2
	MOVEM.L	(SP)+,D4/A0/A2-A5
	RTS

handlechannel_2E2	TST.W	(0,A2)
	BMI.W	lbC0004FC
	SUBQ.W	#1,(0,A2)
	BNE.W	lbC0004FC
	MOVE.L	(2,A2),A3
lbC0002F6	CLR.W	D4
	MOVE.B	(A3)+,D4
	JMP	(lbC0002FE,PC,D4.W)

lbC0002FE	BRA.W	lbC000336

	BRA.W	lbC00040C

	BRA.W	lbC0003F8

	BRA.W	lbC000426

	BRA.W	lbC0003DA

	BRA.W	lbC0003E6

	BRA.W	lbC0004DE

	BRA.W	lbC00043A

	BRA.W	lbC0004C8

	BRA.W	lbC000480

	BRA.W	lbC000494

	BRA.W	lbC000454

	BRA.W	lbC0004B0

	BRA.W	lbC0004BE

lbC000336	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.L	(14,A2),A4
	MOVE.W	(A4,D4.W),-(SP)
	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.W	D4,(0,A2)
	MOVE.W	(2,A0),D4
	AND.W	(6,A2),D4
	BEQ.B	lbC00038C
	MOVE.W	(6,A2),($96,A0)	;dmacon
	MOVE.W	#2,(6,A5)
	MOVE.W	#0,(8,A5)

	bsr	dmawait

	MOVE.W	#0,(10,A5)

lbC00038C	MOVE.L	(10,A2),A4
	MOVE.W	(A4)+,D4
	MOVE.L	A4,(0,A5)
	MOVE.W	D4,(4,A5)
	MOVE.W	($12,A2),(8,A5)
	MOVE.W	(SP)+,(6,A5)
	MOVE.W	(6,A2),D4
	OR.W	#$8000,D4
	MOVE.W	D4,($96,A0)	;dmacon

	bsr	shortdmawait

	LEA	(repeatsample_660),A4
	MOVE.L	A4,(0,A5)
	MOVE.W	#2,(4,A5)
	BRA.W	lbC0004F8

lbC0003DA	MOVE.B	(A3)+,D4
	EXT.W	D4
	MOVE.W	D4,(0,A2)
	BRA.W	lbC0004F8

lbC0003E6	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.W	D4,(0,A2)
	MOVE.W	(6,A2),($96,A0)	;dmacon
	BRA.W	lbC0004F8

lbC0003F8	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.L	(lbL00065A,PC),A4
	ADD.L	(A4,D4.W),A4
	MOVE.L	A4,(10,A2)
	BRA.W	lbC0002F6

lbC00040C	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.L	(14,A2),A4
	ADD.W	D4,A4
	MOVE.L	(A4),(6,A5)
	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.W	D4,(0,A2)
	BRA.W	lbC0004F8

lbC000426	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	ADD.W	D4,D4
	ADD.W	D4,D4
	MOVE.W	D4,(8,A5)
	MOVE.W	D4,($12,A2)
	BRA.W	lbC0002F6

lbC00043A	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.L	($16,A2),A4
	MOVE.L	A3,-(A4)
	MOVE.L	A4,($16,A2)
	MOVE.L	(lbW000656,PC),A3
	ADD.W	(A3,D4.W),A3
	BRA.W	lbC0002F6

lbC000454	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	SWAP	D4
	MOVE.B	(A3)+,D4
	MOVE.L	($16,A2),A4
	MOVE.L	A3,-(A4)
	MOVE.L	A4,($16,A2)
	MOVE.L	(lbW000656,PC),A3
	ADD.W	(A3,D4.W),A3
	LEA	(lbW000502,PC),A4
	SWAP	D4
	EXT.W	D4
	ADD.W	D4,A4
	MOVE.L	A4,(14,A2)
	BRA.W	lbC0002F6

lbC000480	MOVEQ	#0,D4
	MOVE.B	(A3)+,D4
	MOVE.L	($16,A2),A4
	MOVE.L	A3,-(A4)
	MOVE.W	D4,-(A4)
	MOVE.L	A4,($16,A2)
	BRA.W	lbC0002F6

lbC000494	MOVE.L	($16,A2),A4
	SUBQ.W	#1,(A4)
	BEQ.B	lbC0004A4
	MOVE.L	(2,A4),A3
	BRA.W	lbC0002F6

lbC0004A4	LEA	(6,A4),A4
	MOVE.L	A4,($16,A2)
	BRA.W	lbC0002F6

lbC0004B0	MOVE.L	($16,A2),A4
	MOVE.L	A3,-(A4)
	MOVE.L	A4,($16,A2)
	BRA.W	lbC0002F6

lbC0004BE	MOVE.L	($16,A2),A4
	MOVE.L	(A4),A3
	BRA.W	lbC0002F6

lbC0004C8	MOVE.L	($16,A2),A4
	MOVE.L	(A4)+,A3
	MOVE.L	A4,($16,A2)
	LEA	(lbW000502,PC),A4
	MOVE.L	A4,(14,A2)
	BRA.W	lbC0002F6

lbC0004DE	MOVEQ	#-1,D4
	MOVE.W	D4,(0,A2)
	MOVE.W	(6,A2),D4
	MOVE.W	D4,($96,A0)	;dmacon
	MOVE.L	A0,-(SP)
	LEA	(lbW00065E,PC),A0
	NOT.W	D4
	AND.W	D4,(A0)
	MOVE.L	(SP)+,A0
lbC0004F8	MOVE.L	A3,(2,A2)
lbC0004FC	RTS

	dc.w	$358
	dc.w	$328
lbW000502	dc.w	$2FA
	dc.w	$2D0
	dc.w	$2A6
	dc.w	$280
	dc.w	$25C
	dc.w	$23A
	dc.w	$21A
	dc.w	$1FC
	dc.w	$1E0
	dc.w	$1C5
	dc.w	$1AC
	dc.w	$194
	dc.w	$17D
	dc.w	$168
	dc.w	$153
	dc.w	$140
	dc.w	$12E
	dc.w	$11D
	dc.w	$10D
	dc.w	$FE
	dc.w	$F0
	dc.w	$E2
	dc.w	$D6
	dc.w	$CA
	dc.w	$BE
	dc.w	$B4
	dc.w	$A9
	dc.w	$A0
	dc.w	$97
	dc.w	$8E
	dc.w	$86
	dc.w	$7F
	dc.w	$78
	dc.w	$71
	dc.w	$6B
	dc.w	$65
	dc.w	$5F
	dc.w	$5A
lbW00054E	dc.w	$26
	dc.w	0
	dc.w	$C524
	dc.w	1
	dc.w	$80
	dc.w	0
	dc.w	$D354
	dc.w	0
	dc.w	$1500
	dc.w	$14
	dcb.w	2,0
	dc.w	$1086
	dcb.w	$11,0
	dc.w	$C1E5
	dc.w	0
	dc.w	$C1DD
	dc.w	$26
	dc.w	0
	dc.w	$C683
	dc.w	2
	dc.w	$100
	dc.w	0
	dc.w	$D354
	dc.w	0
	dc.w	$1500
	dc.w	$14
	dcb.w	2,0
	dc.w	$10C8
	dcb.w	$11,0
	dc.w	$C251
	dc.w	0
	dc.w	$C249
	dc.w	8
	dc.w	0
	dc.w	$C798
	dc.w	4
	dc.w	$200
	dc.w	1
	dc.w	$4E68
	dc.w	0
	dc.w	$1500
	dc.w	$1C
	dcb.w	2,0
	dc.w	$1104
	dcb.w	13,0
	dc.w	1
	dc.w	0
	dc.w	$C795
	dc.w	0
	dc.w	$C2BD
	dc.w	0
	dc.w	$C2B5
	dc.w	2
	dc.w	0
	dc.w	$C9B7
	dc.w	8
	dc.w	$400
	dc.w	1
	dc.w	$9C2A
	dc.w	0
	dc.w	$1500
	dc.w	$30
	dcb.w	2,0
	dc.w	$114C
	dcb.w	15,0
	dc.w	$C97F
	dc.w	0
	dc.w	$C327
	dc.w	0
	dc.w	$C31F
lbW000656	dc.w	0
	dc.w	$C49A
lbL00065A	dc.l	$D330
lbW00065E	dc.w	15

	SECTION	MarkCookseyOld000660,DATA_C
repeatsample_660	dcb.l	6,0

	end
