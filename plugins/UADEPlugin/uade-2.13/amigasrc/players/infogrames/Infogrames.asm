;               T

_LVOOpenLibrary	equ	-552
EP_OPTION_SIZE	equ	256

dtg_AudioAlloc	EQU	$4C
DTP_Check2	EQU	$8000445C
DTP_InitSound	EQU	$80004465
DTP_Volume	EQU	$80004469
dtg_SndVol	EQU	$2E
DTP_Interrupt	EQU	$8000445E
DTP_ExtLoad	EQU	$8000445D
DTP_NextSong	EQU	$8000446F
dtg_CopyFile	EQU	$44
dtg_AudioFree	EQU	$50
dtg_GetListData	EQU	$38
dtg_PathArrayPtr	EQU	$20
DTP_PlayerVersion	EQU	$80004458
DTP_PlayerName	EQU	$80004459
dtg_LoadFile	EQU	$3C
DTP_EndPlayer	EQU	$80004464
DTP_PrevSong	EQU	$80004470
dtg_Timer	EQU	$36
dtg_CopyDir	EQU	$40
dtg_ChkSize	EQU	$28
dtg_ChkData	EQU	$24
DTP_Creator	EQU	$8000445A
DTP_DeliBase	EQU	$80004473
dtg_WaitAudioDMA	EQU	$68
DTP_InitPlayer	EQU	$80004463
****************************************************************************
	SECTION	Infogrames000000,CODE
ProgStart
	MOVEQ	#-1,D0
	RTS

	dc.b	'DELIRIUM'
	dc.l	lbL000010
lbL000010	dc.l	DTP_PlayerVersion
	dc.l	1
	dc.l	DTP_PlayerName
	dc.l	Infogrames.MSG
	dc.l	DTP_Creator
	dc.l	InfogramesRob.MSG
	dc.l	DTP_DeliBase
	dc.l	Delibase
	dc.l	DTP_Check2
	dc.l	CheckFormat
	dc.l	DTP_ExtLoad
	dc.l	LoadFiles
	dc.l	DTP_InitPlayer
	dc.l	InitPlayer
	dc.l	DTP_InitSound
	dc.l	InitSound
	dc.l	DTP_Interrupt
	dc.l	Interrupt
	dc.l	DTP_EndPlayer
	dc.l	EndPlayer
	dc.l	DTP_NextSong
	dc.l	NextSubsong
	dc.l	DTP_PrevSong
	dc.l	PrevSubsong
	dc.l	DTP_Volume
	dc.l	SetVolume
	dc.l	0
Delibase	dc.l	0	;eagle/delitracker base ptr is stored here
Infogrames.MSG	dc.b	'Infogrames',0
InfogramesRob.MSG	dc.b	'Infogrames (RobHubbard2),',$A
	dc.b	'adapted by Andy Silva',0
	dc.b	'$VER: a player for the famous DeliTracker',0,0

; Load external sample file
LoadFiles	MOVE.L	A6,-(SP)
	MOVE.L	(dtg_PathArrayPtr,A5),A0
	CLR.B	(A0)
	MOVE.L	(dtg_CopyDir,A5),A1
	JSR	(A1)
	MOVE.L	(dtg_CopyFile,A5),A1
	JSR	(A1)
	MOVE.L	(dtg_PathArrayPtr,A5),A0
lbC0000FE	TST.B	(A0)+
	BNE.B	lbC0000FE
	SUBQ.L	#1,A0
	MOVE.L	A0,-(SP)
	CMP.B	#$4D,(-1,A0)
	BEQ.B	lbC000116
	CMP.B	#$6D,(-1,A0)
	BNE.B	lbC000142
lbC000116	CMP.B	#$55,(-2,A0)
	BEQ.B	lbC000126
	CMP.B	#$75,(-2,A0)
	BNE.B	lbC000142
lbC000126	CMP.B	#$44,(-3,A0)
	BEQ.B	lbC000136
	CMP.B	#$64,(-3,A0)
	BNE.B	lbC000142
lbC000136	CMP.B	#$2E,(-4,A0)
	BNE.B	lbC000142
	SUBQ.L	#4,A0
	MOVE.L	A0,(SP)
lbC000142	BSR.B	lbC000166
	MOVE.L	(dtg_PathArrayPtr,A5),D1
	MOVE.L	(dtg_LoadFile,A5),A0
	JSR	(A0)
	TST.L	D0
	BEQ.B	lbC000160
	MOVE.L	(SP),A0
	SUBQ.L	#1,A0
	BSR.W	lbC000166
	MOVE.L	(dtg_LoadFile,A5),A0
	JSR	(A0)
lbC000160	ADDQ.L	#4,SP
	MOVE.L	(SP)+,A6
	RTS

lbC000166	MOVE.B	#$2E,(A0)+	;ascii bytes
	MOVE.B	#$69,(A0)+
	MOVE.B	#$6E,(A0)+
	MOVE.B	#$73,(A0)+
	CLR.B	(A0)+
	RTS

CheckFormat	MOVE.L	A5,(Delibase)	;check that file is infogrames format
	MOVEQ	#0,D0
	MOVE.L	(dtg_ChkData,A5),A0
	MOVE.W	(A0),D0
	BEQ.B	lbC0001B8
	BTST	#0,D0
	BNE.B	lbC0001B8
	MOVE.L	(dtg_ChkSize,A5),D1
	CMP.L	D0,D1
	BLE.B	lbC0001B8
	LEA	(A0,D0.W),A1
	MOVE.W	(2,A1),D0
	TST.B	(A1,D0.W)
	BNE.B	lbC0001B8
	CMP.B	#15,(1,A1,D0.W)
	BNE.B	lbC0001B8
	MOVE.L	A0,(DumFilePtr)
	MOVEQ	#0,D0
	RTS

lbC0001B8	MOVEQ	#-1,D0
	RTS

timerlist
* gobliins 2
	dc.l	$752ab846,2850,$24ff
	dc.l	$c0691337,3634,$24ff
	dc.l	$e19fe7a1,4211,$24ff
	dc.l	$0b0f2eba,3305,$24ff
* gobliins 3
	dc.l	$abdff091,3433,$24ff
	dc.l	$c55b87e3,2605,$24ff
	dc.l	$f8ac972a,2180,$24ff
* horror zombies
	dc.l	$bff9c6c1,3234,$1b66
* ween
	dc.l	$cd059f22,3160,$24ff	* Ween.dum
	dc.l	$0990733e,2614,$24ff	* mus2.dum
	dc.l	$eb38a5f3,2183,$24ff	* mus2b.dum
	dc.l	$b3e5589e,2179,$24ff	* mus2c.dum
* end list
	dc.l	0,0,0

uadebase	dc.l	0
uadename	dc.b	'uade.library',0
timervaluename	dc.b	'timer',0
timervaluereceived	dc.b	0
	even

epoptarray	dc.l	2,timervaluename,timervaluereceived,timervalue
timervalue	dc.l	0

determine_timer_value
	movem.l	d0-a6,-(a7)
	move.l	Delibase,a5

	* set default timer value
	MOVE.W	#$1A00,(dtg_Timer,A5)

	move.l	uadebase,d0
	beq.b	usetimerlist
	move.l	d0,a6
	lea	epoptarray,a0
	jsr	-24(a6)
	tst.l	d0
	beq.b	usetimerlist
	tst.b	timervaluereceived
	beq.b	usetimerlist
	move.l	timervalue,d0
	move	d0,dtg_Timer(a5)
	bra	outoftimercheck

usetimerlist
	* compute sum32
	MOVE.L	(dtg_ChkData,A5),A0
	MOVE.L	(dtg_ChkSize,A5),D1
	moveq	#0,d0
sum32loop	cmp.l	#4,d1
	blt.b	lessthan4
	add.l	(a0)+,d0
	subq.l	#4,d1
	bra.b	sum32loop
lessthan4	lsl	#2,d1
	move.l	masklist(pc,d1),d1
	and.l	(a0),d1
	add.l	d1,d0

	* look up sum32/filesize in timerlist
	move.l	dtg_ChkSize(a5),d1
	lea	timerlist,a0
timerlistloop	move.l	(a0)+,d2	* sum32
	move.l	(a0)+,d3	* file size
	beq.b	outoftimercheck
	move.l	(a0)+,d4	* timer value
	cmp.l	d0,d2
	bne.b	timerlistloop
	cmp.l	d1,d3
	bne.b	timerlistloop
	move	d4,dtg_Timer(a5)
outoftimercheck
	movem.l	(a7)+,d0-a6
	rts

masklist	dc.l	0,$ff000000,$ffff0000,$ffffff00

InitPlayer	MOVE.L	(DumFilePtr,PC),A0
	MOVE.L	A0,(DumFilePtr2)
	MOVE.W	(A0),D0
	LSR.W	#1,D0
	SUBQ.W	#1,D0
	MOVE.W	D0,(Subsong)
	MOVE.L	(dtg_GetListData,A5),A1
	MOVEQ	#1,D0
	JSR	(A1)
	ADDQ.L	#4,A0
	MOVE.L	A0,(SampleDataPtr)

	lea	uadename,a1
	moveq	#0,d0
	move.l	4.w,a6
	jsr	_LVOOpenLibrary(a6)
	move.l	d0,uadebase

	MOVE.L	(dtg_AudioAlloc,A5),A0	;allocate audio channels
	JSR	(A0)
	TST.L	D0
	RTS

EndPlayer	MOVE.L	(dtg_AudioFree,A5),A0	;free audio channels
	JSR	(A0)
	RTS

DumFilePtr2	dc.l	0
DumFilePtr	dc.l	0
Subsong	dc.w	0
MaxSubsong	dc.w	0
PlayBoolean	dc.w	0

; InitSound is buggy. It doesn't read subsong number from delibase to initialise subsong
InitSound	MOVEM.L	D1-D7/A0-A6,-(SP)
	CLR.W	(PlayBoolean)
	bsr	determine_timer_value
	MOVE.L	(DumFilePtr2,PC),-(SP)
	MOVE.W	(Subsong,PC),-(SP)
	BSR.W	lbC0003DE
	ADDQ.L	#6,SP
	MOVE.W	#$80,-(SP)
	CLR.W	-(SP)
	BSR.W	lbC00049E
	ADDQ.L	#4,SP
	MOVE.W	#1,(PlayBoolean)
	MOVEM.L	(SP)+,D1-D7/A0-A6
	RTS

Interrupt	MOVEM.L	D1-D7/A0-A6,-(SP)
	TST.W	(PlayBoolean)
	BEQ.B	lbC00024A
	BSR.W	lbC000586
lbC00024A	MOVEM.L	(SP)+,D1-D7/A0-A6
	RTS

NextSubsong	ADDQ.W	#1,(Subsong)
	MOVE.W	(Subsong,PC),D0
	CMP.W	(MaxSubsong,PC),D0
	BLE.B	InitSound
	CLR.W	(Subsong)
	BRA.B	InitSound

PrevSubsong	SUBQ.W	#1,(Subsong)
	BGE.B	InitSound
	MOVE.W	(MaxSubsong,PC),(Subsong)
	BRA.B	InitSound

SetVolume	LEA	(lbL000A24,PC),A6
	MOVE.W	(dtg_SndVol,A5),D0
	MOVE.B	D0,($1D,A6)
	RTS

	LINK	A6,#0
	MOVEM.L	D1-D7/A0-A6,-(SP)
	MOVE.L	#$FFFF,D0
	MOVE.W	(10,A6),D1
	MOVE.W	(8,A6),D5
	LEA	(lbL000A24),A6
	MOVE.W	D1,D2
	AND.L	#15,D2
	BNE.B	lbC0002B2
	OR.W	#15,D2
lbC0002B2	LEA	(lbL000A78),A5
	MOVEQ	#0,D7
lbC0002BA	LSR.W	#1,D2
	BCC.W	lbC00037C
	MOVE.L	($1C,A5),A0
	BTST	#15,D1
	BEQ.B	lbC0002CE
	MOVE.B	D5,($2F,A0)
lbC0002CE	BTST	#13,D1
	BEQ.B	lbC0002DC
	MOVE.B	D5,($2B,A0)
	MOVE.B	D5,($2F,A0)
lbC0002DC	BTST	#8,D1
	BEQ.B	lbC0002EE
	CLR.B	($2B,A0)
	MOVE.B	($1D,A6),D0
	MOVE.B	D0,($2F,A0)
lbC0002EE	BTST	#9,D1
	BEQ.B	lbC0002FA
	OR.W	#$200,($26,A0)
lbC0002FA	BTST	#6,D1
	BEQ.B	lbC000318
	MOVE.W	#$40,($26,A0)
	BCLR	#1,($27,A5)
	BTST	#6,($27,A5)
	BEQ.B	lbC000318
	BSR.W	lbC000950
lbC000318	BTST	#7,D1
	BEQ.B	lbC000370
	BTST	#14,D1
	BEQ.B	lbC00032C
	BTST	#1,($27,A5)
	BNE.B	lbC000370
lbC00032C	MOVE.B	D5,($2D,A0)
	LEA	($2D,A0),A1
	MOVE.L	A1,(0,A0)
	BCLR	#5,($27,A0)
	BCLR	#6,($27,A0)
	OR.W	#$80,($26,A0)
	OR.W	#1,($26,A0)
	OR.W	#2,($26,A5)
	OR.W	#2,($26,A0)
	MOVE.B	#$3F,($2B,A0)
	MOVE.B	#$3F,($2F,A0)
	ST	($1A,A6)
	MOVE.L	($18,A0),A1
lbC000370	BTST	#5,D1
	BEQ.B	lbC00037C
	OR.W	#$20,($26,A0)
lbC00037C	ADD.L	#$50,A5
	ADDQ.W	#1,D7
	CMP.W	#4,D7
	BNE.W	lbC0002BA
	MOVE.L	D0,-(SP)
	MOVE.L	(SP)+,D0
	MOVEM.L	(SP)+,D1-D7/A0-A6
	UNLK	A6
	RTS

lbC000398	MOVE.W	(A0)+,D3
	BEQ.B	lbC0003AC
	CMP.W	D3,D1
	BEQ.B	lbC0003A6
	LEA	(4,A0),A0
	BRA.B	lbC000398

lbC0003A6	MOVE.L	(A0),A0
	MOVE.L	A0,-(SP)
	RTS

lbC0003AC	ORI.B	#1,CCR
	RTS

lbC0003B2	MOVE.B	D5,($1C,A6)
	MOVE.B	D5,($1B,A6)
	ANDI.B	#$FE,CCR
	RTS

lbC0003C0	MOVE.B	D5,($1D,A6)
	ANDI.B	#$FE,CCR
	RTS

lbC0003CA	MOVE.B	D5,($1E,A6)
	ANDI.B	#$FE,CCR
	RTS

	MOVE.L	(4,SP),(SampleDataPtr)
	RTS

lbC0003DE	LINK	A6,#0
	MOVEM.L	D0-D7/A0-A6,-(SP)
	MOVE.L	(10,A6),A0
	MOVE.W	(8,A6),D0	;subsong number
	LEA	(lbL000A24),A6
	SF	($1A,A6)
	LSL.W	#1,D0
	MOVE.W	(A0,D0.W),D0
	LEA	(A0,D0.W),A0
	MOVE.L	A0,(12,A6)
	MOVE.L	A0,A1
	MOVE.W	(A0)+,D0
	MOVE.B	D0,($1B,A6)
	MOVE.B	D0,($1C,A6)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A2
	MOVE.L	A2,(4,A6)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A2
	MOVE.L	A2,(8,A6)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A3
	LEA	(lbL000A78),A2
	MOVE.L	A3,(0,A2)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A3
	LEA	(lbL000AC8),A2
	MOVE.L	A3,(0,A2)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A3
	LEA	(lbL000B18),A2
	MOVE.L	A3,(0,A2)
	MOVE.W	(A0)+,D0
	LEA	(A1,D0.W),A3
	LEA	(lbL000B68),A2
	MOVE.L	A3,(0,A2)
	MOVE.W	(A0)+,D0
	MOVE.L	A0,($10,A6)
	MOVE.B	#$3F,($1D,A6)
	MOVE.W	#3,D3
	LEA	(lbL000A78),A2
lbC00047C	OR.W	#$40,($26,A2)
	MOVE.B	#$3F,($2B,A2)
	MOVE.B	#$3F,($2F,A2)
	ADD.W	#$50,A2
	DBRA	D3,lbC00047C
	MOVEM.L	(SP)+,D0-D7/A0-A6
	UNLK	A6
	RTS

lbC00049E	LINK	A6,#0
	MOVEM.L	D1-D7/A0-A6,-(SP)
	MOVE.L	#$FFFF,D0
	MOVE.W	(10,A6),D1
	MOVE.W	(8,A6),D5
	LEA	(lbW000A64),A0
	LEA	(lbL000A24),A6
	BSR.W	lbC000398
	BCC.W	lbC00057E
	MOVE.W	D1,D2
	AND.L	#15,D2
	BNE.B	lbC0004D6
	OR.W	#15,D2
lbC0004D6	LEA	(lbL000A78),A0
	MOVEQ	#0,D7
lbC0004DE	LSR.W	#1,D2
	BCC.W	lbC00056E
	BTST	#15,D1
	BEQ.B	lbC0004F2
	AND.B	#$3F,D5
	MOVE.B	D5,($2F,A0)
lbC0004F2	BTST	#13,D1
	BEQ.B	lbC000500
	MOVE.B	D5,($2B,A0)
	MOVE.B	D5,($2F,A0)
lbC000500	BTST	#8,D1
	BEQ.B	lbC000512
	CLR.B	($2B,A0)
	MOVE.B	($1D,A6),D0
	MOVE.B	D0,($2F,A0)
lbC000512	BTST	#9,D1
	BEQ.B	lbC00051C
	CLR.B	($2F,A0)
lbC00051C	BTST	#6,D1
	BEQ.B	lbC00052C
	OR.W	#$40,($26,A0)
	BSR.W	lbC000950
lbC00052C	BTST	#7,D1
	BEQ.B	lbC000546
	CLR.W	($26,A0)
	OR.W	#$80,($26,A0)
	OR.W	#1,($26,A0)
	ST	($1A,A6)
lbC000546	BTST	#5,D1
	BEQ.B	lbC000552
	OR.W	#$20,($26,A0)
lbC000552	BTST	#4,D1
	BEQ.B	lbC00056E
	BTST	#6,($27,A0)
	BNE.B	lbC00056E
	MOVEQ	#0,D6
	MOVE.W	($24,A0),D6
	CMP.L	D6,D0
	BCS.B	lbC00056E
	MOVE.W	($24,A0),D0
lbC00056E	ADD.L	#$50,A0
	ADDQ.W	#1,D7
	CMP.W	#4,D7
	BNE.W	lbC0004DE
lbC00057E	MOVEM.L	(SP)+,D1-D7/A0-A6
	UNLK	A6
	RTS

lbC000586	MOVEM.L	D0/A6,-(SP)
	LEA	(lbL000A24,PC),A6
	MOVEM.L	D0-D7/A0-A5,-(SP)
	SUBQ.B	#1,($1B,A6)
	MOVE.B	#$FF,($18,A6)
	MOVEQ	#0,D7	;channel number
	LEA	(lbL000A78,PC),A3
AudioChannelLoop	CLR.W	(A6)
	CLR.W	(2,A6)
	BSR.W	lbC000726
	MOVE.L	A3,-(SP)
	BTST	#1,($27,A3)
	BEQ.B	lbC0005C4
	MOVE.B	#$FF,($18,A6)
	MOVE.L	($1C,A3),A3
	BSR.W	lbC000726
lbC0005C4	MOVEM.L	D0-D3/A0/A1,-(SP)
	MOVEQ	#0,D1
	MOVE.W	D7,D1
	LSL.W	#4,D1	;shift audio channel
	MOVE.L	#$DFF0A0,A1	;audio hw channel 0
	MOVE.W	(0,A6),(8,A1,D1.W)	;set volume
	MOVE.W	(2,A6),(6,A1,D1.W)	;set period
	CMP.B	#$FF,($18,A6)
	BEQ.B	lbC00064C
	MOVEQ	#0,D0
	MOVE.B	($18,A6),D0	;sample number
	MOVE.B	#$FF,($18,A6)
	LSL.W	#4,D0
	MOVEQ	#0,D1
	MOVE.W	D7,D1
	LSL.W	#4,D1
	MOVEQ	#1,D2
	LSL.W	D7,D2
	MOVE.L	(SampleDataPtr),A0
	MOVE.L	#$DFF0A0,A1
	MOVE.L	(A0,D0.W),D3	;get sample offset
	ADD.L	A0,D3	;add sample base
	MOVE.L	D3,(A1,D1.W)	;set sample pointer
	MOVE.W	(12,A0,D0.W),(4,A1,D1.W)	;set sample length
	MOVE.W	D2,($DFF096)	;kill channel audio dma
	BSR.W	Wait_Audio_DMA_Hardware_To_Reset
	OR.W	#$8200,D2	;Audio DMA enable mask
	MOVE.W	D2,($DFF096)	;Enable Audio DMA with new values
	BSR.W	Wait_Audio_DMA_Hardware_To_Reset
	MOVE.L	(4,A0,D0.W),D3
	ADD.L	A0,D3
	MOVE.L	D3,(A1,D1.W)	;Set repeat sample (notice previous wait)
	MOVE.W	(14,A0,D0.W),(4,A1,D1.W)	;Set repeat length
lbC00064C	MOVEM.L	(SP)+,D0-D3/A0/A1
	MOVE.L	(SP)+,A3
	ADD.W	#$50,A3
	ADDQ.W	#1,D7
	CMP.W	#4,D7
	BNE.W	AudioChannelLoop
	TST.B	($1B,A6)
	BNE.B	lbC00066C
	MOVE.B	($1C,A6),($1B,A6)
lbC00066C	ST	($1A,A6)
	MOVEM.L	(SP)+,D0-D7/A0-A5
	MOVEM.L	(SP)+,D0/A6
	MOVEQ	#0,D0
	RTS

; Waits long enough that audio state machine can reset itself, if dma was turned on/off before this
Wait_Audio_DMA_Hardware_To_Reset
	MOVEM.L	D0-D7/A0-A6,-(SP)
	MOVE.L	(Delibase),A5
	MOVE.L	(dtg_WaitAudioDMA,A5),A1
	JSR	(A1)
	MOVEM.L	(SP)+,D0-D7/A0-A6
	RTS

lbC000692	MOVEQ	#0,D3
	MOVE.B	(10,A0),D3
	MOVE.L	(4,A0),A1
	LEA	(A1,D3.W),A1
	BCLR	#0,(11,A0)
	BEQ.B	lbC0006B2
	MOVE.B	(1,A1),D3
	EXT.W	D3
	ADD.W	D3,(2,A0)
lbC0006B2	ADD.W	(2,A0),D0
	SUB.W	(0,A0),D0
	BTST	#15,D0
	BEQ.B	lbC0006C2
	MOVEQ	#0,D0
lbC0006C2	BTST	#2,(11,A0)
	BNE.B	lbC00071C
	ADDQ.B	#1,(12,A0)
	MOVE.B	(2,A1),D3
	CMP.B	(12,A0),D3
	BNE.B	lbC00071C
	ADDQ.B	#1,(13,A0)
	CLR.B	(12,A0)
	MOVE.B	(0,A1),D3
	CMP.B	(13,A0),D3
	BNE.B	lbC000716
	CLR.B	(13,A0)
	MOVE.B	(10,A0),D3
	ADDQ.W	#3,D3
	CMP.B	#12,D3
	BNE.B	lbC000712
	TST.B	(11,A0)
	BEQ.B	lbC00071E
	CLR.B	(13,A0)
	MOVE.B	(8,A0),D3
	EXT.W	D3
	ADD.W	D3,(0,A0)
	MOVE.W	#3,D3
lbC000712	MOVE.B	D3,(10,A0)
lbC000716	BSET	#0,(11,A0)
lbC00071C	RTS

lbC00071E	BSET	#2,(11,A0)
	RTS

lbC000726	BTST	#6,($27,A3)
	BNE.W	lbC00094E
	MOVE.L	($18,A3),D0
	MOVE.L	D0,($14,A6)
	MOVE.B	($2F,A3),D0
	CMP.B	($2B,A3),D0
	BEQ.B	lbC000768
	ADDQ.B	#1,($2C,A3)
	MOVE.B	($2C,A3),D0
	CMP.B	($1E,A6),D0
	BLS.B	lbC000768
	CLR.B	($2C,A3)
	MOVE.B	($2F,A3),D0
	CMP.B	($2B,A3),D0
	BLS.B	lbC000764
	ADDQ.B	#1,($2B,A3)
	BRA.B	lbC000768

lbC000764	SUBQ.B	#1,($2B,A3)
lbC000768	BCLR	#0,($27,A3)
	BNE.B	lbC0007D6
	TST.B	($1B,A6)
	BNE.B	lbC00077A
	SUBQ.B	#1,($28,A3)
lbC00077A	TST.B	($28,A3)
	BNE.W	lbC00090E
	MOVE.B	($29,A3),($28,A3)
lbC000788	MOVE.L	(8,A3),A0
	MOVE.B	(A0)+,D0
	CMP.B	#$FF,D0
	BNE.B	lbC000802
lbC000794	MOVE.L	(12,A3),A0
	MOVE.B	(A0)+,D0
	CMP.B	#$FF,D0
	BNE.B	lbC0007E2
	BTST	#5,($27,A3)
	BNE.B	lbC0007D6
	MOVE.L	(4,A3),A1
	BTST	#1,($27,A1)
	BEQ.B	lbC0007C0
	BTST	#1,($26,A3)
	BEQ.B	lbC0007C0
	CLR.B	($2B,A1)
lbC0007C0	BCLR	#1,($27,A1)
	OR.W	#$40,($26,A3)
	MOVE.W	#0,(0,A6)
	BRA.W	lbC00094E

lbC0007D6	MOVE.L	(0,A3),(12,A3)
	CLR.W	($24,A3)
	BRA.B	lbC000794

lbC0007E2	MOVE.L	A0,(12,A3)
	ADDQ.W	#1,($24,A3)
	AND.W	#$FF,D0
	LSL.W	#1,D0
	MOVE.L	($10,A6),A0
	MOVE.W	(A0,D0.W),A0
	ADD.L	(12,A6),A0
	MOVE.L	A0,(8,A3)
	BRA.B	lbC000788

lbC000802	MOVE.L	A0,(8,A3)
	AND.L	#$FF,D0
	BTST	#7,D0
	BNE.B	lbC00087A
	LEA	(lbW000D1A),A0
	TST.B	D0
	BEQ.B	lbC000820
	ADD.B	($2A,A3),D0
lbC000820	LSL.W	#1,D0
	MOVE.W	(A0,D0.W),($22,A3)
	MOVE.W	(A0,D0.W),($20,A3)
	MOVE.L	($10,A3),A0
	CLR.B	(10,A0)
	CLR.W	(2,A0)
	CLR.W	(0,A0)
	CLR.B	(13,A0)
	CLR.B	(12,A0)
	BSET	#0,(11,A0)
	BCLR	#2,(11,A0)
	MOVE.L	($14,A3),A0
	CLR.B	(10,A0)
	CLR.W	(2,A0)
	CLR.W	(0,A0)
	CLR.B	(13,A0)
	CLR.B	(12,A0)
	BSET	#0,(11,A0)
	BCLR	#2,(11,A0)
	BRA.W	lbC00090E

lbC00087A	BTST	#6,D0
	BNE.B	lbC0008AC
	BTST	#5,D0
	BEQ.B	lbC000892
	AND.W	#$1F,D0
	MOVE.B	D0,($18,A6)
	BRA.W	lbC000788

lbC000892	AND.B	#15,D0
	LEA	(lbW000A58),A0
	MOVE.B	(A0,D0.W),($29,A3)
	MOVE.B	(A0,D0.W),($28,A3)
	BRA.W	lbC000788

lbC0008AC	BTST	#5,D0
	BNE.B	lbC0008FC
	MOVE.L	(4,A6),A0
	AND.W	#$1F,D0
	MULU	#13,D0
	LEA	(A0,D0.W),A0
	MOVE.L	($10,A3),A2
	MOVE.B	(A0),D0
	AND.B	#$80,D0
	MOVE.B	D0,(11,A2)
	MOVE.B	(A0)+,D0
	AND.W	#$7F,D0
	MOVE.B	D0,(8,A2)
	CLR.B	(10,A2)
	CLR.W	(2,A2)
	CLR.W	(0,A2)
	CLR.B	(13,A2)
	CLR.B	(12,A2)
	BSET	#0,(11,A2)
	MOVE.L	A0,(4,A2)
	BRA.W	lbC000788

lbC0008FC	LEA	(lbL000A44),A0
	AND.W	#7,D0
	LSL.W	#2,D0
	MOVE.L	(A0,D0.W),-(SP)
	RTS

lbC00090E	MOVEQ	#0,D0
	MOVE.L	($10,A3),A0
	BSR.W	lbC000692
	MOVEQ	#0,D1
	MOVE.B	($2B,A3),D1
	CMP.B	($1D,A6),D1
	BLE.B	lbC000928
	MOVE.B	($1D,A6),D1
lbC000928	MOVEQ	#$3F,D2
	SUB.W	D1,D2
	SUB.W	D2,D0
	BCC.B	lbC000932
	MOVEQ	#0,D0
lbC000932	AND.W	#$FF,D0
	MOVE.W	D0,(0,A6)
	MOVE.L	($14,A3),A0
	MOVE.W	($20,A3),D0
	MOVE.L	A1,-(SP)
	BSR.W	lbC000692
	MOVE.L	(SP)+,A1
	MOVE.W	D0,(2,A6)
lbC00094E	RTS

lbC000950	MOVEM.L	D1/D2/A1,-(SP)
	MOVEQ	#0,D1
	MOVE.W	D7,D1
	LSL.W	#4,D1
	MOVEQ	#1,D2
	LSL.W	D7,D2
	MOVE.L	#$DFF0A0,A1
	MOVE.W	#0,(8,A1,D1.W)	;zero volume
	MOVE.W	D2,($DFF096)	;disable channel dma
	MOVEM.L	(SP)+,D1/D2/A1
	RTS

lbC000976	MOVE.L	(8,A3),A0
	MOVE.B	(A0)+,($2A,A3)
	MOVE.L	A0,(8,A3)
	BRA.W	lbC000788

lbC000986	MOVE.L	(8,A3),A0
	MOVE.B	(A0)+,D0
	MOVE.L	A0,(8,A3)
	MOVE.L	(8,A6),A0
	AND.W	#$FF,D0
	MULU	#13,D0
	LEA	(A0,D0.W),A0
	MOVE.L	($14,A3),A2
	MOVE.B	(A0)+,D0
	CLR.B	(11,A2)
	CLR.B	(8,A2)
	CLR.B	(10,A2)
	CLR.W	(2,A2)
	CLR.W	(0,A2)
	CLR.B	(13,A2)
	CLR.B	(12,A2)
	BSET	#0,(11,A2)
	MOVE.L	A0,(4,A2)
	BRA.W	lbC000788

lbC0009D0	MOVE.L	(8,A3),A0
	MOVE.B	(A0)+,D0
	MOVE.L	A0,(8,A3)
	MOVE.L	(8,A6),A0
	AND.W	#$FF,D0
	MULU	#13,D0
	LEA	(A0,D0.W),A0
	MOVE.L	($14,A3),A2
	MOVE.B	(A0)+,D0
	MOVE.B	#$80,(11,A2)
	CLR.B	(8,A2)
	CLR.B	(10,A2)
	CLR.W	(2,A2)
	CLR.W	(0,A2)
	CLR.B	(13,A2)
	CLR.B	(12,A2)
	BSET	#0,(11,A2)
	MOVE.L	A0,(4,A2)
	BRA.W	lbC000788

lbC000A1C	BRA.W	lbC000788

SampleDataPtr	dc.l	0
lbL000A24	dcb.l	5,0
	dc.l	lbL000CF8
	dc.l	0
	dc.l	$800
lbL000A44	dc.l	lbC000976
	dc.l	lbC0009D0
	dc.l	lbC000986
	dc.l	lbC000A1C
	dc.l	$1020408
lbW000A58	dc.w	$203
	dc.w	$406
	dc.w	$80C
	dc.w	$1018
	dc.w	$2030
	dc.w	$4060
lbW000A64	dc.w	$800
	dc.l	lbC0003C0
	dc.w	$400
	dc.l	lbC0003B2
	dc.w	$1000
	dc.l	lbC0003CA
	dc.w	0
lbL000A78	dc.l	0
	dc.l	lbL000A78
	dcb.l	2,0
	dc.l	lbL000AA8
	dc.l	lbL000AB8
	dc.l	lbL000CF8
	dc.l	lbL000BB8
	dcb.l	3,0
	dc.l	$FF00
lbL000AA8	dcb.l	4,0
lbL000AB8	dcb.l	4,0
lbL000AC8	dc.l	0
	dc.l	lbL000AC8
	dcb.l	2,0
	dc.l	lbL000AF8
	dc.l	lbL000B08
	dc.l	lbL000CF8
	dc.l	lbL000C08
	dcb.l	3,0
	dc.l	$FF00
lbL000AF8	dcb.l	4,0
lbL000B08	dcb.l	4,0
lbL000B18	dc.l	0
	dc.l	lbL000B18
	dcb.l	2,0
	dc.l	lbL000B48
	dc.l	lbL000B58
	dc.l	lbL000CF8
	dc.l	lbL000C58
	dcb.l	3,0
	dc.l	$FF00
lbL000B48	dcb.l	4,0
lbL000B58	dcb.l	4,0
lbL000B68	dc.l	0
	dc.l	lbL000B68
	dcb.l	2,0
	dc.l	lbL000B98
	dc.l	lbL000BA8
	dc.l	lbL000CF8
	dc.l	lbL000CA8
	dcb.l	3,0
	dc.l	$FF00
lbL000B98	dcb.l	4,0
lbL000BA8	dcb.l	4,0
lbL000BB8	dc.l	0
	dc.l	lbL000A78
	dcb.l	2,0
	dc.l	lbL000BE8
	dc.l	lbL000BF8
	dc.l	lbL000D00
	dcb.l	4,0
	dc.l	$FF00
lbL000BE8	dcb.l	4,0
lbL000BF8	dcb.l	4,0
lbL000C08	dc.l	0
	dc.l	lbL000AC8
	dcb.l	2,0
	dc.l	lbL000C38
	dc.l	lbL000C48
	dc.l	lbL000D00
	dcb.l	4,0
	dc.l	$FF00
lbL000C38	dcb.l	4,0
lbL000C48	dcb.l	4,0
lbL000C58	dc.l	0
	dc.l	lbL000B18
	dcb.l	2,0
	dc.l	lbL000C88
	dc.l	lbL000C98
	dc.l	lbL000D00
	dcb.l	4,0
	dc.l	$FF00
lbL000C88	dcb.l	4,0
lbL000C98	dcb.l	4,0
lbL000CA8	dc.l	0
	dc.l	lbL000B68
	dcb.l	2,0
	dc.l	lbL000CD8
	dc.l	lbL000CE8
	dc.l	lbL000D00
	dcb.l	4,0
	dc.l	$FF00
lbL000CD8	dcb.l	4,0
lbL000CE8	dcb.l	4,0
lbL000CF8	dc.l	0
	dc.l	$3F00
lbL000D00	dc.l	0
	dc.l	$3F00
	dc.l	$1005A
	dc.l	$7F5A00A6
	dc.l	$81A67F7F
	dc.l	$81817F7F
	dc.w	$8181
lbW000D1A	dc.w	$6ACC
	dc.w	$64CC
	dc.w	$5F25
	dc.w	$59CE
	dc.w	$54C3
	dc.w	$5003
	dc.w	$4B86
	dc.w	$4747
	dc.w	$4346
	dc.w	$3F8B
	dc.w	$3BF3
	dc.w	$3892
	dc.w	$3568
	dc.w	$3269
	dc.w	$2F93
	dc.w	$2CEA
	dc.w	$2A66
	dc.w	$2801
	dc.w	$2566
	dc.w	$23A5
	dc.w	$21AF
	dc.w	$1FC4
	dc.w	$1DFE
	dc.w	$1C4E
	dc.w	$1ABC
	dc.w	$1936
	dc.w	$17CC
	dc.w	$1676
	dc.w	$1533
	dc.w	$1401
	dc.w	$12E4
	dc.w	$11D5
	dc.w	$10D4
	dc.w	$FE3
	dc.w	$EFE
	dc.w	$E26
	dc.w	$D5B
	dc.w	$C9B
	dc.w	$BE5
	dc.w	$B3B
	dc.w	$A9B
	dc.w	$A02
	dc.w	$972
	dc.w	$8E9
	dc.w	$869
	dc.w	$7F1
	dc.w	$77F
	dc.w	$713
	dc.w	$6AD
	dc.w	$64D
	dc.w	$5F2
	dc.w	$59D
	dc.w	$54D
	dc.w	$500
	dc.w	$4B8
	dc.w	$475
	dc.w	$435
	dc.w	$3F8
	dc.w	$3BF
	dc.w	$38A
	dc.w	$356
	dc.w	$326
	dc.w	$2F9
	dc.w	$2CF
	dc.w	$2A6
	dc.w	$280
	dc.w	$25C
	dc.w	$23A
	dc.w	$21A
	dc.w	$1FC
	dc.w	$1E0
	dc.w	$1C5
	dc.w	$1AB
	dc.w	$193
	dc.w	$17D
	dc.w	$167
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
	dc.w	$AA
	dc.w	$A0
	dc.w	$97
	dc.w	$8F
	dc.w	$87
	dc.w	$7F
	dc.w	$78
	dc.w	$70
	dc.w	$60
	dc.w	$50
	dc.w	$40
	dc.w	$30
	dc.w	$20
	dc.w	$10
	dcb.w	2,0
	dc.w	$20
	dcb.w	4,$2020
	dcb.w	2,$3030
	dc.w	$3020
	dcb.w	8,$2020
	dc.w	$2090
	dcb.w	7,$4040
	dc.w	$400C
	dcb.w	4,$C0C
	dc.w	$C40
	dcb.w	3,$4040
	dcb.w	3,$909
	dcb.w	10,$101
	dcb.w	3,$4040
	dcb.w	3,$A0A
	dcb.w	10,$202
	dcb.w	2,$4040
	dc.w	$2000

	section	bss,bss
epoptions	ds.b	EP_OPTION_SIZE

	end
