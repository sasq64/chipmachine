;               T

dtg_AudioAlloc	EQU	$4C
DTP_Check2	EQU	$8000445C
DTP_InitSound	EQU	$80004465
DTP_EndSound	EQU	$80004466
DTP_Interrupt	EQU	$8000445E
dtg_AudioFree	EQU	$50
dtg_GetListData	EQU	$38
DTP_PlayerVersion	EQU	$80004458
DTP_PlayerName	EQU	$80004459
dtg_AslBase	EQU	$0
DTP_EndPlayer	EQU	$80004464
DTP_Process	EQU	$8000447D
dtg_ChkData	EQU	$24
DTP_Creator	EQU	$8000445A
DTP_InitPlayer	EQU	$80004463
****************************************************************************
	SECTION	suntronic000000,CODE
ProgStart
	MOVEQ	#-1,D0
	RTS

	dc.b	'EPPLAYER'
	dc.l	lbL000040
	dc.b	'$VER: SUNtronic player module V1.0 (20 Apr 9'
	dc.b	'7)',0,0
lbL000040	dc.l	DTP_PlayerVersion
	dc.l	1
	dc.l	DTP_Process+$E3
	dc.l	9
	dc.l	DTP_PlayerName
	dc.l	SUNtronic.MSG
	dc.l	DTP_Creator
	dc.l	FelixSchmidta.MSG
	dc.l	DTP_Check2
	dc.l	Check2
	dc.l	DTP_Interrupt
	dc.l	interrupt
	dc.l	DTP_InitPlayer
	dc.l	initplayer
	dc.l	DTP_EndPlayer
	dc.l	endplayer
	dc.l	DTP_InitSound
	dc.l	initsound
	dc.l	DTP_EndSound
	dc.l	endsound
	dc.l	$80004474,2
	dc.l	0
SUNtronic.MSG	dc.b	'SUNtronic',0
FelixSchmidta.MSG	dc.b	'Felix Schmidt,',$A
	dc.b	'adapted by Mr.Larmer/Wanted Team',0
modptr	dc.l	0
lbL0000D2	dc.l	0
lbL0000D6	dc.l	0
lbL0000DA	dcb.l	2,0
lbL0000E2	dc.l	0
delibase	dc.l	0

Check2	move.l	a5,delibase
	MOVE.L	(dtg_ChkData,A5),A0
	MOVEQ	#-1,D0
	CMP.L	#$48E7FFFE,(A0)
	BNE.B	lbC000112
	CMP.W	#$4DFA,(4,A0)
	BNE.B	lbC000112
	CMP.L	#$4A2E0018,(8,A0)
	BEQ.B	lbC000110
	CMP.L	#$4A2E0010,(8,A0)
	BNE.B	lbC000112
lbC000110	MOVEQ	#0,D0
lbC000112	RTS

initplayer	MOVEQ	#0,D0
	MOVE.L	(dtg_GetListData,A5),A0
	JSR	(A0)
	LEA	(modptr,PC),A1
	MOVE.L	A0,(A1)
	MOVE.L	D0,(4,A1)
	MOVE.W	(6,A0),D7
	LSR.W	#1,D7
	MOVE.L	A0,A1
	MOVE.W	D7,D0
lbC000130	CMP.W	#$43EE,(A0)+
	BEQ.B	lbC00013E
	DBRA	D0,lbC000130
	BRA.W	lbC000314

lbC00013E	MOVE.L	(modptr,PC),A2
	ADD.W	(A0),A2
	ADD.W	(6,A1),A2
	ADDQ.W	#6,A2
	MOVE.L	A2,(lbL0000DA)
	MOVE.L	A1,A0
	MOVE.W	D7,D0
lbC000154	CMP.W	#$45EE,(A0)+
	BEQ.B	lbC000162
	DBRA	D0,lbC000154
	BRA.W	lbC000314

lbC000162	CMP.W	#$45EE,(A0)+
	BEQ.B	lbC000170
	DBRA	D0,lbC000162
	BRA.W	lbC000314

lbC000170	MOVE.L	(modptr,PC),A2
	ADD.W	(A0),A2
	ADD.W	(6,A1),A2
	ADDQ.W	#6,A2
	MOVE.L	A2,(lbL0000E2)
lbC000182	CMP.W	#$45EE,(A0)+
	BEQ.B	lbC000190
	DBRA	D0,lbC000182
	BRA.W	lbC000314

lbC000190	MOVE.L	(modptr,PC),A2
	ADD.W	(A0),A2
	ADD.W	(6,A1),A2
	ADDQ.W	#6,A2
	MOVE.L	A2,(lbL0000D6)
	MOVE.L	(modptr,PC),A0
	CMP.B	#$18,(11,A0)
	BNE.W	lbC000264
	MOVE.L	(lbL0000DA,PC),A0
	MOVE.L	(lbL0000D2,PC),D0
	ADD.L	(modptr,PC),D0
	SUB.L	A0,D0
	LSR.W	#1,D0
lbC0001C0	TST.L	(A0)
	BEQ.B	lbC0001D6
	CMP.L	#$FFFFFFFF,(A0)
	BEQ.B	lbC0001D6
	ADDQ.W	#4,A0
	DBRA	D0,lbC0001C0
	BRA.W	lbC000314

lbC0001D6	ADDQ.W	#4,A0
	TST.L	(A0)+
	BNE.W	lbC000314
	TST.L	(A0)+
	BNE.W	lbC000314
	TST.L	(A0)+
	BNE.W	lbC000314
	CMP.L	#$2060A,(A0)+
	BNE.W	lbC000314
	CMP.W	#$F14,(A0)+
	BNE.W	lbC000314
	CMP.B	#$19,(A0)+
	BNE.W	lbC000314
	MOVEQ	#$7F,D0
lbC000206	CMP.B	#$7F,(A0)+
	BEQ.B	lbC000214
	DBRA	D0,lbC000206
	BRA.W	lbC000314

lbC000214	MOVE.L	(modptr,PC),D0
	SUB.L	D0,A0
	MOVE.L	(lbL0000E2,PC),A1
	MOVE.L	(A1),D1
	SUB.L	A0,D1
	MOVE.L	(lbL0000D2,PC),D2
	ADD.L	D1,D2
	MOVE.L	(lbL0000D6,PC),A0
	MOVEQ	#3,D5
lbC00022E	BSR.W	lbC00031A
	ADDQ.W	#2,A1
	CMP.L	A0,A1
	BNE.B	lbC00022E
	MOVE.L	A0,A1
	MOVE.L	(lbL0000DA,PC),A0
	MOVE.L	A0,D5
	SUB.L	A1,D5
	LSR.L	#2,D5
	SUBQ.L	#1,D5
lbC000246	BSR.W	lbC00031A
	CMP.L	A0,A1
	BNE.B	lbC000246
	MOVE.L	A0,A1
	MOVEQ	#3,D5
lbC000252	BSR.W	lbC00031A
	ADDQ.W	#4,A1
	CMP.L	#$FFFFFFFF,(A1)
	BEQ.B	lbC000264
	TST.L	(A1)
	BNE.B	lbC000252
lbC000264	MOVE.L	(modptr,PC),A0
	CMP.B	#$10,(11,A0)
	BNE.W	lbC000314
	MOVE.L	(lbL0000E2,PC),A1
	MOVE.L	(A1)+,D0
	MOVE.L	D0,D1
	MOVE.L	D0,D2
	SUB.L	#$100,D1
	ADD.L	#$100,D2
	MOVE.L	(lbL0000D6,PC),A0
lbC00028C	MOVE.L	(A1),D3
	CMP.L	D1,D3
	BCS.B	lbC0002A0
	CMP.L	D2,D3
	BCC.B	lbC0002A0
	CMP.L	D0,D3
	BCC.B	lbC0002A0
	MOVE.L	D3,D0
	ADDQ.W	#4,A1
	BRA.B	lbC0002A2

lbC0002A0	ADDQ.W	#2,A1
lbC0002A2	CMP.L	A0,A1
	BNE.B	lbC00028C
	MOVE.L	D0,D1
	MOVE.L	(lbL0000DA,PC),A0
	MOVE.L	(lbL0000D2,PC),D0
	ADD.L	(modptr,PC),D0
	SUB.L	A0,D0
	LSR.W	#1,D0
lbC0002B8	TST.L	(A0)+
	BEQ.B	lbC0002C4
	DBRA	D0,lbC0002B8
	BRA.W	lbC000314

lbC0002C4	MOVE.L	(modptr,PC),D0
	SUB.L	D0,A0
	SUB.L	A0,D1
	MOVE.L	(lbL0000E2,PC),A1
	MOVE.L	(lbL0000D2,PC),D2
	ADD.L	D1,D2
	MOVE.L	(lbL0000D6,PC),A0
lbC0002DA	MOVE.L	(A1),D3
	CMP.L	D1,D3
	BCS.B	lbC0002EC
	CMP.L	D2,D3
	BCC.B	lbC0002EC
	SUB.L	D1,(A1)
	ADD.L	D0,(A1)
	ADDQ.W	#4,A1
	BRA.B	lbC0002EE

lbC0002EC	ADDQ.W	#2,A1
lbC0002EE	CMP.L	A0,A1
	BNE.B	lbC0002DA
	MOVE.L	A0,A1
	MOVE.L	(lbL0000DA,PC),A0
	MOVE.L	A0,D5
	SUB.L	A1,D5
	LSR.L	#2,D5
	SUBQ.L	#1,D5
lbC000300	BSR.W	lbC00031A
	CMP.L	A0,A1
	BNE.B	lbC000300
	MOVE.L	A0,A1
	MOVEQ	#0,D5
lbC00030C	BSR.W	lbC00031A
	TST.L	(A1)
	BNE.B	lbC00030C
lbC000314
	move.l	modptr(pc),a0
	lea	$100(a0),a1
	add	#$60,a0
floop_1	cmp	#$302e,2(a0)
	bne.b	fnext_1
	cmp.l	#$001ec0fc,4(a0)
	bne.b	fnext_1
	cmp.l	#$00140cb1,8(a0)
	bne.b	fnext_1
	cmp.l	#$ffffffff,12(a0)
	bne.b	fnext_1
	cmp.l	#$00006606,16(a0)
	bne.b	fnext_1
	cmp.l	#$1d7c0001,20(a0)
	bne.b	fnext_1
	cmp	#$0018,24(a0)
	bne.b	fnext_1
	cmp.l	#$4ab10000,26(a0)
	bne.b	fnext_1
	cmp.l	#$6606426e,30(a0)
	bne.b	fnext_1
	cmp.l	#$001e60da,34(a0)
	beq.b	fout_1
fnext_1	addq.l	#2,a0
	cmp.l	a0,a1
	bne.b	floop_1
fout_1	cmp.l	a0,a1
	beq.b	noendpatch_1
	move.l	a0,-(a7)
	add	#20,a0
	bsr	checkstop1
	move.l	(a7),a0
	add	#32,a0
	bsr	checkrestart1
	move.l	(a7)+,a0
	bra	noendpatch
noendpatch_1
;	move.l	modptr(pc),a0
;	lea	$80(a0),a1
;	add	#$30,a0
;floop_2	cmp	#$302e,2(a0)
;	bne.b	fnext_2
;	cmp.l	#$0014c0fc,4(a0)
;	bne.b	fnext_2
;	cmp	#$0014,8(a0)
;	bne.b	fnext_2
;	cmp.l	#$4ab10000,10(a0)
;	bne.b	fnext_2
;	cmp.l	#$6606426e,14(a0)
;	bne.b	fnext_2
;	cmp	#$0014,18(a0)
;	bne.b	fnext_2
;	cmp.b	#$60,20(a0)
;	beq.b	fout_2
;fnext_2	addq.l	#2,a0
;	cmp.l	a0,a1
;	bne.b	floop_2
;fout_2	cmp.l	a0,a1
;	beq.b	noendpatch_2
;	add	#16,a0
;	bsr	checkrestart2
;	bra	noendpatch
;noendpatch_2
noendpatch
	MOVE.L	(dtg_AudioAlloc,A5),A0
	JMP	(A0)

checkstop1	cmp.l	#$1d7c0001,(a0)
	bne.b	nostop1
	cmp	#$0018,4(a0)
	bne.b	nostop1
	move	jsrcom(pc),(a0)
	move.l	#songendstop1,2(a0)
nostop1	rts

checkrestart1	cmp.l	#$426e001e,(a0)
	bne.b	norestart1
	cmp	#$60da,4(a0)
	bne.b	norestart1
	move	jsrcom(pc),(a0)
	move.l	#songendrestart1,2(a0)
norestart1	rts

;checkrestart2	cmp.l	#$426e0014,(a0)
;	bne.b	norestart2
;	cmp.b	#$60,4(a0)
;	bne.b	norestart2
;	move.b	5(a0),d0
;	neg.b	d0
;	ext	d0
;	ext.l	d0
;	move.l	d0,restart2const
;	move	jsrcom(pc),(a0)
;	move.l	#songendrestart2,2(a0)
;norestart2	rts
;
;restart2const	dc.l	0

jsrcom	jsr	0

songendstop1	move.b	#1,$18(a6)
	bsr	songend
	rts

songendrestart1	clr	$1e(a6)
	move.l	d0,-(a7)
	move.l	4(a7),d0
	sub.l	#$26,d0
	move.l	d0,4(a7)
	move.l	(a7)+,d0
	bsr	songend
	rts

;songendrestart2	clr	$14(a6)
;	move.l	d0,-(a7)
;	move.l	4(a7),d0
;	sub.l	restart2const(pc),d0
;	move.l	d0,4(a7)
;	move.l	(a7)+,d0
;	bsr	songend
;	rts

songend	movem.l	d0-d7/a0-a6,-(a7)
	move.l	delibase(pc),a5
	move.l	$5c(a5),a0
	jsr	(a0)
	movem.l	(a7)+,d0-d7/a0-a6
	rts

lbC00031A	MOVE.W	D5,D4
lbC00031C	MOVE.L	(A1),D3
	CMP.L	D1,D3
	BCS.B	lbC00032A
	CMP.L	D2,D3
	BCC.B	lbC00032A
	SUB.L	D1,(A1)
	ADD.L	D0,(A1)
lbC00032A	ADDQ.W	#4,A1
	DBRA	D4,lbC00031C
	RTS

endplayer	MOVE.L	(dtg_AudioFree,A5),A0
	JMP	(A0)

initsound	MOVE.L	(modptr,PC),A1
	ADD.W	(6,A1),A1
	ADDQ.W	#6,A1
	MOVE.L	(modptr,PC),A0
	CMP.B	#$18,(11,A0)
	BNE.B	lbC0003B6
	CLR.L	(12,A1)
	CLR.L	($10,A1)
	MOVE.B	#5,($14,A1)
	MOVE.B	#$FF,($15,A1)
	MOVE.B	#6,($16,A1)
	CLR.L	($1A,A1)
	CLR.W	($1E,A1)
	LEA	(lbL000490),A0
	MOVE.L	A0,($20,A1)
	MOVE.W	#$12F,D0
lbC00037E	CLR.L	(A0)+
	DBRA	D0,lbC00037E
	LEA	(lbL000490),A0
	MOVE.L	(lbL0000DA,PC),A1
	MOVEQ	#3,D0
lbC000390	MOVE.L	(A1)+,(A0)
	MOVE.B	#$FF,($14,A0)
	LEA	($130,A0),A0
	DBRA	D0,lbC000390
	LEA	(lbL000490),A0
	MOVEQ	#3,D0
lbC0003A8	MOVE.B	(A1)+,($12E,A0)
	LEA	($130,A0),A0
	DBRA	D0,lbC0003A8
	RTS

lbC0003B6	MOVE.L	(lbL0000DA,PC),A0
	TST.B	($10,A0)
	BEQ.B	lbC000426
	MOVE.B	#4,($10,A1)
	MOVE.B	#15,($11,A1)
	MOVE.B	#5,($12,A1)
	CLR.B	($13,A1)
	CLR.B	($14,A1)
	MOVE.B	#$1F,($15,A1)
	LEA	(lbL000490),A0
	MOVE.L	A0,($16,A1)
	MOVE.W	#$12F,D0
lbC0003EE	CLR.L	(A0)+
	DBRA	D0,lbC0003EE
	LEA	(lbL000490),A0
	MOVE.L	(lbL0000DA,PC),A1
	MOVEQ	#3,D0
lbC000400	MOVE.L	(A1)+,(A0)
	MOVE.B	#$FF,($14,A0)
	LEA	($68,A0),A0
	DBRA	D0,lbC000400
	LEA	(lbL000490),A0
	MOVEQ	#3,D0
lbC000418	MOVE.B	(A1)+,($12E,A0)
	LEA	($68,A0),A0
	DBRA	D0,lbC000418
	RTS

lbC000426	CLR.L	(12,A1)
	MOVE.B	#5,($10,A1)
	MOVE.B	#$FF,($11,A1)
	MOVE.B	#6,($12,A1)
	CLR.B	($13,A1)
	CLR.B	($14,A1)
	MOVE.B	#$40,($15,A1)
	LEA	(lbL000490),A0
	MOVE.L	A0,($16,A1)
	MOVE.W	#$12F,D0
lbC000458	CLR.L	(A0)+
	DBRA	D0,lbC000458
	LEA	(lbL000490),A0
	MOVE.L	(lbL0000DA,PC),A1
	MOVEQ	#3,D0
lbC00046A	MOVE.L	(A1)+,(A0)
	MOVE.B	#$FF,($14,A0)
	LEA	($62,A0),A0
	DBRA	D0,lbC00046A
	RTS

endsound	RTS

interrupt	MOVEM.L	D1-D7/A0-A6,-(SP)
	MOVE.L	(modptr,PC),A0
	JSR	(A0)
	MOVEM.L	(SP)+,D1-D7/A0-A6
	MOVEQ	#0,D0
	RTS


	SECTION	suntronic000490,BSS_c
lbL000490	ds.l	$130

	end
